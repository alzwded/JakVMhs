#include "xmldumpvisitor.hpp"
#include <algorithm>
#include <iostream>

void XMLDumpVisitor::Visit(Program& node)
{
    Indent();
    std::cout << "<program>" << std::endl;
    indentation++;
    std::for_each(node.Children().begin(), node.Children().end(), [&](std::shared_ptr<Node> const& n){
        n->Accept(this);
    });
    --indentation;
    Indent();
    std::cout << "</program>" << std::endl;
}

void XMLDumpVisitor::Visit(If& node)
{
    Indent();
    std::cout << "<if true='" << node.Truth() << "' false='" << node.Otherwise() << "'>" << std::endl;
    indentation++;
    node.Operand()->Accept(this);
    --indentation;
    Indent();
    std::cout << "</if>" << std::endl;
}

void XMLDumpVisitor::Visit(For& node)
{
    Indent();
    std::cout << "<for name='" << node.VarName() << "'>" << std::endl;
    indentation++;

    Indent();
    std::cout << "<begin>" << std::endl;
    indentation++;
    node.Begin()->Accept(this);
    --indentation;
    Indent();
    std::cout << "</begin>" << std::endl;

    Indent();
    std::cout << "<end>" << std::endl;
    indentation++;
    node.End()->Accept(this);
    --indentation;
    Indent();
    std::cout << "</end>" << std::endl;

    Indent();
    std::cout << "<step>" << std::endl;
    indentation++;
    node.Step()->Accept(this);
    --indentation;
    Indent();
    std::cout << "</step>" << std::endl;

    Indent();
    std::cout << "<body>" << std::endl;
    indentation++;
    std::for_each(node.Children().begin(), node.Children().end(), [&](std::shared_ptr<Node> const& n){
        n->Accept(this);
    });
    --indentation;
    Indent();
    std::cout << "</body>" << std::endl;

    --indentation;
    Indent();
    std::cout << "</for>" << std::endl;
}

void XMLDumpVisitor::Visit(Loop& node)
{
    Indent();
    std::cout << "<loop>" << std::endl;
    indentation++;
    if(node.Condition()) {
        Indent();
        std::cout << "<condition>" << std::endl;
        indentation++;
        node.Condition()->Accept(this);
        --indentation;
        Indent();
        std::cout << "</condition>" << std::endl;
    }
    Indent();
    std::cout << "<body>" << std::endl;
    indentation++;
    std::for_each(node.Children().begin(), node.Children().end(), [&](std::shared_ptr<Node> const& n){
        n->Accept(this);
    });
    --indentation;
    Indent();
    std::cout << "</body>" << std::endl;
    
    --indentation;
    Indent();
    std::cout << "</loop>" << std::endl;
}

void XMLDumpVisitor::Visit(VarDecl& node)
{
    Indent();
    std::cout << "<var name='" << node.Name();
    if(node.Initialization()) {
        std::cout << "'>" << std::endl;
        indentation++;
        node.Initialization()->Accept(this);
        --indentation;
        Indent();
        std::cout << "</var>" << std::endl;
    } else {
        std::cout << "'/>" << std::endl;
    }
}

void XMLDumpVisitor::Visit(SharedDecl& node)
{
    Indent();
    std::cout << "<shared name='" << node.Name();
    if(node.Initialization()) {
        std::cout << "'>" << std::endl;
        indentation++;
        node.Initialization()->Accept(this);
        --indentation;
        Indent();
        std::cout << "</shared>" << std::endl;
    } else {
        std::cout << "'/>" << std::endl;
    }
}

void XMLDumpVisitor::Visit(Call& node)
{
    Indent();
    std::cout << "<call";
    if(!node.From().empty()) std::cout << " from='" << node.From() << "'";
    std::cout << " name='" << node.Name() << "'>" << std::endl;

    indentation++;
    std::for_each(node.With().begin(), node.With().end(), [&](std::shared_ptr<Node> const& n){
        Indent();
        std::cout << "<with>" << std::endl;
        indentation++;
        n->Accept(this);
        --indentation;
        Indent();
        std::cout << "</with>" << std::endl;
    });
    --indentation;

    Indent();
    std::cout << "</call>" << std::endl;
}

void XMLDumpVisitor::Visit(Sub& node)
{
    Indent();
    std::cout << "<sub name='" << node.Name() << "'";
    if(!node.Params().empty()) {
        std::cout << " with='";
        std::for_each(node.Params().begin(), node.Params().end(), [&](std::string const& s){
            std::cout << s << ";";
        });
        std::cout << "'";
    }
    std::cout << ">" << std::endl;

    indentation++;
    std::for_each(node.Children().begin(), node.Children().end(), [&](std::shared_ptr<Node> const& n){
        n->Accept(this);
    });
    --indentation;

    Indent();
    std::cout << "</sub>" << std::endl;
}

void XMLDumpVisitor::Visit(BinaryOp& node)
{
    Indent();
    std::cout << "<" << node.Operation() << ">" << std::endl;
    indentation++;
    node.Left()->Accept(this);
    node.Right()->Accept(this);
    --indentation;
    Indent();
    std::cout << "</" << node.Operation() << ">" << std::endl;
}

void XMLDumpVisitor::Visit(UnaryOp& node)
{
    Indent();
    std::cout << "<" << node.Operation() << ">" << std::endl;
    indentation++;
    node.Operand()->Accept(this);
    --indentation;
    Indent();
    std::cout << "</" << node.Operation() << ">" << std::endl;
}

void XMLDumpVisitor::Visit(Assignation& node)
{
    Indent();
    std::cout << "<assign>" << std::endl;
    indentation++;
    Indent();
    std::cout << "<target>" << std::endl;
    indentation++;
    node.Name()->Accept(this);
    --indentation;
    Indent();
    std::cout << "</target>" << std::endl;
    Indent();
    std::cout << "<expression>" << std::endl;
    indentation++;
    node.Expression()->Accept(this);
    --indentation;
    Indent();
    std::cout << "</expression>" << std::endl;
    --indentation;
    Indent();
    std::cout << "</assign>" << std::endl;
}

void XMLDumpVisitor::Visit(Atom& node)
{
    Indent();
    std::cout << "<atom name='" << node.Thing() << "'";
    if(node.Deref()) {
        std::cout << ">" << std::endl;
        indentation++;
        node.Deref()->Accept(this);
        --indentation;
        Indent();
        std::cout << "</atom>" << std::endl;
    } else {
        std::cout << "/>" << std::endl;
    }
}

void XMLDumpVisitor::Visit(RefVar& node)
{
    Indent();
    std::cout << "<ref>" << std::endl; 
    indentation++;
    node.Variable()->Accept(this);
    --indentation;
    Indent();
    std::cout << "</ref>" << std::endl; 
}

void XMLDumpVisitor::Visit(Labelled& node)
{
    Indent();
    std::cout << "<label name='" << node.Label() << "'>" << std::endl;
    indentation++;
    node.Operand()->Accept(this);
    --indentation;
    Indent();
    std::cout << "</label>" << std::endl;
}

void XMLDumpVisitor::Visit(Return& node)
{
    Indent();
    std::cout << "<return";
    if(node.Operand()) {
        std::cout << ">" << std::endl;
        indentation++;
        node.Operand()->Accept(this);
        --indentation;
        Indent();
        std::cout << "</return>" << std::endl;
    } else {
        std::cout << "/>" << std::endl;
    }
}

void XMLDumpVisitor::Visit(Empty& node)
{
    /* EMPTY */ // as the name implies
}
