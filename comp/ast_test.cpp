#include <iostream>
#include <memory>
#include <algorithm>
#include "ast.hpp"
#include "strvisitor.hpp"

int main(int argc, char* argv[])
{
    std::shared_ptr<Empty> blank;
    std::shared_ptr<Program> prg(new Program());

    std::shared_ptr<SharedDecl> shDecl1(new SharedDecl("y", blank));
    prg->Add(shDecl1);

    std::shared_ptr<Node> initX(new Atom("2"));
    std::shared_ptr<SharedDecl> shDecl2(new SharedDecl("x", initX));
    prg->Add(shDecl2);

    std::vector<std::string> sub1params;
    sub1params.push_back("a");
    sub1params.push_back("b");
    std::shared_ptr<Sub> sub1(new Sub("Main", sub1params));
    prg->Add(sub1);

    std::shared_ptr<Atom> atomA(new Atom("a"));
    std::shared_ptr<Atom> atomB(new Atom("b"));
    std::shared_ptr<BinaryOp> initRes(new BinaryOp("add", atomA, atomB));
    std::shared_ptr<VarDecl> resDecl(new VarDecl("res", initRes));
    sub1->Add(resDecl);

    std::shared_ptr<Atom> atomC(new Atom("res"));
    std::shared_ptr<BinaryOp> subCsumAB(new BinaryOp("substract", atomC, initRes));
    std::shared_ptr<Assignation> incC(new Assignation(atomC, subCsumAB));
    sub1->Add(incC);

    std::shared_ptr<Atom> atom0(new Atom("0"));
    std::shared_ptr<BinaryOp> cond1cond(new BinaryOp("equals", atomC, atom0));
    std::shared_ptr<If> cond1(new If(cond1cond, "10", "20"));
    sub1->Add(cond1);

    std::shared_ptr<Atom> atom1(new Atom("1"));
    std::shared_ptr<Atom> atom10(new Atom("10"));
    std::shared_ptr<For> for1(new For("i", atom1, atom10, atom1));
    sub1->Add(for1);

    std::shared_ptr<BinaryOp> addC1(new BinaryOp("add", atomC, atom1));
    std::shared_ptr<Assignation> incResActually(new Assignation(atomC, addC1));
    for1->Add(incResActually);

    std::shared_ptr<Loop> loop1(new Loop(cond1cond));
    loop1->Add(for1); // don't double reference ever in practice...
    loop1->Add(addC1);
    sub1->Add(loop1);

    std::shared_ptr<Call> call1(new Call("testFun", ""));
    call1->AddParam(atomA);
    call1->AddParam(addC1);
    sub1->Add(call1);

    std::shared_ptr<Return> ret1(new Return(atom0));
    sub1->Add(ret1);

    StringDumpVisitor v;
    prg->Accept(&v);

    return 0;
}
