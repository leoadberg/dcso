#include "parse_ast.h"

using namespace std;

int main() {
  auto prog = make_shared<Program>();

  prog->decl("x", VarType::INPUT, 2);
  prog->decl("y", VarType::INPUT, 2);

  prog->decl("z", VarType::OUTPUT, 2);
  prog->decl("iter", VarType::OUTPUT, 4);

  prog->decl("it", VarType::REG, 4);

  /*
  if (it == 1) {
    z = y | (y & x)
  } else {
    z = (x + 1) - 1
  }
  */
  prog->IF(EQ(V(prog, "it"), Int(1, 4), prog->vm()));
    prog->add(Assign("z", Or(V(prog, "y"), And(V(prog, "y"), V(prog, "x"))), prog->vm()));
  prog->ELSE();
    prog->add(Assign("z", Plus(Plus(V(prog, "x"), Int(1, 2)), Int(-1, 2)), prog->vm()));
  prog->ENDIF();


  prog->add(Assign("it", Plus(V(prog, "it"), Int(1, 4)), prog->vm()));
  prog->add(Assign("iter", V(prog, "it"), prog->vm()));

  auto LG = prog->genLogicGraph();

  LG->optimize();

  for (int i = 0; i < 5; i++) {
    LG->iterate({{"x", 1}, {"y", 2}});
    LG->print_outputs();
  }

  return 0;
}
