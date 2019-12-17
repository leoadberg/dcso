#include "logic_ast.h"

int LogicNode::getVal(LogicGraph* lg) {
  if (lg->iter == setIter) return val;

  switch (ty) {
    case NodeType::IDENTITY:
      val = inputs.at(0)->getVal(lg);
      break;
    case NodeType::LIT:
      break;
    case NodeType::REG_SRC:
      if (lg->iter == 1) {
        val = 0;
      } else {
        ERROR("Reg should be set already");
      }
      break;
    case NodeType::REG_DEST:
      val = inputs.at(0)->getVal(lg);
      break;
    case NodeType::INPUT:
      // val is set externally
      // val = lg->inputValues[v];
      break;
    case NodeType::SUM:
      val = inputs.at(0)->getVal(lg) + inputs.at(1)->getVal(lg);
      break;
    case NodeType::CAT:
      val = (inputs.at(0)->getVal(lg) << inputs.at(0)->size) + inputs.at(1)->getVal(lg);
      break;
    case NodeType::EQ:
      val = inputs.at(0)->getVal(lg) == inputs.at(1)->getVal(lg);
      break;
    case NodeType::SUB:
      val = (inputs.at(0)->getVal(lg) >> aux) & 1;
      break;
    case NodeType::AND:
      val = inputs.at(0)->getVal(lg) & inputs.at(1)->getVal(lg);
      break;
    case NodeType::OR:
      val = inputs.at(0)->getVal(lg) | inputs.at(1)->getVal(lg);
      break;
    case NodeType::XOR:
      val = inputs.at(0)->getVal(lg) ^ inputs.at(1)->getVal(lg);
      break;
    case NodeType::MUX:
      val = inputs.at(0)->getVal(lg) ? inputs.at(1)->getVal(lg) : inputs.at(2)->getVal(lg);
      break;
    default:
      ERROR("Unknown NodeType");
  }

  val = val & ((1 << size) - 1);

  setIter = lg->iter;
  return val;
}

string LogicNode::repr() {
  switch (ty) {
    case NodeType::IDENTITY:
    // return "I(" + inputs.at(0)->repr() + ")";
      return inputs.at(0)->repr();
    case NodeType::LIT:
      return to_string(val);
    case NodeType::REG_SRC:
    case NodeType::REG_DEST:
    case NodeType::INPUT:
      return v;
    case NodeType::SUM:
      return "(" + inputs.at(0)->repr() + " + " + inputs.at(1)->repr() + ")";
    case NodeType::CAT:
      return "(" + inputs.at(0)->repr() + " . " + inputs.at(1)->repr() + ")";
    case NodeType::EQ:
      return "(" + inputs.at(0)->repr() + " == " + inputs.at(1)->repr() + ")";
    case NodeType::SUB:
      return inputs.at(0)->repr() + "[" + to_string(aux) + "]";
    case NodeType::AND:
      return "(" + inputs.at(0)->repr() + " & " + inputs.at(1)->repr() + ")";
    case NodeType::OR:
      return "(" + inputs.at(0)->repr() + " | " + inputs.at(1)->repr() + ")";
    case NodeType::XOR:
      return "(" + inputs.at(0)->repr() + " ^ " + inputs.at(1)->repr() + ")";
    case NodeType::MUX:
      return "(" + inputs.at(0)->repr() + " ? " + inputs.at(1)->repr() + " : " + inputs.at(2)->repr() + ")";
    default:
      ERROR("Unknown NodeType");
  }
  return "error";
}

float LogicNode::weight() {
  if (_weight >= 0) return _weight;

  switch (ty) {
    case NodeType::IDENTITY:
      _weight = inputs.at(0)->weight();
      break;
    case NodeType::LIT:
    case NodeType::REG_SRC:
    case NodeType::REG_DEST:
    case NodeType::INPUT:
      _weight = 0;
      break;
    case NodeType::SUB:
      _weight = 1 + inputs.at(0)->weight();
      break;
    case NodeType::SUM:
    case NodeType::CAT:
    case NodeType::EQ:
    case NodeType::AND:
    case NodeType::OR:
    case NodeType::XOR:
      _weight = 1 + max(inputs.at(0)->weight(), inputs.at(1)->weight());
      break;
    case NodeType::MUX:
      _weight = 1 + max(max(inputs.at(0)->weight(), inputs.at(1)->weight()), inputs.at(2)->weight());
      break;
    default:
      ERROR("Unknown NodeType");
  }

  return _weight;
}

void LogicNode::addInput(LNP inp) {
  inputs.push_back(inp);
  inp->outputs.insert(shared_from_this());
}

bool LogicNode::dead() {
  return ty == NodeType::DEAD;
}

bool LogicNode::depends(LNP other) {
  if (shared_from_this() == other) return true;

  for (LNP in : inputs) {
    if (in->depends(other)) return true;
  }

  return false;
}


void LogicGraph::optimize() {
  int statebits = 0;
  vector<LNP> state_nodes;
  state_nodes.insert(state_nodes.begin(), input_nodes.begin(), input_nodes.end());
  state_nodes.insert(state_nodes.begin(), reg_nodes.begin(), reg_nodes.end());

  for (LNP n : state_nodes) {
    statebits += n->size;
  }

  if (statebits > 16) ERROR("State size too big to optimize");

  while (true) {
    reset();

    // First, generate truth tables for every node
    vector<string> truth_tables(all_nodes.size());
    for (int curstate = 0; curstate < (1 << statebits); curstate++) {
      iter++;
      int bit_ind = 0;
      for (LNP n : state_nodes) {
        // Shift and mask relevant bits
        n->val = (curstate >> bit_ind) & ((1 << n->size) - 1);
        bit_ind += n->size;

        n->setIter = iter;
      }

      for (int i = 0; i < all_nodes.size(); i++) {
        LNP n = all_nodes[i];
        truth_tables[i] += to_string(n->getVal(this)) + ",";
      }
    }

    vector<size_t> node_hashes(all_nodes.size());

    hash<string> hasher;
    unordered_map<size_t, pair<LNP, float>> min_weight_gadgets;

    for (int i = 0; i < all_nodes.size(); i++) {
      LNP n = all_nodes[i];
      if (n->dead()) continue;

      float w = n->weight();
      size_t tt_hash = hasher(truth_tables[i]);
      node_hashes[i] = tt_hash;

      if (min_weight_gadgets.count(tt_hash) == 0
       || min_weight_gadgets[tt_hash].second > w) {
        min_weight_gadgets[tt_hash] = make_pair(n, w);
      }
    }

    bool combined = false;
    for (int i = 0; i < all_nodes.size(); i++) {
      LNP n = all_nodes[i];
      if (n->dead()) continue;

      size_t tt_hash = node_hashes[i];
      float w = n->weight();

      auto minpair = min_weight_gadgets[tt_hash];
      if (minpair.second + 0.001 < w && !minpair.first->depends(n)) {
        combineNodes(n, minpair.first);
        combined = true;
      }
    }

    if (!combined) {
      cout << "Found no reductions, optimization over" << endl;
      break;
    }
  }

  reset();
}

void LogicGraph::combineNodes(LNP oldNode, LNP newNode) {
  cout << "Reducing:  " << oldNode->repr() << endl
       << "into:      " << newNode->repr() << endl << endl;

  // Remove pointers to oldNode from its inputs
  for (LNP in : oldNode->inputs) {
    in->outputs.erase(oldNode);
  }
  oldNode->inputs.clear();

  // Turn oldNode into an IDENTITY of newNode
  oldNode->inputs.push_back(newNode);
  if (oldNode->ty != NodeType::REG_DEST) {
    oldNode->ty = NodeType::IDENTITY;
  }
}

void LogicGraph::reset() {
  iter = 0;
  for (LNP node : all_nodes) {
    node->setIter = 0;
    node->_weight = -1;
    if (node->ty == NodeType::REG_SRC) {
      node->val = 0;
    }
  }
}

void LogicGraph::iterate(unordered_map<Var, int> inputValues) {
  iter++;

  cout << endl << "*** Cycle " << iter << " ***" << endl;

  for (LNP in : input_nodes) {
    if (inputValues.count(in->v) == 0) {
      in->val = 0;
    } else {
      in->val = inputValues[in->v];
    }
  }

  for (LNP node : all_nodes) {
    if (node->dead()) continue;

    node->getVal(this);
  }

  for (LNP node : reg_nodes) {
    node->val = node->dst->getVal(this);
    node->setIter = iter + 1;
  }
}

void LogicGraph::print_outputs() {
  for (LNP n : output_nodes) {
    cout << n->v << " = " << n->val << " = " << n->repr() << endl;
  }
}
