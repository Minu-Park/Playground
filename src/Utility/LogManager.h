#pragma once
#include <QObject>
#include <QQueue>
#include <QString>
#include <QMutex>

class StdStreamRedirector; // Forward declaration

class LogManager : public QObject {
    Q_OBJECT
public:
    static LogManager* instance();
    
    static void qtMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);

    void init();
    void log(QtMsgType type, const QString& msg);
    void logDirect(const QString& msg, QtMsgType type);

signals:
    void logAdded(const QString& formattedMsg);

private:
    LogManager(QObject* parent = nullptr);
    ~LogManager() override;

    void writeToLogFile();

    QQueue<QString> _logBuffer;
    QMutex _mutex;
    QString _logFilePath;

    StdStreamRedirector* _coutRedirector = nullptr;
    StdStreamRedirector* _cerrRedirector = nullptr;
};
