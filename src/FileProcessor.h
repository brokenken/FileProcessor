#pragma once

#include <QObject>
#include <QString>
#include <QMutex>
#include <QWaitCondition>
#include <atomic>

class FileProcessor final : public QObject
{
    Q_OBJECT
public:
    explicit FileProcessor(QString inputPath,
        QString outputPath,
        quint64 xorValue,
        bool deleteFile,
        QObject *parent = nullptr);
public slots:
    void processFile();
    void pause();
    void resume();
    void stop();
signals:
    void started();
    void progressChanged(int32_t percent);
    void statusChanged(const QString &status);
    void errorDuringProcessing(const QString &msg);
    void finished(bool status, const QString &msg);
private:
    void applyOperation(QByteArray& arr);
    void saveToByteArray(quint64 xorValue);
    void nextIdx();
    void clear();
private:
    QString m_inputPath;
    QString m_outputPath;

    QByteArray m_byteArray;
    int64_t m_currIdx {0};

    bool m_deleteFile;

    std::atomic<bool> m_stopRequested {false};
    std::atomic<bool> m_pauseRequested {false};

    QMutex m_pauseMutex;
    QWaitCondition m_pauseCondition;

    static constexpr quint64 m_bufferSize = 64 * 1024 * 1024; // 64 Mb
};
