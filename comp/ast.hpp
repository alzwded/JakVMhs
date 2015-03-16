#ifndef AST_HPP
#define AST_HPP

#include <vector>
#include <memory>
#include <string>
#include <typeinfo>

class Visitor;

class Program;
class If;
class For;
class Loop;
class VarDecl;
class SharedDecl;
class Call;
class Sub;
class BinaryOp;
class UnaryOp;
class Allocation;
class Deallocation;
class Assignation;
class Atom;
class Labelled;
class RefVar;
class Break;
class Next;
class Return;
class Empty;

class Visitor
{
public:
    virtual ~Visitor() {}
    // implement Visit(concrete_class&);

    virtual void Visit(Program& node) =0;
    virtual void Visit(If& node) =0;
    virtual void Visit(For& node) =0;
    virtual void Visit(Loop& node) =0;
    virtual void Visit(VarDecl& node) =0;
    virtual void Visit(SharedDecl& node) =0;
    virtual void Visit(Call& node) =0;
    virtual void Visit(Sub& node) =0;
    virtual void Visit(BinaryOp& node) =0;
    virtual void Visit(UnaryOp& node) =0;
    virtual void Visit(Allocation& node) =0;
    virtual void Visit(Deallocation& node) =0;
    virtual void Visit(Assignation& node) =0;
    virtual void Visit(Atom& node) =0;
    virtual void Visit(RefVar& node) =0;
    virtual void Visit(Labelled& node) =0;
    virtual void Visit(Next& node) =0;
    virtual void Visit(Break& node) =0;
    virtual void Visit(Return& node) =0;
    virtual void Visit(Empty& node) =0;
};

#define CETTER(NAME, MEMBER) \
    decltype(MEMBER) const& NAME() { return MEMBER; }
#define ACCEPT() virtual void Accept(Visitor* const v) { v->Visit(*this); }

class Node
{
public:
    virtual ~Node() {}
    virtual void Accept(Visitor* const) =0;
};

class ContainerNode
: public Node
{
protected:
    std::vector<std::shared_ptr<Node> > childrens_;
public:
    void Add(std::shared_ptr<Node> const& n)
    {
        childrens_.push_back(n);
    }

    CETTER(Children, childrens_)
};

class Program
: public ContainerNode
{
public:
    ACCEPT()
};

class UnaryNode
: public Node
{
protected:
    std::shared_ptr<Node> operand_;
    UnaryNode() = delete;
public:
    typedef decltype(operand_) operand_t;
    UnaryNode(decltype(operand_) const& operand)
        : operand_(operand)
    {}
    CETTER(Operand, operand_)
};

class If
: public UnaryNode
{
    std::string tl_, fl_;
public:
    If(operand_t const& operand, std::string const& tl, std::string const& fl)
        : UnaryNode(operand)
        , tl_(tl)
        , fl_(fl)
    {}

    CETTER(Truth, tl_)
    CETTER(Otherwise, fl_)

    ACCEPT()
};

class For
: public ContainerNode
{
    std::string var_;
    std::shared_ptr<Node> begin_;
    std::shared_ptr<Node> end_;
    std::shared_ptr<Node> step_;
public:
    For(decltype(var_) const& varr, decltype(begin_) const& beginn, decltype(end_) const& endd, decltype(step_) const& stepp)
        : var_(varr)
        , begin_(beginn)
        , end_(endd)
        , step_(stepp)
    {}

    CETTER(VarName, var_)
    CETTER(Begin, begin_)
    CETTER(End, end_)
    CETTER(Step, step_)
    ACCEPT()
};

class Loop
: public ContainerNode
{
    std::shared_ptr<Node> condition_;
public:
    Loop(decltype(condition_) const& condition)
        : condition_(condition)
    {}

    CETTER(Condition, condition_)
    ACCEPT()
};

class VarDecl
: public Node
{
    std::string name_;
    std::shared_ptr<Node> init_;
public:
    VarDecl(decltype(name_) const& name, decltype(init_) const& init)
        : name_(name)
        , init_(init)
    {}

    CETTER(Name, name_)
    CETTER(Initialization, init_)
    ACCEPT()
};

class SharedDecl
: public Node
{
    std::string name_;
    std::shared_ptr<Node> init_;
public:
    SharedDecl(decltype(name_) const& name, decltype(init_) const& init)
        : name_(name)
        , init_(init)
    {}

    CETTER(Name, name_)
    CETTER(Initialization, init_)
    ACCEPT()
};

class Call
: public Node
{
    typedef std::vector<std::shared_ptr<Node> > listOfNodes;
    std::string from_;
    std::string name_;
    listOfNodes with_;
public:
    Call(decltype(name_) const& name, decltype(from_) const& from)
        : name_(name)
        , from_(from)
    {}

    void AddParam(std::shared_ptr<Node> const& l)
    {
        with_.push_back(l);
    }

    CETTER(From, from_)
    CETTER(Name, name_)
    CETTER(With, with_)
    ACCEPT()
};

class Sub
: public ContainerNode
{
    std::string name_;
    std::vector<std::string> params_;
public:
    Sub(decltype(name_) const& name)
        : name_(name)
        , params_()
    {}

    void AddParam(std::string const& param)
    {
        params_.push_back(param);
    }

    CETTER(Name, name_)
    CETTER(Params, params_)
    ACCEPT()
};

class BinaryOp
: public Node
{
    std::string  op_;
    std::shared_ptr<Node> left_;
    std::shared_ptr<Node> right_;
public:
    BinaryOp(decltype(op_) const& op, decltype(left_) const& left, decltype(right_) right)
        : op_(op)
        , left_(left)
        , right_(right)
    {}

    CETTER(Operation, op_)
    CETTER(Left, left_)
    CETTER(Right, right_)
    ACCEPT()
};

class UnaryOp
: public Node
{
    std::string op_;
    std::shared_ptr<Node> operand_;
public:
    UnaryOp(decltype(op_) const& op, decltype(operand_) const& operand)
        : op_(op)
        , operand_(operand)
    {}

    CETTER(Operation, op_)
    CETTER(Operand, operand_)
    ACCEPT()
};

class Atom
: public Node
{
    std::string thing_;
    std::shared_ptr<Node> deref_;
public:
    Atom(decltype(thing_) const& thing)
        : thing_(thing)
        , deref_(NULL)
    {}
    Atom(decltype(thing_) const& base, decltype(deref_) const& deref)
        : thing_(base)
        , deref_(deref)
    {}

    CETTER(Thing, thing_)
    CETTER(Deref, deref_)
    ACCEPT()
};

class Assignation
: public Node
{
    std::shared_ptr<Node> name_;
    std::shared_ptr<Node> expression_;
public:
    Assignation(decltype(name_) const& name, decltype(expression_) const & expression)
        : name_(name)
        , expression_(expression)
    {}

    CETTER(Name, name_)
    CETTER(Expression, expression_)
    ACCEPT()
};

class RefVar
: public Node
{
    std::shared_ptr<Node> var_;
public:
    RefVar(decltype(var_) const& var)
        : var_(var)
    {}

    CETTER(Variable, var_)
    ACCEPT()
};

class Labelled
: public UnaryNode
{
    std::string label_;
public:
    Labelled(decltype(label_) const& label, operand_t const& operand)
        : UnaryNode(operand)
        , label_(label)
    {}

    CETTER(Label, label_);
    ACCEPT()
};

class Next
: public UnaryNode
{
public:
    Next(operand_t const& operand)
        : UnaryNode(operand)
    {}

    ACCEPT()
};

class Break
: public UnaryNode
{
public:
    Break(operand_t const& operand)
        : UnaryNode(operand)
    {}

    ACCEPT()
};

class Return
: public UnaryNode
{
public:
    Return(operand_t const& operand)
        : UnaryNode(operand)
    {}

    ACCEPT()
};

class Allocation
: public UnaryNode
{
public:
    Allocation(operand_t const& operand)
        : UnaryNode(operand)
    {}
    ACCEPT();
};

class Deallocation
: public UnaryNode
{
    Deallocation() = delete;
public:
    Deallocation(operand_t const& operand)
        : UnaryNode(operand)
    {}
    ACCEPT();
};

class Empty
: public Node
{
public:
    ACCEPT()
};

#endif
