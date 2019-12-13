#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include "logic_ast.h"
#include "common.h"

using namespace std;

// class Location {
//   int lineno;
// };

class Program;

enum class VarType {
  INPUT,
  OUTPUT,
  REG,
  WIRE
};

class Type {
public:
  bool _reg;
  int _size;

  Type();
  Type(bool reg, int size);
  bool weak_eq(Type t2);
  bool eq(const Type t2) const;
};

inline bool operator==(const Type& lhs, const Type& rhs) {
  return lhs.eq(rhs);
}

enum class ArithType {
  SUM,
  CAT,
  AND,
  OR,
  XOR,
  EQ
};

typedef shared_ptr<unordered_map<Var, pair<Type, bool>>> VarMap;

class ParseNumExpression {
public:
  LNP node;

  virtual ~ParseNumExpression() { }
  // virtual void check() = 0;
  virtual Type type() = 0;
  virtual LNP get_node(Program& p) = 0;
};

typedef shared_ptr<ParseNumExpression> PNum;

class ParseNumExpressionLit : public ParseNumExpression {
  int _val;
  int _size;

public:
  ParseNumExpressionLit(int val, int size);
  Type type();
  LNP get_node(Program& p);
};

class ParseNumExpressionArith : public ParseNumExpression {
  PNum _v1;
  PNum _v2;
  ArithType _aty;
  Type _ty;

public:
  ParseNumExpressionArith(PNum v1, PNum v2, ArithType aty);
  Type type();
  LNP get_node(Program& p);
};

class ParseNumExpressionSubscript : public ParseNumExpression {
  PNum _v;
  int _ind;

public:
  ParseNumExpressionSubscript(PNum v, int ind);
  Type type();
  LNP get_node(Program& p);
};

class ParseNumExpressionVar : public ParseNumExpression {
  Var _v;
  Type _ty;

public:
  ParseNumExpressionVar(shared_ptr<Program> p, Var v);
  Type type();
  LNP get_node(Program& p);
};

PNum Int(int i, int size);
PNum Plus(PNum a, PNum b);
PNum Cat(PNum a, PNum b);
PNum Sub(PNum a, int ind);
PNum And(PNum a, PNum b);
PNum Or(PNum a, PNum b);
PNum Xor(PNum a, PNum b);
PNum V(shared_ptr<Program> p, Var v);


// class ParseBoolExpression {
// public:
//   // virtual void check() = 0;
// };
//
// typedef shared_ptr<ParseBoolExpression> PBool;

PNum EQ(PNum a, PNum b, VarMap vm);

class Statement {
public:
  // virtual void check() = 0;
  virtual void addNodes(Program& p) = 0;
  virtual ~Statement() { }
};

class AssignStatement : public Statement {
  Var _v;
  PNum _expr;
public:
  AssignStatement(Var v, PNum expr, VarMap vm);
  void addNodes(Program& p);
  // virtual void check() = 0;
};

shared_ptr<AssignStatement> Assign(string v, PNum expr, VarMap vm);

class Program {
  vector<Var> inputs;
  vector<Var> outputs;
  vector<shared_ptr<Statement>> lines;
  VarMap _vm;
  vector<VarMap> _vmstack;
  bool decls_done = false;

  vector<LNP> all_nodes;
  vector<LNP> input_nodes;
  vector<LNP> output_nodes;
  vector<LNP> reg_nodes;

  vector<LNP> cond_stack;
  vector<vector<LNP>> mux_stack;
  vector<vector<LNP>> nodes_to_mux_stack;


public:
  Program();
  void decl(Var v, VarType vty, int size);
  void add(shared_ptr<Statement> stmt);
  void IF(PNum cond);
  void ELSE();
  void ENDIF();
  // void check();
  // shared_ptr<LogicNode> make_logic();
  VarMap vm();

  LNP newNode(NodeType t, int size, int val = 0);

  LNP varToNode(Var v);

  void assignNodeToVar(LNP n, Var v);

  shared_ptr<LogicGraph> genLogicGraph();

  unordered_map<Var, LNP> varNodeMap;
};
