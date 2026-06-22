#include "AppController.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    AppController appController;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("appController"), &appController);
    engine.addImportPath(QCoreApplication::applicationDirPath() + "/qml");

    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return QGuiApplication::exec();
}