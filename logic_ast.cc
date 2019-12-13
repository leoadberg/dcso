#include "logic_ast.h"

int LogicNode::getVal(LogicGraph* lg, int iteration) {
  if (iteration == setIter) return val;

  switch (ty) {
    case NodeType::IDENTITY:
      val = inputs.at(0)->getVal(lg, iteration);
      break;
    case NodeType::LIT:
      break;
    case NodeType::REG_SRC:
      if (iteration == 1) {
        val = 0;
      } else {
        ERROR("Reg should be set already");
      }
      break;
    case NodeType::REG_DEST:
      val = inputs.at(0)->getVal(lg, iteration);
      break;
    case NodeType::INPUT:
      val = lg->inputValues[v];
      break;
    case NodeType::SUM:
      val = inputs.at(0)->getVal(lg, iteration) + inputs.at(1)->getVal(lg, iteration);
      break;
    case NodeType::CAT:
      val = (inputs.at(0)->getVal(lg, iteration) << inputs.at(0)->size) + inputs.at(1)->getVal(lg, iteration);
      break;
    case NodeType::EQ:
      val = inputs.at(0)->getVal(lg, iteration) == inputs.at(1)->getVal(lg, iteration);
      break;
    case NodeType::SUB:
      val = (inputs.at(0)->getVal(lg, iteration) >> aux) & 1;
      break;
    case NodeType::AND:
      val = inputs.at(0)->getVal(lg, iteration) & inputs.at(1)->getVal(lg, iteration);
      break;
    case NodeType::OR:
      val = inputs.at(0)->getVal(lg, iteration) | inputs.at(1)->getVal(lg, iteration);
      break;
    case NodeType::XOR:
      val = inputs.at(0)->getVal(lg, iteration) ^ inputs.at(1)->getVal(lg, iteration);
      break;
    case NodeType::MUX:
      val = inputs.at(0)->getVal(lg, iteration) ? inputs.at(1)->getVal(lg, iteration) : inputs.at(2)->getVal(lg, iteration);
      break;
    default:
      ERROR("Unknown NodeType");
  }

  setIter = iteration;
  return val;
}

string LogicNode::repr() {
  switch (ty) {
    case NodeType::IDENTITY:
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
}

void LogicNode::addInput(LNP inp) {
  inputs.push_back(inp);
  inp->outputs.push_back(shared_from_this());
}

void LogicGraph::optimize() {
  int statesize = 0;
  for (LNP n : input_nodes) {
    statesize += n->size;
  }
  for (LNP n : reg_nodes) {
    statesize += n->size;
  }

  if (statesize > 16) ERROR("State size too big to optimize");
}

void LogicGraph::iterate() {
  iter++;

  cout << endl << "*** Iteration " << iter << " ***" << endl;

  for (LNP node : all_nodes) {
    node->getVal(this, iter);
  }

  for (LNP node : reg_nodes) {
    node->val = node->dst->getVal(this, iter);
    node->setIter = iter + 1;
  }
}

void LogicGraph::print_outputs() {
  for (LNP n : output_nodes) {
    cout << n->v << " = " << n->val << " = " << n->repr() << endl;
  }
}
