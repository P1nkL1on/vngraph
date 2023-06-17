#ifndef HEADERS_H
#define HEADERS_H

#include <exception>

#include <QString>
#include <QVector>
#include <QMap>
#include <QSet>


namespace VisualNovelGraph {

using NodeId = int;
using SpeakerId = int;
using Text = QString;

struct Visitor;
struct Counters;


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
    virtual ~Predicate() = default;
    virtual Text title() const = 0;
    virtual Text text() const = 0;
    virtual bool isOk(const Counters &) const = 0;
};


struct Counter : Node
{
    virtual ~Counter() = default;
    virtual Text title() const = 0;
    virtual Text description() const = 0;
    virtual int initialValue() const = 0;
};


struct Advance : Node
{
    virtual ~Advance() = default;
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
    virtual bool shouldStop() const = 0;
};


struct Print : Visitor
{
    Print(const Node *start);
    void visit(const Frame &frame) override;
    void visit(const Predicate &predicate) override;
    void visit(const Counter &counter) override;
    void visit(const Advance &advance) override;
    inline bool shouldStop() const override { return shouldStop_; }
private:
    const Node *start_ = nullptr;
    bool shouldStop_ = false;
};


struct Graph
{
    const Node &node(const NodeId id) const;
    const QSet<NodeId> next(const NodeId id) const;
    QMap<NodeId, Node *> nodes_;
    QMap<NodeId, QSet<NodeId>> connections_;
};


void traverse(const Graph &graph, const NodeId id, Visitor &visitor);
void traverse_(const Graph &graph, const NodeId id, Visitor &visitor, QSet<NodeId> &traversed);


struct StaticFrame : Frame
{
    StaticFrame() = default;
    inline Text title() const override          { return title_; }
    inline Text text() const override           { return text_; }
    inline SpeakerId speakerId() const override { return speakerId_; }
    Text title_;
    Text text_;
    SpeakerId speakerId_ = -1;
};


class Error : std::runtime_error
{
public:
    Error(const QString &message);
    QString message;
};

}


namespace vn = VisualNovelGraph;

#endif // HEADERS_H