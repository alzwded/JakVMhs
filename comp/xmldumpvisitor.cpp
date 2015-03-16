#include "xmldumpvisitor.hpp"
#include <algorithm>
#include <iostream>

void XMLDumpVisitor::Visit(Program& node)
{
    Indent();
    output_ << "<program>" << std::endl;
    indentation++;
    std::for_each(node.Children().begin(), node.Children().end(), [&](std::shared_ptr<Node> const& n){
        n->Accept(this);
    });
    --indentation;
    Indent();
    output_ << "</program>" << std::endl;
}

void XMLDumpVisitor::Visit(If& node)
{
    Indent();
    output_ << "<if true='" << node.Truth() << "' false='" << node.Otherwise() << "'>" << std::endl;
    indentation++;
    node.Operand()->Accept(this);
    --indentation;
    Indent();
    output_ << "</if>" << std::endl;
}

void XMLDumpVisitor::Visit(For& node)
{
    Indent();
    output_ << "<for name='" << node.VarName() << "'>" << std::endl;
    indentation++;

    Indent();
    output_ << "<begin>" << std::endl;
    indentation++;
    node.Begin()->Accept(this);
    --indentation;
    Indent();
    output_ << "</begin>" << std::endl;

    Indent();
    output_ << "<end>" << std::endl;
    indentation++;
    node.End()->Accept(this);
    --indentation;
    Indent();
    output_ << "</end>" << std::endl;

    Indent();
    output_ << "<step>" << std::endl;
    indentation++;
    node.Step()->Accept(this);
    --indentation;
    Indent();
    output_ << "</step>" << std::endl;

    Indent();
    output_ << "<body>" << std::endl;
    indentation++;
    std::for_each(node.Children().begin(), node.Children().end(), [&](std::shared_ptr<Node> const& n){
        n->Accept(this);
    });
    --indentation;
    Indent();
    output_ << "</body>" << std::endl;

    --indentation;
    Indent();
    output_ << "</for>" << std::endl;
}

void XMLDumpVisitor::Visit(Loop& node)
{
    Indent();
    output_ << "<loop>" << std::endl;
    indentation++;
    if(node.Condition()) {
        Indent();
        output_ << "<condition>" << std::endl;
        indentation++;
        node.Condition()->Accept(this);
        --indentation;
        Indent();
        output_ << "</condition>" << std::endl;
    }
    Indent();
    output_ << "<body>" << std::endl;
    indentation++;
    std::for_each(node.Children().begin(), node.Children().end(), [&](std::shared_ptr<Node> const& n){
        n->Accept(this);
    });
    --indentation;
    Indent();
    output_ << "</body>" << std::endl;
    
    --indentation;
    Indent();
    output_ << "</loop>" << std::endl;
}

void XMLDumpVisitor::Visit(VarDecl& node)
{
    Indent();
    output_ << "<var name='" << node.Name();
    if(node.Initialization()) {
        output_ << "'>" << std::endl;
        indentation++;
        node.Initialization()->Accept(this);
        --indentation;
        Indent();
        output_ << "</var>" << std::endl;
    } else {
        output_ << "'/>" << std::endl;
    }
}

void XMLDumpVisitor::Visit(SharedDecl& node)
{
    Indent();
    output_ << "<shared name='" << node.Name();
    if(node.Initialization()) {
        output_ << "'>" << std::endl;
        indentation++;
        node.Initialization()->Accept(this);
        --indentation;
        Indent();
        output_ << "</shared>" << std::endl;
    } else {
        output_ << "'/>" << std::endl;
    }
}

void XMLDumpVisitor::Visit(Call& node)
{
    Indent();
    output_ << "<call";
    if(!node.From().empty()) output_ << " from='" << node.From() << "'";
    output_ << " name='" << node.Name() << "'>" << std::endl;

    indentation++;
    std::for_each(node.With().begin(), node.With().end(), [&](std::shared_ptr<Node> const& n){
        Indent();
        output_ << "<with>" << std::endl;
        indentation++;
        n->Accept(this);
        --indentation;
        Indent();
        output_ << "</with>" << std::endl;
    });
    --indentation;

    Indent();
    output_ << "</call>" << std::endl;
}

void XMLDumpVisitor::Visit(Sub& node)
{
    Indent();
    output_ << "<sub name='" << node.Name() << "'";
    if(!node.Params().empty()) {
        output_ << " with='";
        std::for_each(node.Params().begin(), node.Params().end(), [&](std::string const& s){
            output_ << s << ";";
        });
        output_ << "'";
    }
    output_ << ">" << std::endl;

    indentation++;
    std::for_each(node.Children().begin(), node.Children().end(), [&](std::shared_ptr<Node> const& n){
        n->Accept(this);
    });
    --indentation;

    Indent();
    output_ << "</sub>" << std::endl;
}

void XMLDumpVisitor::Visit(BinaryOp& node)
{
    Indent();
    output_ << "<binary op='" << node.Operation() << "'>" << std::endl;
    indentation++;
    node.Left()->Accept(this);
    node.Right()->Accept(this);
    --indentation;
    Indent();
    output_ << "</binary>" << std::endl;
}

void XMLDumpVisitor::Visit(UnaryOp& node)
{
    Indent();
    output_ << "<unary op='" << node.Operation() << "'>" << std::endl;
    indentation++;
    node.Operand()->Accept(this);
    --indentation;
    Indent();
    output_ << "</unary>" << std::endl;
}

void XMLDumpVisitor::Visit(Allocation& node)
{
    Indent();
    output_ << "<new>" << std::endl;
    indentation++;
    node.Operand()->Accept(this);
    --indentation;
    Indent();
    output_ << "</new>" << std::endl;
}

void XMLDumpVisitor::Visit(Deallocation& node)
{
    Indent();
    output_ << "<free>" << std::endl;
    indentation++;
    node.Operand()->Accept(this);
    --indentation;
    Indent();
    output_ << "</free>" << std::endl;
}

void XMLDumpVisitor::Visit(Assignation& node)
{
    Indent();
    output_ << "<assign>" << std::endl;
    indentation++;
    Indent();
    output_ << "<target>" << std::endl;
    indentation++;
    node.Name()->Accept(this);
    --indentation;
    Indent();
    output_ << "</target>" << std::endl;
    Indent();
    output_ << "<expression>" << std::endl;
    indentation++;
    node.Expression()->Accept(this);
    --indentation;
    Indent();
    output_ << "</expression>" << std::endl;
    --indentation;
    Indent();
    output_ << "</assign>" << std::endl;
}

void XMLDumpVisitor::Visit(Atom& node)
{
    Indent();
    output_ << "<atom name='" << node.Thing() << "'";
    if(node.Deref()) {
        output_ << ">" << std::endl;
        indentation++;
        node.Deref()->Accept(this);
        --indentation;
        Indent();
        output_ << "</atom>" << std::endl;
    } else {
        output_ << "/>" << std::endl;
    }
}

void XMLDumpVisitor::Visit(RefVar& node)
{
    Indent();
    output_ << "<ref>" << std::endl; 
    indentation++;
    node.Variable()->Accept(this);
    --indentation;
    Indent();
    output_ << "</ref>" << std::endl; 
}

void XMLDumpVisitor::Visit(Labelled& node)
{
    Indent();
    output_ << "<label name='" << node.Label() << "'>" << std::endl;
    indentation++;
    node.Operand()->Accept(this);
    --indentation;
    Indent();
    output_ << "</label>" << std::endl;
}

void XMLDumpVisitor::Visit(Next& node)
{
    Indent();
    output_ << "<next";
    if(node.Operand()) {
        output_ << ">" << std::endl;
        indentation++;
        node.Operand()->Accept(this);
        --indentation;
        Indent();
        output_ << "</next>" << std::endl;
    } else {
        output_ << "/>" << std::endl;
    }
}

void XMLDumpVisitor::Visit(Break& node)
{
    Indent();
    output_ << "<break";
    if(node.Operand()) {
        output_ << ">" << std::endl;
        indentation++;
        node.Operand()->Accept(this);
        --indentation;
        Indent();
        output_ << "</break>" << std::endl;
    } else {
        output_ << "/>" << std::endl;
    }
}

void XMLDumpVisitor::Visit(Return& node)
{
    Indent();
    output_ << "<return";
    if(node.Operand()) {
        output_ << ">" << std::endl;
        indentation++;
        node.Operand()->Accept(this);
        --indentation;
        Indent();
        output_ << "</return>" << std::endl;
    } else {
        output_ << "/>" << std::endl;
    }
}

void XMLDumpVisitor::Visit(Empty& node)
{
    /* EMPTY */ // as the name implies
}
