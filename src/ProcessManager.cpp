#include "ProcessManager.h"

ProcessManager::ProcessManager(QObject* parent) : QObject(parent) {}
ProcessManager::~ProcessManager()
{
    stop();
    if (m_thread) {
        m_thread->quit();
        m_thread->wait();
    }
}

bool ProcessManager::isRunning() const
{
    return m_isRunning.load();
}

bool ProcessManager::isPaused() const
{
    return m_isPaused.load();
}

int32_t ProcessManager::currTaskId() const
{
    return m_currTaskId;
}
void ProcessManager::setTasks(const QVector<ProcessingTask>& tasks)
{
    if (m_isRunning.load()) {
        emit errorDuringProcessing(QStringLiteral("Task already running"));
        return;
    }
    m_tasks = tasks;
    m_currTaskId = 0;

    emit totalProgressChanged(0);
}

void ProcessManager::start()
{
    if (m_isRunning.load()) {
        return;
    }

    if (m_tasks.isEmpty()) {
        emit finished();
        return;
    }
    m_isRunning.store(true);
    m_isPaused.store(false);
    m_stopRequested.store(false);
    m_currTaskId = 0;

    emit started();
    processNextTask();
}

void ProcessManager::stop()
{
    if (!m_isRunning.load()) {
        return;
    }
    m_stopRequested.store(true);
    m_isPaused.store(false);
    if (m_fileProcessor) {
        QMetaObject::invokeMethod(m_fileProcessor, "stop", Qt::QueuedConnection);
    }
}

void ProcessManager::pause()
{
    if (!m_isRunning.load() || m_isPaused.load() || !m_fileProcessor) {
        return;
    }
    m_isPaused.store(true);
    QMetaObject::invokeMethod(m_fileProcessor, "pause", Qt::QueuedConnection);

    emit paused();
}

void ProcessManager::resume()
{
    if (!m_isRunning.load() || !m_isPaused.load() || !m_fileProcessor) {
        return;
    }
    m_isPaused.store(false);
    QMetaObject::invokeMethod(m_fileProcessor, "resume", Qt::QueuedConnection);
    emit resumed();
}

void ProcessManager::processNextTask()
{
    if (m_stopRequested.load()) {
        m_isRunning.store(false);
        m_isPaused.store(false);

        emit stopped();
        emit finished();
        return;
    }
    if (m_currTaskId >= m_tasks.size()) {
        clearCurrWorker();
        m_isRunning.store(false);
        m_isPaused.store(false);

        emit totalProgressChanged(100);
        emit finished();
        return;
    }
    const auto task = m_tasks[m_currTaskId];
    emit fileChanged(task.inputPath);
    emit progressChanged(0);

    updateTotalProgress(0);
    m_thread = new QThread;
    m_fileProcessor = new FileProcessor(task.inputPath,
        task.outputPath,
        task.xorValue,
        task.deleteInputFile);
    m_fileProcessor->moveToThread(m_thread);

    connect(m_thread, &QThread::started, m_fileProcessor, &FileProcessor::processFile);
    connect(m_fileProcessor, &FileProcessor::progressChanged, this, [this](int32_t percent)
    {
        emit progressChanged(percent);
        updateTotalProgress(percent);
    });
    connect(m_fileProcessor, &FileProcessor::errorDuringProcessing, this, &ProcessManager::errorDuringProcessing);
    connect(m_fileProcessor, &FileProcessor::finished, this, &ProcessManager::onCurrTaskFinished);
    connect(m_fileProcessor, &FileProcessor::finished, m_thread, &QThread::quit);
    connect(m_thread, &QThread::finished, m_fileProcessor, &QObject::deleteLater);
    connect(m_thread, &QThread::finished, m_thread, &QObject::deleteLater);
    m_thread->start();
}

void ProcessManager::onCurrTaskFinished(bool success, const QString &message)
{
    m_fileProcessor = nullptr;
    m_thread = nullptr;
    if (!success && m_stopRequested.load()) {
        m_isRunning.store(false);
        m_isPaused.store(false);
        emit stopped();
        emit finished();
        return;
    }
    if (!success) {
        emit errorDuringProcessing(message);
    }
    ++m_currTaskId;
    processNextTask();
}

void ProcessManager::clearCurrWorker()
{
    m_fileProcessor = nullptr;
    m_thread = nullptr;
}

void ProcessManager::updateTotalProgress(int currentFilePercent)
{
    if (m_tasks.isEmpty() || m_currTaskId < 0) {
        emit totalProgressChanged(0);
        return;
    }

    const int completedTasks = m_currTaskId;
    const int totalTasks = m_tasks.size();

    const int totalPercent = static_cast<int>(
        ((completedTasks * 100.0) + currentFilePercent) / totalTasks
    );

    emit totalProgressChanged(totalPercent);
}