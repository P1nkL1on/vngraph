#ifndef HEADERS_H
#define HEADERS_H

#include <exception>
#include <sstream>
#include <iostream>

#include <QJsonObject>
#include <QString>
#include <QVector>
#include <QtDebug>
#include <QMap>
#include <QSet>

#include "utils.h"


namespace VisualNovelGraph {

using NodeId = int;
using SpeakerId = int;
using Text = QString;

struct Visitor;
struct Compute;
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
    void accept(Visitor &visitor) const override;

    using Value = std::variant<Text, bool, int>;
    constexpr const static std::array<const char *, std::variant_size_v<Value>> names {
        "error", "bool", "int",
    };
};


struct Static42 : Op
{
    void accept(Visitor &visitor) const override;
};


struct Static69 : Op
{
    void accept(Visitor &visitor) const override;
};


struct Equal : Op
{
    void accept(Visitor &visitor) const override;
};


struct NonEqual : Op
{
    void accept(Visitor &visitor) const override;
};


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
    inline virtual void visit(const Node &)          {}
    inline virtual void visit(const Frame &node)     { return visit(static_cast<const Node &>(node)); }
    inline virtual void visit(const Predicate &node) { return visit(static_cast<const Node &>(node)); }
    inline virtual void visit(const Counter &node)   { return visit(static_cast<const Node &>(node)); }
    inline virtual void visit(const Advance &node)   { return visit(static_cast<const Node &>(node)); }
    inline virtual void visit(const Op &node)        { return visit(static_cast<const Node &>(node)); }
    inline virtual void visit(const Equal &node)     { return visit(static_cast<const Op &>(node)); }
    inline virtual void visit(const NonEqual &node)  { return visit(static_cast<const Op &>(node)); }
    inline virtual void visit(const Static42 &node)  { return visit(static_cast<const Op &>(node)); }
    inline virtual void visit(const Static69 &node)  { return visit(static_cast<const Op &>(node)); }
    inline virtual bool shouldStop() const           { return false; }
    inline virtual void stepIn(const Node &)         {}
    inline virtual void stepOut()                    {}
};


struct Q_PACKED Print : Visitor
{
    Print(const Node *start);
    void visit(const Frame &frame) override;
    void visit(const Predicate &predicate) override;
    inline bool shouldStop() const override { return shouldStop_; }
    inline void stepIn(const Node &) override { depth_++; }
    inline void stepOut() override { depth_--; }
private:
    static QString space(const int depth);
    const Node *start_ = nullptr;
    bool shouldStop_ = false;
    int depth_ = 0;
};


struct ToGraphViz : Visitor
{
    ToGraphViz() = default;
    void visit(const Frame &frame) override;
    void stepIn(const Node &node) override;
    void stepOut() override;
    QString digraphText() const;
private:
    QMap<const Node *, NodeId> ptrToId_;
    QVector<const Node *> stack_;
    const Node *parent_ = nullptr;
    std::stringstream ss_;
};


struct Compute : Visitor
{
    Compute() = default;
    void visit(const Equal &op) override;
    void visit(const NonEqual &op) override;
    void visit(const Static42 &op) override;
    void visit(const Static69 &op) override;
    void stepIn(const Node &node) override;
    void stepOut() override;
    bool shouldStop() const override;
private:
    using Value = Op::Value;
    using LazyValue = std::function<Value()>;
    using MaybeLazyValue = std::variant<LazyValue, Value>;

    QVector<const Node *> stack_;
    QMap<const Node *, MaybeLazyValue> opToFoo_;
    QVector<Value> values_;

    template <typename T>
    static bool read(const Op::Value &value, T &res)
    {
        const T *variantPtr = std::get_if<T>(&value);
        if (variantPtr == nullptr) {
//            const std::size_t nameExpectedInd = variant_index<Op::Value, T>();
//            const std::size_t nameRecievedInd = value.index();
//            res = Text("Expected %1 but recieved %2")
//                    .arg(Op::names.at(nameExpectedInd))
//                    .arg(Op::names.at(nameRecievedInd));
            return false;
        }
        res = *variantPtr;
        return true;
    }
};


struct Graph
{
    // TODO: personally don't like
    NodeId add(Node *node);
    // TODO: personally don't like
    void connect(const NodeId src, const NodeId dst);
    void disconnect(const NodeId src, const NodeId dst);

    const Node &node(const NodeId id) const;
    const QVector<NodeId> next(const NodeId id) const;
    QMap<NodeId, Node *> nodes_;
    QMap<NodeId, QVector<NodeId>> connections_;
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
    inline Text title() const override { return title_; }
    inline Text text() const override  { return text_; }
    inline bool isOk() const override  { return true; }
    Text title_;
    Text text_;
};


struct Counters
{
    // TODO: change to match a real one
    int value(const NodeId) const { return 42; }
};

//template <typename T>
//bool Op::read(const Value &value, T &res, Text &err)
//{
//    const T *variantPtr = std::get_if<T>(&value);
//    if (variantPtr == nullptr) {
//        const std::size_t nameExpectedInd = variant_index<Value, T>();
//        const std::size_t nameRecievedInd = value.index();
//        err = QString("Expected %1 but recieved %2")
//                .arg(names.at(nameExpectedInd))
//                .arg(names.at(nameRecievedInd));
//        return false;
//    }

//    res = *variantPtr;
//    err.clear();
//    return true;
//}


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


std::ostream &operator<<(std::ostream &stream, const vn::Op::Value &value);

#endif // HEADERS_H
