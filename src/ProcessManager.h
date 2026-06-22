#pragma once

#include <QThread>

#include <QObject>

#include "FileProcessor.h"

struct ProcessingTask
{
    QString inputPath;
    QString outputPath;
    quint64 xorValue;
    bool deleteInputFile;
};

class ProcessManager final : public QObject
{
    Q_OBJECT
public:
    explicit ProcessManager(QObject *parent = nullptr);
    ~ProcessManager() override;
    bool isRunning() const;
    bool isPaused() const;
    int32_t currTaskId() const;

public slots:
    void setTasks(const QVector<ProcessingTask> &tasks);
    void start();
    void pause();
    void resume();
    void stop();

signals:
    void started();
    void paused();
    void resumed();
    void stopped();
    void finished();
    void fileChanged(const QString &path);
    void progressChanged(int32_t percent);
    void totalProgressChanged(int32_t percent);
    void errorDuringProcessing(const QString &error);

private slots:
    void processNextTask();
    void onCurrTaskFinished(bool status, const QString &msg);

private:
    void clearCurrWorker();
    void updateTotalProgress(int32_t percent);

private:
    QVector<ProcessingTask> m_tasks;
    QThread* m_thread {nullptr};
    FileProcessor* m_fileProcessor {nullptr};
    int32_t m_currTaskId {0};
    std::atomic<bool> m_isRunning {false};
    std::atomic<bool> m_isPaused {false};
    std::atomic<bool> m_stopRequested {false};
};
