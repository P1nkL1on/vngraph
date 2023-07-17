#include <QApplication>
#include "window.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Window win;
    win.resize(600, 400);
    win.show();
    return a.exec();
}
