#include "libcntlm.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>
#include <QFuture>
#include <QSettings>

#include <QDebug>


namespace ccntlm {
    extern "C" int start(int argc, char *argv[], char **errorMsg);
    extern "C" void stop();
}


Libcntlm::Libcntlm(QObject *parent) : QObject(parent)
{

    QSettings settings;
    listen = settings.value("listen", "8080").toString();
    user = settings.value("user", "").toString();
    password = settings.value("password", "").toString();
    proxy = settings.value("proxy", "10.0.0.1:8080").toString();

    running = false;
}

QString Libcntlm::getUser() const
{
    return user;
}

void Libcntlm::setUser(const QString &value)
{
    user = value;
}

QString Libcntlm::getPassword() const
{
    return password;
}

void Libcntlm::setPassword(const QString &value)
{
    password = value;
}

void Libcntlm::start()
{
    if (memorizeSettings) {
        QSettings settings;
        settings.setValue("listen", listen);
        settings.setValue("user", user);
        settings.setValue("password", password);
        settings.setValue("proxy", proxy);
        settings.sync();
    }

    running = true;
    Q_EMIT(runningChanged(running));

    QFuture<void> f = QtConcurrent::run(cntlm_start, listen, user, password, proxy, this);

    QFutureWatcher<void> *watcher = new QFutureWatcher<void>();
    watcher->setFuture(f);
    connect(watcher, &QFutureWatcher<void>::finished, [=]() {
        qDebug() << "FINISHED";
        running = false;
        Q_EMIT(runningChanged(running));

        watcher->deleteLater();
    });
}

void Libcntlm::stop()
{
    ccntlm::stop();
}

void Libcntlm::cntlm_start(QString listen, QString user, QString password, QString proxy, Libcntlm *instance)
{
    //char * argsStr = (char*) malloc(sizeof(char) * 1024);
//    sprintf(argsStr, "cntlm -vfl %s -u \"%s\" -p \"%s\" %s", listen.toLatin1().data(), user.toLatin1().data(), password.toLatin1().data(), proxy.toLatin1().data());
    QStringList argsList;
    argsList << "cntlm" << "-vfl" << listen << "-u" << user << "-p" << password << proxy;

    qDebug() << argsList;
    char ** cntlm_args = QStringListToCharArray(argsList);
    char * errorMsg = NULL;
    int argc = argsList.size();
    qDebug() << "Starting with "<< argc << "args ";
//    for(int i = 0; i < argc; i ++)
//        qDebug() << cntlm_args[i];

    int result = ccntlm::start(argc, cntlm_args, &errorMsg);
    for(int i = 0; i < argc; i ++)
        delete(cntlm_args[i]);
    delete cntlm_args;

    if (result != 0 && errorMsg != NULL) {
        QString errorString(errorMsg);
        emit( instance->error( errorString ) ) ;
    }

    qDebug() << "ended" << result;
}

char **Libcntlm::QStringListToCharArray(const QStringList list)
{
    int i =0;
    int size = list.size();
    char ** c = new char *[size];

    for (QString s : list) {
        const char* byteArray = s.toLocal8Bit().constData();
        int len = strlen(byteArray);
        c[i] = new char[len + 1];
        strcpy(c[i], byteArray);
        c[i][len] = '\0';
        i++;
    }

    return c;
}

QStringList Libcntlm::getNoProxy() const
{
    return noProxy;
}

void Libcntlm::setNoProxy(const QStringList &value)
{
    noProxy = value;
}

bool Libcntlm::getMemorizeSettings() const
{
    return memorizeSettings;
}

void Libcntlm::setMemorizeSettings(bool value)
{
    memorizeSettings = value;
}

QString Libcntlm::getProxy() const
{
    return proxy;
}

void Libcntlm::setProxy(const QString &value)
{
    proxy = value;
}

QString Libcntlm::getListen() const
{
    return listen;
}

void Libcntlm::setListen(const QString &value)
{
    listen = value;
}

bool Libcntlm::getRunning() const
{
    return running;
}
