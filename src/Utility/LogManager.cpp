#include "Utility/LogManager.h"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QMutexLocker>
#include <QStandardPaths>
#include <QDebug>
#include <iostream>
#include <streambuf>
#include <string>

// Custom stream buffer to redirect std::cout and std::cerr to the LogManager
class StdStreamRedirector : public std::streambuf {
public:
    StdStreamRedirector(std::ostream& stream, QtMsgType type)
        : _stream(stream), _type(type) {
        _oldBuf = _stream.rdbuf(this);
    }

    ~StdStreamRedirector() override {
        processBuffer(true);
        _stream.rdbuf(_oldBuf);
    }

    [[nodiscard]] std::streambuf* originalBuffer() const {
        return _oldBuf;
    }

protected:
    int_type overflow(int_type v) override {
        if (v != traits_type::eof()) {
            _buffer.push_back(static_cast<char>(v));
            if (v == '\n') {
                processBuffer(false);
            }
        }
        return v;
    }

    int sync() override {
        processBuffer(false);
        return 0;
    }

private:
    void processBuffer(bool force = false) {
        size_t pos;
        while ((pos = _buffer.find('\n')) != std::string::npos) {
            std::string line = _buffer.substr(0, pos);
            _buffer.erase(0, pos + 1);
            QString msg = QString::fromStdString(line).trimmed();
            if (!msg.isEmpty()) {
                LogManager::instance()->logDirect(msg, _type);
            }
        }
        if (force && !_buffer.empty()) {
            QString msg = QString::fromStdString(_buffer).trimmed();
            _buffer.clear();
            if (!msg.isEmpty()) {
                LogManager::instance()->logDirect(msg, _type);
            }
        }
    }

    std::ostream& _stream;
    std::streambuf* _oldBuf;
    std::string _buffer;
    QtMsgType _type;
};

LogManager* LogManager::instance() {
    static LogManager inst;
    return &inst;
}

LogManager::LogManager(QObject* parent) : QObject(parent) {
    QString logRoot = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (logRoot.isEmpty()) {
        logRoot = QDir::tempPath() + QStringLiteral("/Playground");
    }
    QDir().mkpath(logRoot);
    _logFilePath = QDir(logRoot).filePath(QStringLiteral("lastlog.log"));
}

LogManager::~LogManager() {
    // Unregister handler and delete redirectors first to restore original stream buffers
    qInstallMessageHandler(nullptr);
    delete _coutRedirector;
    delete _cerrRedirector;
}

void LogManager::init() {
    {
        QFile file(_logFilePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            file.close();
        }
    }

    // 1. Install Qt global message handler
    qInstallMessageHandler(LogManager::qtMessageHandler);

    // 2. Redirect std::cout and std::cerr to catch external module logs
    _coutRedirector = new StdStreamRedirector(std::cout, QtInfoMsg);
    _cerrRedirector = new StdStreamRedirector(std::cerr, QtWarningMsg);

    qDebug() << "LogManager initialized. Log file path:" << _logFilePath;
}

void LogManager::qtMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
    LogManager::instance()->log(type, msg);
}

void LogManager::logDirect(const QString& msg, QtMsgType type) {
    // Directly process logs bypassing the qtMessageHandler setup
    log(type, msg);
}

void LogManager::log(QtMsgType type, const QString& msg) {
    QMutexLocker locker(&_mutex);

    QString typeStr;
    QString color;
    switch (type) {
        case QtDebugMsg:    typeStr = QStringLiteral("DEBUG"); color = QStringLiteral("#78909c"); break; // Blue-gray
        case QtInfoMsg:     typeStr = QStringLiteral("INFO");  color = QStringLiteral("#1e88e5"); break; // Blue
        case QtWarningMsg:  typeStr = QStringLiteral("WARN");  color = QStringLiteral("#fb8c00"); break; // Orange
        case QtCriticalMsg: typeStr = QStringLiteral("CRIT");  color = QStringLiteral("#e53935"); break; // Red
        case QtFatalMsg:    typeStr = QStringLiteral("FATAL"); color = QStringLiteral("#b71c1c"); break; // Dark red
    }

    QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz"));
    
    // 1. Plain text format for logging file and console output
    QString plainLogMsg = QStringLiteral("[%1] [%2] %3").arg(timestamp, typeStr, msg);

    // 2. HTML format with curated color schemes for premium white-theme GUI console
    QString htmlLogMsg = QStringLiteral("<span style=\"color:#7f8c8d;\">[%1]</span> <span style=\"color:%2; font-weight:bold;\">[%3]</span> %4")
        .arg(timestamp, color, typeStr, msg.toHtmlEscaped());

    // 3. Fallback to original terminal console output without infinite looping
    std::streambuf* consoleBuf = nullptr;
    if (type == QtWarningMsg || type == QtCriticalMsg || type == QtFatalMsg) {
        consoleBuf = _cerrRedirector ? _cerrRedirector->originalBuffer() : std::cerr.rdbuf();
    } else {
        consoleBuf = _coutRedirector ? _coutRedirector->originalBuffer() : std::cout.rdbuf();
    }

    if (consoleBuf) {
        std::ostream originalStream(consoleBuf);
        originalStream << plainLogMsg.toStdString() << std::endl;
    }

    // 4. Update the rolling 500-line queue
    _logBuffer.enqueue(plainLogMsg);
    while (_logBuffer.size() > 500) {
        _logBuffer.dequeue();
    }

    appendToLogFile(plainLogMsg);

    // 5. Emit the html log for UI render
    emit logAdded(htmlLogMsg);
}

void LogManager::appendToLogFile(const QString& line) {
    QFile file(_logFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
        return;
    }

    QTextStream stream(&file);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    stream.setEncoding(QStringConverter::Utf8);
#else
    stream.setCodec("UTF-8");
#endif

    stream << line << "\n";
    file.close();
}
