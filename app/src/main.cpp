#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include <libcntlm.h>

static QObject *libcntlm_singletontype_provider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)

    Libcntlm *example = new Libcntlm();
    return example;
}

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication app(argc, argv);

    qmlRegisterSingletonType<Libcntlm>("cu.uci.cntlmanywhere", 1, 0, "Cntlm", libcntlm_singletontype_provider);

    QQmlApplicationEngine engine;
    engine.load(QUrl(QLatin1String("qrc:/src/qml/main.qml")));

    return app.exec();
}
