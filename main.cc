#include "parse_ast.h"

using namespace std;

int main() {
  auto prog = make_shared<Program>();

  // prog->decl("x", VarType::INPUT, 2);
  // prog->decl("y", VarType::INPUT, 2);
  //
  // prog->decl("z", VarType::OUTPUT, 2);
  // prog->decl("iter", VarType::OUTPUT, 4);
  //
  // prog->decl("it", VarType::REG, 4);
  //
  // /*
  // if (it == 1) {
  //   z = y | (y & x)
  // } else {
  //   z = (x + 1) - 1
  // }
  // */
  // prog->IF(EQ(V(prog, "it"), Int(1, 4), prog->vm()));
  //   prog->add(Assign("z", Or(V(prog, "y"), And(V(prog, "y"), V(prog, "x"))), prog->vm()));
  // prog->ELSE();
    // prog->add(Assign("z", Plus(Plus(V(prog, "y"), Int(1, 2)), Int(-1, 2)), prog->vm()));
  // prog->ENDIF();
  //
  //
  // prog->add(Assign("it", Plus(V(prog, "it"), Int(1, 4)), prog->vm()));
  // prog->add(Assign("iter", V(prog, "it"), prog->vm()));
  //
  // for (int i = 1; i < 4; i++) {
  //   prog->add(Plus(V(prog, "x"), Int(i, 2)));
  // }

  // prog->decl("x", VarType::INPUT, 8);
  // prog->decl("a", VarType::WIRE, 8);
  // prog->decl("b", VarType::WIRE, 8);
  // prog->decl("ret", VarType::OUTPUT, 1);
  // prog->add(Assign("a", And((Plus(V(prog, "x"), Int(1, 8))), Int(1, 8)), prog->vm()));
  // prog->add(Assign("b", Cat(Int(0, 7), EQ((And(V(prog, "x"), Int(1, 8))), Int(0, 8))), prog->vm()));
  // prog->add(Assign("ret", EQ(V(prog, "a"), V(prog, "b")), prog->vm()));

  prog->decl("x", VarType::INPUT, 8);
  prog->decl("ret", VarType::OUTPUT, 8);
  prog->add(Assign("ret", Plus(Plus(Xor(V(prog, "x"), Int(0xfe, 8)), Int(2, 8)), V(prog, "x")), prog->vm()));

  // prog->add(Cat(Int(0, 6), Cat(Sub(V(prog, "x"), 0), Int(0, 1))));
  prog->add(Cat(Sub(V(prog, "x"), 0), Int(0, 1)));

  auto LG = prog->genLogicGraph();

  LG->optimize();

  for (int i = 0; i < 5; i++) {
    LG->iterate({{"x", 1}, {"y", 2}});
    LG->print_outputs();
  }

  return 0;
}
