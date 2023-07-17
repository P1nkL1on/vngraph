#ifndef WINDOW_H
#define WINDOW_H

#include <memory>
#include <QMainWindow>
#include <QPushButton>
#include <QTextEdit>
#include <QBoxLayout>

template <typename T>
using Ptr = std::shared_ptr<T>;
using Text = QString;
using FilePath = QString;

struct INode
{
    virtual ~INode() = default;
    virtual Text cover() const = 0;
    virtual Text text() const = 0;
    virtual Ptr<INode> next(const int ind) = 0;
};

struct Node final : INode
{
    Text cover_;
    Text text_;
    std::vector<Ptr<INode>> nodes_;
    inline Text cover() const override { return cover_; }
    inline Text text() const override { return text_; }
    Ptr<INode> next(const int ind) override;
};


class Window : public QMainWindow
{
    Q_OBJECT
public:
    explicit Window(QWidget *parent = nullptr);

private:
    static Ptr<INode> readTree(const FilePath &filePath);
    static void printNode(Window *w, Ptr<INode> root);
    void onClicked(const int ind);

    Ptr<INode> root_ = nullptr;
    QTextEdit *widgetText_ = nullptr;
    QVector<QPushButton *> widgetButtons_;
    QBoxLayout *layoutButtons_ = nullptr;
};

#endif // WINDOW_H
