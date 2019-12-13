#include "parse_ast.h"

using namespace std;

int main() {
  auto prog = make_shared<Program>();

  prog->decl("x", VarType::INPUT, 2);
  prog->decl("y", VarType::INPUT, 2);

  prog->decl("z", VarType::OUTPUT, 2);
  prog->decl("iter", VarType::OUTPUT, 4);

  prog->decl("it", VarType::REG, 4);

  // prog->add(Assign("z", Cat(V(prog, "it"), Int(0, 2)), prog->vm()));
  prog->IF(EQ(V(prog, "it"), Int(1, 4), prog->vm()));
    prog->add(Assign("z", Int(3, 2), prog->vm()));
  prog->ELSE();
    prog->add(Assign("z", Int(1, 2), prog->vm()));
  prog->ENDIF();


  prog->add(Assign("it", Plus(V(prog, "it"), Int(1, 4)), prog->vm()));
  prog->add(Assign("iter", V(prog, "it"), prog->vm()));

  auto LG = prog->genLogicGraph();

  for (int i = 0; i < 5; i++) {
    LG->iterate();
    LG->print_outputs();
  }

  return 0;
}
