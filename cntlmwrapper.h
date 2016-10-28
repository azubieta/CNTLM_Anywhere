#ifndef CNTLMWRAPPER_H
#define CNTLMWRAPPER_H

#include <QObject>
#include <QString>

class CntlmWrapper : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString user READ getUser WRITE setUser)
    Q_PROPERTY(QString password READ getPassword WRITE setPassword)
    Q_PROPERTY(QString listen READ getListen WRITE setListen)
    Q_PROPERTY(QString proxy READ getProxy WRITE setProxy)
    Q_PROPERTY(bool running READ getRunning NOTIFY runningChanged)

    Q_PROPERTY(bool memorizeSettings READ getMemorizeSettings WRITE setMemorizeSettings)
public:
    explicit CntlmWrapper(QObject *parent = 0);

    QString getUser() const;
    void setUser(const QString &value);

    QString getPassword() const;
    void setPassword(const QString &value);

    bool getRunning() const;

    QString getListen() const;
    void setListen(const QString &value);

    QString getProxy() const;
    void setProxy(const QString &value);

    bool getMemorizeSettings() const;
    void setMemorizeSettings(bool value);

signals:
    void runningChanged(bool running);
    void error(QString msg);

public slots:
    void start();
    void stop();

private:
    static void cntlm_start(QString listen, QString user, QString password, QString proxy, CntlmWrapper *instance);
    static char ** QStringListToCharArray(const QStringList list);
    QString user;
    QString password;
    QString listen;
    QString proxy;
    bool running;
    bool memorizeSettings : true;
};

#endif // CNTLMWRAPPER_H
