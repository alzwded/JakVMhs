#ifndef STRVISITOR_HPP
#define STRVISITOR_HPP

#include "ast.hpp"
#include <iostream>

class XMLDumpVisitor
: public Visitor
{
    size_t indentation;

    void Indent()
    {
        for(size_t i = 0; i < indentation; ++i) std::cout << "  ";
    }
public:
    XMLDumpVisitor() : indentation(0) {}

    virtual void Visit(Program& node);
    virtual void Visit(If& node);
    virtual void Visit(For& node);
    virtual void Visit(Loop& node);
    virtual void Visit(VarDecl& node);
    virtual void Visit(SharedDecl& node);
    virtual void Visit(Call& node);
    virtual void Visit(Sub& node);
    virtual void Visit(BinaryOp& node);
    virtual void Visit(UnaryOp& node);
    virtual void Visit(Assignation& node);
    virtual void Visit(Atom& node);
    virtual void Visit(RefVar& node);
    virtual void Visit(Labelled& node);
    virtual void Visit(Return& node);
    virtual void Visit(Empty& node);
};

#endif
