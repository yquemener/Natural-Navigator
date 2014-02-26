
#include <QtGui/QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
        //printf("Natural Navigator interface\n(C) 2011 IV-devs\n(C) 2011 Studio CreArtCom\n");
    MainWindow w;
    w.show();

    return a.exec();
}
