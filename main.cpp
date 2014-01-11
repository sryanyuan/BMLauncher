#include "dialog.h"
#include <QApplication>
#include <QFile>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Dialog w;
    w.show();

    //  Load the css file
    QFile xCSSFile(":/qss/main.qss");
    xCSSFile.open(QFile::ReadOnly);
    if(xCSSFile.isOpen())
    {
        a.setStyleSheet(xCSSFile.readAll());
        xCSSFile.close();
    }

    return a.exec();
}
