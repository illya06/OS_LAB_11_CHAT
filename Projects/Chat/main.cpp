#include "mainwindow.h"
#include "frameless.h"
#include <QtWidgets/qapplication.h>
#include <QApplication>
void start_loops();
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    FrameLess f(&w);
    w.show();
    return a.exec();
}
