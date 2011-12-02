
#include <QtGui/QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
		printf("Virtopia interface\n(C) 2011 Yves Quemener\n");
    MainWindow w;
    w.show();

    return a.exec();
}
