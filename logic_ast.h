#include "common.h"
#include <vector>
#include <unordered_map>
#include <memory>

enum class NodeType {
  IDENTITY,
  LIT,
  REG_SRC,
  REG_DEST,
  INPUT,
  SUM,
  CAT,
  EQ,
  SUB,
  AND,
  OR,
  XOR,
  MUX,
};

class LogicGraph;
class LogicNode;
typedef shared_ptr<LogicNode> LNP;

class LogicNode : public enable_shared_from_this<LogicNode> {
public:

  vector<LNP> inputs;
  vector<LNP> outputs;
  NodeType ty;
  int id;

  int val;
  int size;

  // For vars
  Var v;

  // For registers
  LNP dst;

  // Which iteration the value was set on
  int setIter;

  // If needed (e.g. in Sub)
  int aux;

  int getVal(LogicGraph* lg, int iteration);
  void addInput(LNP inp);
  string repr();
};

typedef shared_ptr<LogicNode> LNP;

class LogicGraph {
public:
  unordered_map<Var, int> inputValues;
  vector<LNP> all_nodes;
  vector<LNP> input_nodes;
  vector<LNP> output_nodes;
  vector<LNP> reg_nodes;
  int iter;

  void optimize();
  void iterate();

  void print_outputs();
};
