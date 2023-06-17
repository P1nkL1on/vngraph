#include <QString>

namespace VisualNovelGraph {

using SpeakerId = int;
using Text = QString;

struct Visitor;
struct Counters;


struct Node
{
    virtual ~Node() = default;
    virtual void accept(const Visitor &) = 0;
};


struct Frame : Node
{
    virtual ~Frame() = default;
    virtual Text title() const = 0;
    virtual Text text() const = 0;
    virtual SpeakerId speakerId() const = 0;
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
    virtual void doIt(Counters &) const = 0;
};


struct Visitor
{
    virtual ~Visitor() = default;
    virtual void visitFrame(const Frame &) = 0;
};



}


namespace vn = VisualNovelGraph;


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
    return 0;
}
