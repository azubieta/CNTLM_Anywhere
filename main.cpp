#include <QtQml>
#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include "cntlmwrapper.h"

static QObject *cntlmwrapper_singletontype_provider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)

    CntlmWrapper *example = new CntlmWrapper();
    return example;
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setOrganizationName("Project42");
    QGuiApplication::setOrganizationDomain("uci.cu");
    QGuiApplication::setApplicationName("CntlmAnywhere");


    qmlRegisterSingletonType<CntlmWrapper>("cu.uci.cntlm", 1, 0, "Cntlm", cntlmwrapper_singletontype_provider);

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

    return app.exec();
}

