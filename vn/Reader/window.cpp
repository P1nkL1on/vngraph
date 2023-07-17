#include "window.h"
#include <QScrollArea>
#include <QFile>
#include <QtDebug>
#include <QTextStream>

Window::Window(QWidget *parent) : QMainWindow(parent)
{
    /// spawn UI
    widgetText_ = new QTextEdit;
    widgetText_->setReadOnly(true);
    widgetText_->clear();

    auto *sa1 = new QScrollArea;
    sa1->setWidget(widgetText_);
    sa1->setWidgetResizable(true);

    layoutButtons_ = new QVBoxLayout;
    layoutButtons_->setAlignment(Qt::AlignCenter);

    auto *q = new QWidget;
    q->setLayout(layoutButtons_);
    auto *sa2 = new QScrollArea;
    sa2->setWidget(q);
    sa2->setWidgetResizable(true);

    auto *l = new QVBoxLayout;
    l->addWidget(sa1, 2);
    l->addWidget(sa2, 1);

    auto *w = new QWidget;
    w->setLayout(l);
    setCentralWidget(w);

    /// read
    root_ = readTree("/home/pl/jff/vngraph/vn/script.md");

    /// step
    printNode(this, root_);
}

void Window::onClicked(const int ind)
{
    root_ = root_->next(ind);
    printNode(this, root_);
}

Ptr<INode> Window::readTree(const FilePath &filePath)
{
    QFile inputFile(filePath);
    if (!inputFile.open(QIODevice::ReadOnly))
        return nullptr;
    QTextStream in(&inputFile);
    struct IParser
    {
        virtual ~IParser() = default;
        virtual int newNode(const Text &cover, const Text &text) = 0;
        virtual void connect(const int parent, const int child) = 0;
        virtual Text line(const int) const = 0;
    };

    struct Reader
    {
        IParser *parser_ = nullptr;
        std::vector<int> stack_;
        bool hasRoot_ = false;

        static int numberOfSpaces(const Text &line)
        {
            return line.indexOf(QRegExp("[^\\s]"));
        }
        static int numberInFront(const Text &line)
        {
            const int ind = line.indexOf(QRegExp("^\\s*\\d+\\. "));
            if (ind < 0)
                return -1;
            return line.mid(0, line.indexOf(". ")).toInt();
        }
        static int last(const std::vector<int> &stack, const int ind = 0)
        {
            const int n = stack.size();
            Q_ASSERT(n > ind);
            return stack.at(n - 1 - ind);
        }
        bool operator()(const Text &line)
        {
            if (!hasRoot_) {
                hasRoot_ = true;
                const int id = parser_->newNode("", line);
                stack_.push_back(id);
                return true;
            }

            int lastId_ = last(stack_);
            const int tab = numberOfSpaces(parser_->line(lastId_));
            const int tab2 = numberOfSpaces(line);
            if (tab2 <= tab) {
                if (tab2 < tab) {
                    /// NOTE: poping until new tab will match last in stack
                    /// and it won't be a numeric option
                    for (;;) {
                        if (stack_.empty()) {
                            qWarning() << "Bad tab alignment on returning to previous lines. Failed at "<< line;
                            return false;
                        }
                        stack_.pop_back();
                        lastId_ = last(stack_);
                        const int tab0 = numberOfSpaces(parser_->line(lastId_));
                        const int num0 = numberInFront(parser_->line(lastId_));
                        if (tab0 <= tab2 && num0 == -1)
                            break;
                    }
                }
                const int numStart = numberInFront(parser_->line(lastId_));
                const int numStart2 = numberInFront(line);
                if (numStart2 != -1 && numStart != -1) {
                    stack_.pop_back();
                    lastId_ = last(stack_);
                }
                const bool isOption = (numStart2 != -1);
                if (!isOption) {
                    const int id = parser_->newNode("->", line);
                    parser_->connect(lastId_, id);
                    stack_.pop_back();
                    stack_.push_back(id);
                    return true;
                }
                // TODO: remove the number from line. cover is good
                const int id = parser_->newNode(line, line);
                parser_->connect(lastId_, id);
                stack_.push_back(id);
                return true;
            }
            if (tab2 > tab) {
                /// NOTE: option branch
                if (numberInFront(parser_->line(lastId_)) == -1) {
                    qWarning() << "Branch can be only from option. For now at least. Failed at "<< line;
                    return false;
                }
                const int id = parser_->newNode("->", line);
                parser_->connect(lastId_, id);
                stack_.push_back(id);
                return true;
            }
            qWarning() << "not implemented for " << line;
            return false;
        }
    } reader;

    struct Parser final : IParser
    {
        int newNode(const Text &cover, const Text &text) override
        {
            Ptr<Node> n(new Node);
            n->cover_ = cover;
            n->text_ = text;
            nodes_.push_back(n);
            return nodes_.size() - 1;
        }
        void connect(const int parent, const int child) override
        {
            Q_ASSERT(parent < nodes_.size());
            Q_ASSERT(child < nodes_.size());
            nodes_.at(parent)->nodes_.push_back(nodes_.at(child));
        }
        Text line(const int ind) const override
        {
            Q_ASSERT(ind < nodes_.size());
            return nodes_.at(ind)->text();
        }
        inline Ptr<INode> res() const
        {
            return nodes_.empty() ? nullptr : nodes_.at(0);
        }
    private:
        std::vector<Ptr<Node>> nodes_;
    };

    Parser parser;
    reader.parser_ = &parser;

    /// tests for Reader::numberOfSpaces
    Q_ASSERT(Reader::numberOfSpaces("ok") == 0);
    Q_ASSERT(Reader::numberOfSpaces("  ok") == 2);
    Q_ASSERT(Reader::numberOfSpaces("    ok") == 4);
    Q_ASSERT(Reader::numberOfSpaces("    yes yes") == 4);
    Q_ASSERT(Reader::numberOfSpaces(" ") == -1);
    Q_ASSERT(Reader::numberOfSpaces("") == -1);

    /// tests for Reader::numberInFront
    Q_ASSERT(Reader::numberInFront("") == -1);
    Q_ASSERT(Reader::numberInFront("ok") == -1);
    Q_ASSERT(Reader::numberInFront("  ok") == -1);
    Q_ASSERT(Reader::numberInFront("  a. ok") == -1);
    Q_ASSERT(Reader::numberInFront("  123.ok") == -1);
    Q_ASSERT(Reader::numberInFront("  123 .ok") == -1);
    Q_ASSERT(Reader::numberInFront("  123 . ok") == -1);
    Q_ASSERT(Reader::numberInFront("  123. ok") == 123);
    Q_ASSERT(Reader::numberInFront("5. ok") == 5);
    Q_ASSERT(Reader::numberInFront("    13. ok 14") == 13);
    Q_ASSERT(Reader::numberInFront("  42. 1. 2. 3") == 42);

    while (!in.atEnd()) {
        const Text line = in.readLine();
        // NOTE: stop parsing if fails
        if (!reader(line))
            break;
    }
    inputFile.close();
    return parser.res();
}

void Window::printNode(Window *w, Ptr<INode> root)
{
    if (root != nullptr) {
        /// clear previous data
        for (auto *b : w->widgetButtons_) {
            w->layoutButtons_->removeWidget(b);
            b->setParent(nullptr);
            delete b;
        }
        w->widgetButtons_.clear();

        /// add new data
        w->widgetText_->append(Text("\n%1").arg(root->text().trimmed()));

        for (int i = 0;; ++i) {
            Ptr<INode> next = root->next(i);
            if (next == nullptr)
                break;
            const Text cover = next->cover();
            auto *b = new QPushButton(w);
            b->setText(cover);

            w->widgetButtons_.push_back(b);
            w->layoutButtons_->addWidget(b);
            connect(b, &QPushButton::clicked, w, [w, i]{ w->onClicked(i); });
        }
    }
}

Ptr<INode> Node::next(const int ind)
{
    return (ind >= nodes_.size()) ? nullptr : nodes_.at(ind);
}
