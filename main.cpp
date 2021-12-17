#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.setWindowIcon(QIcon(":/image/KSSA_Icon.png"));
    w.setWindowTitle("KSSA-Server");
    w.show();
    return a.exec();
}
