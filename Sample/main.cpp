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
    connections_[src].append(dst);
}

const vn::Node &vn::Graph::node(const vn::NodeId id) const
{
    const Node *node = nodes_.value(id, nullptr);
    if (node)
        return *node;

    throw Error(QString("Missing node with id %1").arg(id));
}

const QVector<vn::NodeId> vn::Graph::next(const vn::NodeId id) const
{
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
    visitor.stepIn(node);
    if (traversed.contains(id)) {
        visitor.stepOut();
        return;
    }

    traversed.insert(id);
    node.accept(visitor);

    if (visitor.shouldStop()) {
        visitor.stepOut();
        return;
    }

    const QVector<vn::NodeId> next = graph.next(id);
    if (next.isEmpty()) {
        visitor.stepOut();
        return;
    }

    for (const NodeId id : next)
        traverse_(graph, id, visitor, traversed);
    visitor.stepOut();
}

void vn::Frame::accept(Visitor &visitor) const
{
    return visitor.visit(*this);
}

void vn::Predicate::accept(Visitor &visitor) const
{
    return visitor.visit(*this);
}

void vn::Op::accept(Visitor &visitor) const
{
    return visitor.visit(*this);
}

vn::Print::Print(const vn::Node *start) :
    start_(start)
{}

void vn::Print::visit(const Frame &frame)
{
    shouldStop_ = &frame != start_;
    qDebug().noquote()
            << space(depth_)
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
    qDebug().noquote()
            << space(depth_)
            << "predicate"
            << &predicate
            << predicate.title()
            << predicate.text()
            << "satisfied: "
            << predicate.isOk();
}

QString vn::Print::space(const int depth)
{
    return QString("*").rightJustified(depth * 2);
}

void vn::ToGraphViz::visit(const Frame &frame)
{
    ss_ << ptrToId_.value(&frame, -1)
        << " [label=\""
        << frame.title().toStdString()
        << ":\\n"
        << frame.text().toStdString()
        << "\"]\n";
}

void vn::ToGraphViz::stepIn(const Node &node)
{
    const Node *ptr = &node;
    ptrToId_.insert(ptr, ptrToId_.value(ptr, ptrToId_.size()));
    stack_.push_back(ptr);

    const NodeId idSrc = ptrToId_.value(parent_, -1);
    const NodeId idDst = ptrToId_.value(ptr);
    parent_ = ptr;

    if (idSrc >= 0)
        ss_ << idSrc << "->" << idDst << ";\n";
}

void vn::ToGraphViz::stepOut()
{
    stack_.pop_back();
    parent_ = stack_.empty() ? nullptr : stack_.back();
}

QString vn::ToGraphViz::digraphText() const
{
    return QString("digraph {\n%1\n}").arg(QString::fromStdString(ss_.str()));
}
void vn::Compute::visit(const Equal &op)
{
    opToFoo_.insert(&op, LazyValue([this]{
        int l;
        int r;
        if (!read(values_[0], l))
            return Value("Missing left part of the equality!");
        if (!read(values_[1], r))
            return Value("Missing right part of the equality!");

        values_.pop_back();
        values_.pop_back();
        return Value(l == r);
    }));
}

void vn::Compute::visit(const NonEqual &op)
{
    opToFoo_.insert(&op, LazyValue([]{ return Op::Value(13); }));
}

void vn::Compute::visit(const Static42 &op)
{
    opToFoo_.insert(&op, Value(42));
}

void vn::Compute::visit(const Static69 &op)
{
    opToFoo_.insert(&op, Value(69));
}

void vn::Compute::stepIn(const Node &node)
{
    stack_.push_back(&node);
}

void vn::Compute::stepOut()
{
    Q_ASSERT(!stack_.isEmpty());
    const Node *node = stack_.back();
    stack_.pop_back();

    MaybeLazyValue maybeLazyValue = opToFoo_.value(node);
    Value *value = std::get_if<Value>(&maybeLazyValue);
    if (value != nullptr) {
        values_.push_front(*value);
        std::cout << "get " << node << " -> " << *value << "\n";
        return;
    }

    const auto foo = std::get<LazyValue>(maybeLazyValue);
    const Value res = foo();
    opToFoo_.insert(node, res);
    values_.push_front(res);
    std::cout << "compute " << node << " -> " << res << "\n";
}

bool vn::Compute::shouldStop() const
{
    return false;
}

void vn::Static42::accept(Visitor &visitor) const
{
    return visitor.visit(*this);
}

void vn::Static69::accept(Visitor &visitor) const
{
    return visitor.visit(*this);
}

void vn::Equal::accept(Visitor &visitor) const
{
    return visitor.visit(*this);
}

void vn::NonEqual::accept(Visitor &visitor) const
{
    return visitor.visit(*this);
}

std::ostream &operator<<(std::ostream &stream, const vn::Op::Value &value)
{
    std::visit([&stream](auto &&arg) {
        using T = std::decay_t<decltype(arg)>;
        const std::size_t ind = vn::variant_index<vn::Op::Value, T>();
        stream << vn::Op::names.at(ind);
        if constexpr (std::is_same_v<T, int>)
            stream << " " << arg;
        else if constexpr (std::is_same_v<T, bool>)
            stream << (arg ? " True" : " False");
    }, value);
    return stream;
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
    nodeFrameStart->title_ = "Start";
    nodeFrameStart->text_ = "Welcome!";
    nodeFrameChoice->title_ = "Choice";
    nodeFrameChoice->text_ = "Where would you go?";
    nodeFrameMines->title_ = "Golden mines";
    nodeFrameMines->text_ = "You are approaching the golden mines";
    nodeFrameMinesWork->title_ = "Work";
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

    const vn::NodeId id42 = graph.add(new vn::Static42);
    const vn::NodeId id69 = graph.add(new vn::Static69);
    const vn::NodeId idEq = graph.add(new vn::Equal);
    const vn::NodeId idNeq = graph.add(new vn::NonEqual);
    const vn::NodeId idEq2 = graph.add(new vn::Equal);

    /// 42 -> != <- 69
    graph.connect(idNeq, id42);
    graph.connect(idNeq, id69);
    graph.connect(idEq, id42);
    graph.connect(idEq, id42);
    graph.connect(idEq2, idEq);
    graph.connect(idEq2, idNeq);

    /// polish notation: 42 42 == 42 69 != ==

    try {
        vn::Print print = vn::Print(nodeFrameChoice);
        vn::traverse(graph, 1, print);
        qDebug() << "";

        print = vn::Print(nodeFrameShop);
        vn::traverse(graph, 4, print);
        qDebug() << "";

        vn::ToGraphViz json;
        vn::traverse(graph, 0, json);
        qDebug() << json.digraphText();
        qDebug() << "";

        vn::Compute compute;
        vn::traverse(graph, idEq2, compute);

    } catch (const vn::Error &err) {
        qDebug() << err.message;
        return 1;
    }


    return 0;
}
