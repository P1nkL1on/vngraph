#include <QtDebug>
#include <QList>

#include "headers.h"


vn::Error::Error(const QString &message) :
    std::runtime_error(message.toStdString()),
    message(message)
{}

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
    qDebug() << "predicate";
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

    auto *nodeFrameStart     = new vn::StaticFrame();
    auto *nodeFrameChoice    = new vn::StaticFrame();
    auto *nodeFrameMines     = new vn::StaticFrame();
    auto *nodeFrameMinesWork = new vn::StaticFrame();
    auto *nodeFrameShop      = new vn::StaticFrame();
    auto *nodeFrameBuySword  = new vn::StaticFrame();
    auto *nodeFrameBuyApple  = new vn::StaticFrame();
    auto *nodeFrameBuyGoBack = new vn::StaticFrame();
    auto *nodeFrameAdventure = new vn::StaticFrame();
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
    nodeFrameBuyApple->title_ = "Your hunger is no more";
    nodeFrameBuyGoBack->title_ = "Turn around";
    nodeFrameBuyGoBack->text_ = "You came back where you started";
    nodeFrameAdventure->title_ = "Adventure";
    nodeFrameAdventure->text_ = "You are going to the adventure! Finally...";

    vn::Graph graph;
    graph.nodes_.insert(0, nodeFrameStart);
    graph.nodes_.insert(1, nodeFrameChoice);
    graph.nodes_.insert(2, nodeFrameMines);
    graph.nodes_.insert(3, nodeFrameMinesWork);
    graph.nodes_.insert(4, nodeFrameShop);
    graph.nodes_.insert(5, nodeFrameBuySword);
    graph.nodes_.insert(6, nodeFrameBuyApple);
    graph.nodes_.insert(7, nodeFrameBuyGoBack);
    graph.nodes_.insert(8, nodeFrameAdventure);
    graph.connections_[0].insert(1);
    graph.connections_[1].insert(2);
    graph.connections_[1].insert(4);
    graph.connections_[1].insert(8);
    graph.connections_[2].insert(3);
    graph.connections_[3].insert(1);
    graph.connections_[4].insert(5);
    graph.connections_[5].insert(4);
    graph.connections_[4].insert(6);
    graph.connections_[6].insert(4);
    graph.connections_[4].insert(7);
    graph.connections_[7].insert(1);

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
