#include <iostream>
#include <memory>
#include <algorithm>
#include "ast.hpp"

int main(int argc, char* argv[])
{
    std::shared_ptr<Empty> blank;
    std::shared_ptr<Program> prg(new Program());

    std::shared_ptr<SharedDecl> shDecl1(new SharedDecl("x", blank));
    prg->Add(shDecl1);

    std::shared_ptr<Node> initX(new Atom("2"));
    std::shared_ptr<SharedDecl> shDecl2(new SharedDecl("x", initX));
    prg->Add(shDecl2);

    Visitor v;
    prg->Accept(&v);

    return 0;
}
