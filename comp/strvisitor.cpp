#include "ast.hpp"
#include <algorithm>
#include <iostream>

void Visitor::Visit(Program& node)
{
    std::cout << "<program>" << std::endl;
    std::for_each(node.Children().begin(), node.Children().end(), [&](std::shared_ptr<Node> const& n){
        n->Accept(this);
    });
    std::cout << "</program>" << std::endl;
}

void Visitor::Visit(If& node)
{
    std::cout << "<if true=" << node.Truth() << " false=" << node.Otherwise() << ">" << std::endl;
    node.Operand()->Accept(this);
    std::cout << "</if>" << std::endl;
}

void Visitor::Visit(For& node)
{
    std::cout << "<for name=" << node.VarName() << ">" << std::endl;

    std::cout << "<begin>" << std::endl;
    node.Begin()->Accept(this);
    std::cout << "</begin>" << std::endl;

    std::cout << "<end>" << std::endl;
    node.End()->Accept(this);
    std::cout << "</end>" << std::endl;

    std::cout << "<step>" << std::endl;
    node.Step()->Accept(this);
    std::cout << "</step>" << std::endl;

    std::cout << "<body>" << std::endl;
    std::for_each(node.Children().begin(), node.Children().end(), [&](std::shared_ptr<Node> const& n){
        n->Accept(this);
    });
    std::cout << "</body>" << std::endl;

    std::cout << "</for>" << std::endl;
}

void Visitor::Visit(Loop& node)
{
    std::cout << "<loop>" << std::endl;
    if(node.Condition()) {
        std::cout << "<condition>" << std::endl;
        node.Condition()->Accept(this);
        std::cout << "</condition>" << std::endl;
    }
    std::cout << "<body>" << std::endl;
    std::for_each(node.Children().begin(), node.Children().end(), [&](std::shared_ptr<Node> const& n){
        n->Accept(this);
    });
    std::cout << "</body>" << std::endl;
    
    std::cout << "</loop>" << std::endl;
}

void Visitor::Visit(VarDecl& node)
{
}

void Visitor::Visit(SharedDecl& node)
{
    std::cout << "<shared name=" << node.Name() << ">" << std::endl;
    if(node.Initialization()) {
        node.Initialization()->Accept(this);
    }
    std::cout << "</shared>" << std::endl;
}
void Visitor::Visit(Call& node) {}
void Visitor::Visit(Sub& node) {}
void Visitor::Visit(BinaryOp& node) {}
void Visitor::Visit(UnaryOp& node) {}
void Visitor::Visit(VarId& node) {}
void Visitor::Visit(Assignation& node) {}
void Visitor::Visit(Atom& node)
{
    std::cout << "<atom value=" << node.Thing() << ">" << std::endl;
}
void Visitor::Visit(RefVar& node) {}
void Visitor::Visit(Jumpable& node) {}
void Visitor::Visit(Labelled& node) {}
void Visitor::Visit(Return& node) {}
void Visitor::Visit(Empty& node) {}
