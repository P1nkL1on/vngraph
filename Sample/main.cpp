#include <QtDebug>
#include <QList>

#include <iostream>
#include "headers.h"


vn::Error::Error(const QString &message) :
    std::runtime_error(message.toStdString()),
    message(message)
{}

vn::NodeId vn::Graph::add(Node *node)
{
    const NodeId nextKey = nodes_.isEmpty() ? 0 : (nodes_.lastKey() + 1);
    nodes_.insert(nextKey, node);
    return nextKey;
}

void vn::Graph::connect(const NodeId src, const NodeId dst)
{
    if (!nodes_.contains(src))
        throw Error(QString("Missing node with id %1 as connection source").arg(src));
    if (!nodes_.contains(dst))
        throw Error(QString("Missing node with id %1 as connection destination").arg(dst));
    connections_[src].insert(dst);
}

const vn::Node &vn::Graph::node(const vn::NodeId id) const
{
    const Node *node = nodes_.value(id, nullptr);
    if (node)
        return *node;

    throw Error(QString("Missing node with id %1").arg(id));
}

const QSet<vn::NodeId> vn::Graph::next(const vn::NodeId id) const
{
    if (!connections_.contains(id))
        throw Error(QString("Missing connections for node with id %1").arg(id));

    return connections_.value(id, {});
}

void vn::traverse(
        const Graph &graph,
        const NodeId id,
        Visitor &visitor)
{
    QSet<NodeId> traversed;
    return traverse_(graph, id, visitor, traversed);
}

void vn::traverse_(
        const Graph &graph,
        const NodeId id,
        Visitor &visitor,
        QSet<NodeId> &traversed)
{
    const Node &node = graph.node(id);
    traversed.insert(id);
    node.accept(visitor);

    if (visitor.shouldStop())
        return;

    QSet<vn::NodeId> next = graph.next(id);
    next -= traversed;

    QList<vn::NodeId> nextSorted = next.toList();
    std::sort(nextSorted.begin(), nextSorted.end());

    for (const NodeId id : nextSorted)
        traverse_(graph, id, visitor, traversed);
}

void vn::Frame::accept(Visitor &visitor) const
{
    return visitor.visit(*this);
}

void vn::Predicate::accept(Visitor &visitor) const
{
    return visitor.visit(*this);
}

vn::Print::Print(const vn::Node *start) :
    start_(start)
{}

void vn::Print::visit(const Frame &frame)
{
    shouldStop_ = &frame != start_;
    qDebug()
            << "frame"
            << &frame
            << frame.title()
            << frame.text()
            << frame.speakerId();
}

void vn::Print::visit(const Predicate &predicate)
{
    Q_UNUSED(predicate)
    shouldStop_ = false;
    qDebug()
            << "predicate"
            << &predicate
            << predicate.title()
            << predicate.text()
            << "satisfied: "
            << predicate.isOk();
}

void vn::Print::visit(const Counter &counter)
{
    Q_UNUSED(counter)
    shouldStop_ = false;
    qDebug() << "counter";
}

void vn::Print::visit(const Advance &advance)
{
    Q_UNUSED(advance)
    shouldStop_ = false;
    qDebug() << "advance";
}

void vn::Print::visit(const Op &advance)
{
    Q_UNUSED(advance)
    shouldStop_ = false;
    qDebug() << "op";
}

//bool vn::PredicateCompare::isOk(const Counters &counters) const
//{
//    const int counter = counters.value(nodeId_);
//    switch (compareOption_) {
//    case Greater:        return counter >  valueToCompare_;
//    case Less:           return counter <  valueToCompare_;
//    case GreaterOrEqual: return counter >= valueToCompare_;
//    case LessOrEqual:    return counter <= valueToCompare_;
//    case Equal:          return counter == valueToCompare_;
//    case NonEqual:       return counter != valueToCompare_;
//    }
//    throw new Error(QString("Unknown predicate compare option: %1").arg(compareOption_));
//}

int main(int argc, char *argv[])
{
    Q_UNUSED(argc)
    Q_UNUSED(argv)

    /// NOTE: the wished dialog to make
    ///
    /// NARRATOR: where to go?
    /// 1) gold mines
    /// 2) shop
    /// 3) adventure [sword needed] -> happy end
    ///
    /// gold mines: several steps, resulted in +10 gp
    /// then come back to the beginning
    ///
    /// shop:
    /// KEEPER: want to buy something?
    /// 1) sword [20gp]
    /// 2) apple [free]
    /// 3) go away [back to start]
    ///
    /// TODO:
    /// frame -> [[predicate] -> frame]*

    vn::Op::Value res;
    vn::Text err;
    vn::Op op;
    std::vector<vn::Op::Value> values {
        vn::Op::Value(42),
                vn::Op::Value(true),
        vn::Op::Value(6900),
        vn::Op::Value(vn::Op::Void()),
        vn::Op::Value(false),
    };
    std::cout
            << op.compute(values.data(), res, err)  << "\n"
            << err.toStdString() << "\n"
            << res << "\n";

    auto *nodeFrameStart     = new vn::FrameStatic();
    auto *nodeFrameChoice    = new vn::FrameStatic();
    auto *nodeFrameMines     = new vn::FrameStatic();
    auto *nodeFrameMinesWork = new vn::FrameStatic();
    auto *nodeFrameShop      = new vn::FrameStatic();
    auto *nodeFrameBuySword  = new vn::FrameStatic();
    auto *nodeFrameBuyApple  = new vn::FrameStatic();
    auto *nodeFrameBuyGoBack = new vn::FrameStatic();
    auto *nodeFrameAdventure = new vn::FrameStatic();
    auto *nodePredicateHas20gp = new vn::PredicateStatic();
    nodeFrameStart->title_ = "";
    nodeFrameStart->text_ = "Welcome!";
    nodeFrameChoice->title_ = "";
    nodeFrameChoice->text_ = "Where would you go?";
    nodeFrameMines->title_ = "Golden mines";
    nodeFrameMines->text_ = "You are approaching the golden mines";
    nodeFrameMinesWork->title_ = "";
    nodeFrameMinesWork->text_ = "You are working hard in mines and doing your job";
    nodeFrameShop->title_ = "Shop";
    nodeFrameShop->text_ = "You are moving your bones to the town merchant";
    nodeFrameBuySword->title_ = "Buy a sword";
    nodeFrameBuySword->text_ = "A good sword of old master found a shelter in yout untrained arms";
    nodeFrameBuyApple->title_ = "Buy an apple";
    nodeFrameBuyApple->text_ = "Your hunger is no more";
    nodeFrameBuyGoBack->title_ = "Turn around";
    nodeFrameBuyGoBack->text_ = "You came back where you started";
    nodeFrameAdventure->title_ = "Adventure";
    nodeFrameAdventure->text_ = "You are going to the adventure! Finally...";
    nodePredicateHas20gp->title_ = "Has 20 gp";
    nodePredicateHas20gp->text_ = "You probably should work on mines for a while...";

    vn::Graph graph;
    const vn::NodeId idStart = graph.add(nodeFrameStart);
    const vn::NodeId idChoice = graph.add(nodeFrameChoice);
    const vn::NodeId idMines = graph.add(nodeFrameMines);
    const vn::NodeId idWork = graph.add(nodeFrameMinesWork);
    const vn::NodeId idShop = graph.add(nodeFrameShop);
    const vn::NodeId idSword = graph.add(nodeFrameBuySword);
    const vn::NodeId idApple = graph.add(nodeFrameBuyApple);
    const vn::NodeId idBack = graph.add(nodeFrameBuyGoBack);
    const vn::NodeId idEnd = graph.add(nodeFrameAdventure);
    const vn::NodeId idCheck20gp = graph.add(nodePredicateHas20gp);
    graph.connect(idStart, idChoice);
    graph.connect(idChoice, idMines);
    graph.connect(idChoice, idShop);
    graph.connect(idChoice, idEnd);
    graph.connect(idMines, idWork);
    graph.connect(idWork, idChoice);
    graph.connect(idShop, idCheck20gp);
    graph.connect(idCheck20gp, idSword);
    graph.connect(idSword, idShop);
    graph.connect(idShop, idApple);
    graph.connect(idApple, idShop);
    graph.connect(idShop, idBack);
    graph.connect(idBack, idChoice);

    try {
        vn::Print print = vn::Print(nodeFrameChoice);
        vn::traverse(graph, 1, print);
        qDebug() << "";

        print = vn::Print(nodeFrameShop);
        vn::traverse(graph, 4, print);

    } catch (const vn::Error &err) {
        qDebug() << err.message;
        return 1;
    }
    return 0;
}
