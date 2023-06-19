#ifndef HEADERS_H
#define HEADERS_H

#include <exception>

#include <QString>
#include <QMap>
#include <QSet>

#include "utils.h"


namespace VisualNovelGraph {

using NodeId = int;
using SpeakerId = int;
using Text = QString;

struct Visitor;
struct Counters;
struct FrameStatic;


struct Node
{
    virtual ~Node() = default;
    virtual void accept(Visitor &) const = 0;
};


struct Frame : Node
{
    virtual Text title() const = 0;
    virtual Text text() const = 0;
    virtual SpeakerId speakerId() const = 0;
    void accept(Visitor &visitor) const override;
};


struct Predicate : Node
{
    virtual Text title() const = 0;
    virtual Text text() const = 0;
    virtual bool isOk() const = 0;
    void accept(Visitor &visitor) const override;
};


class Error : std::runtime_error
{
public:
    Error(const QString &message);
    QString message;
};



struct Op : Node
{
    struct Void {};
    using Value = std::variant<Void, bool, int>;
    constexpr const static std::array<const char *, std::variant_size_v<Value>> names {
        "void", "bool", "int",
    };
    virtual bool compute(Value *value, Value &res, Text &err)
    {
        int a = 0;
        int b = 0;
        if (!read(value[0], a, err)
                || !read(value[1], b, err))
            return false;

        res = a + b;
        return true;
    }
    void accept(Visitor &) const override
    {
//        return visitor.visit(*this);
    }
protected:
    template <typename T>
    bool read(Value &value, T &res, Text &err)
    {
        T *variantPtr = std::get_if<T>(&value);
        if (variantPtr == nullptr) {
            const std::size_t nameExpectedInd = variant_index<Value, T>();
            const std::size_t nameRecievedInd = value.index();
            err = QString("Expected %1 but recieved %2")
                    .arg(names.at(nameExpectedInd))
                    .arg(names.at(nameRecievedInd));
            return false;
        }

        res = *variantPtr;
        err.clear();
        return true;
    }
};


std::ostream& operator<<(std::ostream &stream, const Op::Value &value)
{
    std::visit([&stream](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, int>)
            stream << "int " << arg;
        else if constexpr (std::is_same_v<T, bool>)
            stream << (arg ? "True" : "False");
        else if constexpr (std::is_same_v<T, Op::Void>)
            stream << "None";
    }, value);
    return stream;
}


struct Counter : Node
{
    virtual Text title() const = 0;
    virtual Text description() const = 0;
    virtual int initialValue() const = 0;
};


struct Advance : Node
{
    virtual void redo(Counters &) const = 0;
    virtual void undo(Counters &) const = 0;
};


struct Visitor
{
    virtual ~Visitor() = default;
    virtual void visit(const Frame &) = 0;
    virtual void visit(const Predicate &) = 0;
    virtual void visit(const Counter &) = 0;
    virtual void visit(const Advance &) = 0;
    virtual void visit(const Op &) = 0;
    virtual bool shouldStop() const = 0;
};


struct Print : Visitor
{
    Print(const Node *start);
    void visit(const Frame &frame) override;
    void visit(const Predicate &predicate) override;
    void visit(const Counter &counter) override;
    void visit(const Advance &advance) override;
    void visit(const Op &advance) override;
    inline bool shouldStop() const override { return shouldStop_; }
private:
    const Node *start_ = nullptr;
    bool shouldStop_ = false;
};


struct Graph
{
    // TODO: personally don't like
    NodeId add(Node *node);
    // TODO: personally don't like
    void connect(const NodeId src, const NodeId dst);
    void disconnect(const NodeId src, const NodeId dst);


    const Node &node(const NodeId id) const;
    const QSet<NodeId> next(const NodeId id) const;
    QMap<NodeId, Node *> nodes_;
    QMap<NodeId, QSet<NodeId>> connections_;
};


void traverse(const Graph &graph, const NodeId id, Visitor &visitor);
void traverse_(const Graph &graph, const NodeId id, Visitor &visitor, QSet<NodeId> &traversed);


struct FrameStatic : Frame
{
    FrameStatic() = default;
    inline Text title() const override          { return title_; }
    inline Text text() const override           { return text_; }
    inline SpeakerId speakerId() const override { return -1; }
    Text title_;
    Text text_;
};


struct PredicateStatic : Predicate
{
    PredicateStatic() = default;
    inline Text title() const override  { return title_; }
    inline Text text() const override   { return text_; }
    inline bool isOk() const override   { return true; }
    Text title_;
    Text text_;
};


struct Counters
{
    // TODO: change to match a real one
    int value(const NodeId) const { return 42; }
};


//struct PredicateCompare : Predicate
//{
//    PredicateCompare() = default;
//    inline Text title() const override { return title_; }
//    inline Text text() const override  { return text_; }
//    bool isOk(const Counters &) const override;
//    Text title_;
//    Text text_;
//    NodeId nodeId_ = -1;
//    int valueToCompare_ = 0;
//    enum {
//        Greater,
//        Less,
//        GreaterOrEqual,
//        LessOrEqual,
//        Equal,
//        NonEqual,
//    } compareOption_ = Greater;
//};

}


namespace vn = VisualNovelGraph;

#endif // HEADERS_H
