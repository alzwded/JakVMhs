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
    std::cout << "<if true='" << node.Truth() << "' false='" << node.Otherwise() << "'>" << std::endl;
    node.Operand()->Accept(this);
    std::cout << "</if>" << std::endl;
}

void Visitor::Visit(For& node)
{
    std::cout << "<for name='" << node.VarName() << "'>" << std::endl;

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
    std::cout << "<var name='" << node.Name();
    if(node.Initialization()) {
        std::cout << "'>" << std::endl;
        node.Initialization()->Accept(this);
        std::cout << "</var>" << std::endl;
    } else {
        std::cout << "'/>" << std::endl;
    }
}

void Visitor::Visit(SharedDecl& node)
{
    std::cout << "<shared name='" << node.Name();
    if(node.Initialization()) {
        std::cout << "'>" << std::endl;
        node.Initialization()->Accept(this);
        std::cout << "</shared>" << std::endl;
    } else {
        std::cout << "'/>" << std::endl;
    }
}

void Visitor::Visit(Call& node)
{
    std::cout << "<call";
    if(!node.From().empty()) std::cout << " from='" << node.From() << "'";
    std::cout << " name='" << node.Name() << "'>" << std::endl;

    std::for_each(node.With().begin(), node.With().end(), [&](std::shared_ptr<Node> const& n){
        std::cout << "<with>" << std::endl;
        n->Accept(this);
        std::cout << "</with>" << std::endl;
    });

    std::cout << "</call>";
}

void Visitor::Visit(Sub& node)
{
    std::cout << "<sub name='" << node.Name() << "'";
    if(!node.Params().empty()) {
        std::cout << " with='";
        std::for_each(node.Params().begin(), node.Params().end(), [&](std::string const& s){
            std::cout << s << ";";
        });
        std::cout << "'";
    }
    std::cout << ">" << std::endl;

    std::for_each(node.Children().begin(), node.Children().end(), [&](std::shared_ptr<Node> const& n){
        n->Accept(this);
    });

    std::cout << "</sub>" << std::endl;
}

void Visitor::Visit(BinaryOp& node)
{
    std::cout << "<" << node.Operation() << ">" << std::endl;
    node.Left()->Accept(this);
    node.Right()->Accept(this);
    std::cout << "</" << node.Operation() << ">" << std::endl;
}

void Visitor::Visit(UnaryOp& node)
{
    std::cout << "<" << node.Operation() << ">" << std::endl;
    node.Operand()->Accept(this);
    std::cout << "</" << node.Operation() << ">" << std::endl;
}

void Visitor::Visit(Assignation& node)
{
    std::cout << "<assign>" << std::endl << "<target>" << std::endl;
    node.Name()->Accept(this);
    std::cout << "</target>" << std::endl;
    std::cout << "<expression>" << std::endl;
    node.Expression()->Accept(this);
    std::cout << "</expression>" << std::endl;
    std::cout << "</assign>" << std::endl;
}

void Visitor::Visit(Atom& node)
{
    std::cout << "<atom name='" << node.Thing() << "'";
    if(node.Deref()) {
        std::cout << ">" << std::endl;
        node.Deref()->Accept(this);
        std::cout << "</atom>" << std::endl;
    } else {
        std::cout << "/>" << std::endl;
    }
}

void Visitor::Visit(RefVar& node)
{
    std::cout << "<ref>" << std::endl; 
    node.Variable()->Accept(this);
    std::cout << "</ref>" << std::endl; 
}

void Visitor::Visit(Labelled& node)
{
    std::cout << "<label name='" << node.Label() << "'>" << std::endl;
    node.Operand()->Accept(this);
    std::cout << "</label>" << std::endl;
}

void Visitor::Visit(Return& node)
{
    std::cout << "<return";
    if(node.Operand()) {
        std::cout << ">" << std::endl;
        node.Operand()->Accept(this);
        std::cout << "</return>" << std::endl;
    } else {
        std::cout << "/>" << std::endl;
    }
}

void Visitor::Visit(Empty& node)
{
    /* EMPTY */ // as the name implies
}
