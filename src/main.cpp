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
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication app(argc, argv);

    qmlRegisterSingletonType<CntlmWrapper>("cu.uci.cntlmanywhere", 1, 0, "Cntlm", cntlmwrapper_singletontype_provider);

    QQmlApplicationEngine engine;
    engine.load(QUrl(QLatin1String("qrc:/src/qml/main.qml")));

    return app.exec();
}
