#include "mainwindow.h"

#include <QApplication>
#include <QFile>
#include <QFont>
#include <QFontDatabase>
#include <QLibraryInfo>
#include <QLocale>
#include <QStyleFactory>
#include <QTextStream>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QLocale::setDefault(QLocale(QLocale::Spanish, QLocale::Spain));
    QTranslator qtTranslator;
    if (qtTranslator.load(QLocale(),
                          QStringLiteral("qtbase"),
                          QStringLiteral("_"),
                          QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
        a.installTranslator(&qtTranslator);
    }

#ifdef Q_OS_WIN
    if (auto *fusionStyle = QStyleFactory::create(QStringLiteral("Fusion"))) {
        a.setStyle(fusionStyle);
    }
#endif

    QFontDatabase::addApplicationFont(":/fonts/Inter-Regular.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Inter-SemiBold.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Inter-Bold.ttf");
    a.setFont(QFont(QStringLiteral("Inter")));

    // Load and apply stylesheet
    QFile file(":/stylesheet.qss");
    if(file.open(QFile::ReadOnly | QFile::Text))
    {
        QTextStream stream(&file);
        a.setStyleSheet(stream.readAll());
        file.close();
    }

    MainWindow w;
    w.show();
    return a.exec();
}
