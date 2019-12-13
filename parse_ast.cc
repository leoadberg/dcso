#include "parse_ast.h"

NodeType aty2nty(ArithType aty) {
  switch (aty) {
    case ArithType::SUM:
    return NodeType::SUM;
    case ArithType::CAT:
    return NodeType::CAT;
    case ArithType::EQ:
    return NodeType::EQ;
    case ArithType::AND:
    return NodeType::AND;
    case ArithType::OR:
    return NodeType::OR;
    case ArithType::XOR:
    return NodeType::XOR;
  }
  ERROR("Unknown aty");
}

Type::Type(bool reg, int size) {
  _reg = reg;
  _size = size;
}

Type::Type() {
  _reg = false;
  _size = 0;
}

bool Type::weak_eq(Type t2) {
  return _size == t2._size;
}

bool Type::eq(const Type t2) const {
  return _size == t2._size && _reg == t2._reg;
}

void Program::assignNodeToVar(LNP n, Var v) {
  if (nodes_to_mux_stack.size() == 0) {
    // If not in an IF statement assign to the Var
    LNP dest = varToNode(v);
    if (dest->dst) dest = dest->dst; // Move to dest node if exists
    if (dest->inputs.size() != 0) ERROR("Assigning to non-empty node");
    // cout << "Assigning " << v << " to node " << n->id << endl;
    dest->addInput(n);
  } else {
    // If in an IF statement assign to a temp to MUX

    LNP tempVar = newNode(NodeType::IDENTITY, n->size);
    tempVar->v = v;
    tempVar->addInput(n);

    nodes_to_mux_stack.back().push_back(tempVar);
  }
}

Program::Program() {
  _vm = make_shared<unordered_map<Var, pair<Type, bool>>>();
}

void Program::decl(Var v, VarType vty, int size) {
  if (decls_done) {
    ERROR("Declaration not at top of program");
  }
  if (_vm->count(v) != 0) {
    ERROR("Variable already exists");
  }

  LNP varNode = newNode(NodeType::IDENTITY, size);
  varNode->v = v;
  varNodeMap[v] = varNode;

  bool assigned = false;
  bool reg = false;

  if (vty == VarType::INPUT) {
    varNode->ty = NodeType::INPUT;
    inputs.push_back(v);
    input_nodes.push_back(varNode);
    assigned = true;
  } else if (vty == VarType::OUTPUT) {
    outputs.push_back(v);
    output_nodes.push_back(varNode);
  } else if (vty == VarType::REG) {
    varNode->ty = NodeType::REG_SRC;
    LNP destNode = newNode(NodeType::REG_DEST, size);
    destNode->v = v;
    varNode->dst = destNode;
    reg_nodes.push_back(varNode);
    reg = true;
  }

  auto val = pair<Type, bool>(Type(reg, size), assigned);

  (*_vm)[v] = val;
}

void Program::add(shared_ptr<Statement> stmt) {
  decls_done = true;
  lines.push_back(stmt);
  stmt->addNodes(*this);
}

void Program::IF(PNum cond) {
  if (cond->type()._size != 1) ERROR("Non-boolean value used as condition");

  decls_done = true;

  _vmstack.push_back(_vm);

  VarMap newvm = make_shared<unordered_map<Var, pair<Type, bool>>>();
  newvm->insert(_vm->begin(), _vm->end());
  _vm = newvm;

  nodes_to_mux_stack.push_back({});
  cond_stack.push_back(cond->get_node(*this));
}

void Program::ELSE() {
  VarMap freshvm = _vmstack.back();
  _vmstack[_vmstack.size() - 1] = _vm;
  _vm = freshvm;

  // Get the condition in the IF
  LNP cond_node = cond_stack.back();
  cond_stack.pop_back();

  // Take all the assignments from the IF case
  vector<LNP> nodes_to_mux = nodes_to_mux_stack.back();
  nodes_to_mux_stack.back().clear();

  // Save the partial muxes on the stack
  mux_stack.push_back({});
  for (LNP node : nodes_to_mux) {
    LNP muxnode = newNode(NodeType::MUX, node->size);
    muxnode->addInput(cond_node);
    muxnode->addInput(node);
    muxnode->v = node->v;
    mux_stack.back().push_back(muxnode);
  }
}

void Program::ENDIF() {
  VarMap lastvm = _vmstack.back();
  _vmstack.pop_back();
  if (*lastvm != *_vm) {
    ERROR("Mismatching variable states after if statement");
  }

  // Complete the partial MUXes

  vector<LNP> muxes = mux_stack.back();
  mux_stack.pop_back();

  vector<LNP> else_nodes = nodes_to_mux_stack.back();
  nodes_to_mux_stack.pop_back();

  for (LNP mux : muxes) {
    for (LNP else_node : else_nodes) {
      if (else_node->v == mux->v) {
        mux->addInput(else_node);
        assignNodeToVar(mux, mux->v);
        break;
      }
    }
  }
}

// void Program::check() {
//
// }

VarMap Program::vm() {
  return _vm;
}

AssignStatement::AssignStatement(Var v, shared_ptr<ParseNumExpression> expr, VarMap vm) {
  if (vm->count(v) == 0) {
    ERROR("Undefined variable in assign");
  }

  if ((*vm)[v].second) {
    ERROR("Assigned to variable twice");
  }

  (*vm)[v].second = true;

  Type vty = (*vm)[v].first;
  Type ety = expr->type();

  if (!vty.weak_eq(ety)) {
    ERROR("Type mismatch in assign");
  }

  _v = v;
  _expr = expr;
}

shared_ptr<AssignStatement> Assign(string v, shared_ptr<ParseNumExpression> expr, VarMap vm) {
  return make_shared<AssignStatement>(v, expr, vm);
}

ParseNumExpressionLit::ParseNumExpressionLit(int val, int size) {
  _val = val;
  _size = size;
}

Type ParseNumExpressionLit::type() {
  return Type(false, _size);
}

ParseNumExpressionVar::ParseNumExpressionVar(shared_ptr<Program> p, Var v) {
  _v = v;
  _ty = p->vm()->at(v).first;
}

Type ParseNumExpressionVar::type() {
  return _ty;
}

LNP ParseNumExpressionVar::get_node(Program& p) {
  LNP n = p.varNodeMap.at(_v);

  if (n->ty != NodeType::INPUT
   && n->ty != NodeType::REG_SRC
   && n->ty != NodeType::IDENTITY) {
    ERROR("Using bad var: " + _v);
  }

  return n;
}

PNum Int(int i, int size) {
  return make_shared<ParseNumExpressionLit>(i, size);
}

PNum Plus(PNum a, PNum b) {
  return make_shared<ParseNumExpressionArith>(a, b, ArithType::SUM);
}

PNum Cat(PNum a, PNum b) {
  return make_shared<ParseNumExpressionArith>(a, b, ArithType::CAT);
}

PNum Sub(PNum a, int ind) {
  return make_shared<ParseNumExpressionSubscript>(a, ind);
}

PNum And(PNum a, PNum b) {
  return make_shared<ParseNumExpressionArith>(a, b, ArithType::AND);
}

PNum Or(PNum a, PNum b) {
  return make_shared<ParseNumExpressionArith>(a, b, ArithType::OR);
}

PNum Xor(PNum a, PNum b) {
  return make_shared<ParseNumExpressionArith>(a, b, ArithType::XOR);
}

PNum V(shared_ptr<Program> p, Var v) {
  return make_shared<ParseNumExpressionVar>(p, v);
}


ParseNumExpressionArith::ParseNumExpressionArith(shared_ptr<ParseNumExpression> v1, shared_ptr<ParseNumExpression> v2, ArithType aty) {

  bool inputs_eq = false;

  if (aty == ArithType::CAT) {
    _ty = Type(false, v1->type()._size + v2->type()._size);
  } else if (aty == ArithType::EQ) {
    inputs_eq = true;
    _ty = Type(false, 1);
  } else {
    inputs_eq = true;
    _ty = v1->type();
  }

  if (inputs_eq && !v1->type().weak_eq(v2->type())) {
    ERROR("Type mismatch in arith expr");
  }

  _v1 = v1;
  _v2 = v2;
  _aty = aty;
}

Type ParseNumExpressionArith::type() {
  return _ty;
}

ParseNumExpressionSubscript::ParseNumExpressionSubscript(PNum v, int ind) {
  if (ind < 0 || ind >= v->type()._size) {
    ERROR("Subscript out of bounds");
  }

  _v = v;
  _ind = ind;
}

Type ParseNumExpressionSubscript::type() {
  return Type(false, 1);
}

LNP ParseNumExpressionSubscript::get_node(Program& p) {
  if (node) return node;

  node = p.newNode(NodeType::SUB, 1);
  node->aux = _ind;
  node->addInput(_v->get_node(p));

  return node;
}

PNum EQ(PNum a, PNum b, VarMap vm) {
  if (!a->type().weak_eq(b->type())) {
    ERROR("Type mismatch in EQ");
  }

  return make_shared<ParseNumExpressionArith>(a, b, ArithType::EQ);
}

LNP Program::newNode(NodeType t, int size, int val) {
  LNP n = make_shared<LogicNode>();
  n->id = all_nodes.size();
  n->ty = t;
  n->size = size;
  n->val = val;
  n->_weight = -1;
  all_nodes.push_back(n);

  return n;
}

void AssignStatement::addNodes(Program& p) {
  LNP valnode = _expr->get_node(p);

  p.assignNodeToVar(valnode, _v);
}

LNP Program::varToNode(Var v) {
  if (varNodeMap.count(v) == 0) {
    ERROR("Unknown var: " + v);
  }

  return varNodeMap.at(v);
}

LNP ParseNumExpressionLit::get_node(Program& p) {
  if (node) return node;

  node = p.newNode(NodeType::LIT, _size, _val);

  return node;
}


LNP ParseNumExpressionArith::get_node(Program& p) {
  if (node) return node;

  node = p.newNode(aty2nty(_aty), type()._size);

  node->addInput(_v1->get_node(p));
  node->addInput(_v2->get_node(p));

  return node;
}

shared_ptr<LogicGraph> Program::genLogicGraph() {

  for (auto p : *_vm) {
    if (!p.second.second) {
      ERROR("Var not assigned: " + p.first);
    }
  }

  shared_ptr<LogicGraph> lg = make_shared<LogicGraph>();

  lg->all_nodes = all_nodes;
  lg->input_nodes = input_nodes;
  lg->output_nodes = output_nodes;
  lg->reg_nodes = reg_nodes;
  lg->iter = 0;

  return lg;
}
