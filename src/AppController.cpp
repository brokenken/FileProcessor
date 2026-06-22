#include "AppController.h"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>

AppController::AppController(QObject* parent) : QObject(parent)
{
    m_pollTimer.setInterval(m_pollIntervalMs);

    connect(&m_pollTimer, &QTimer::timeout, this, &AppController::scanAndStart);

    connect(&m_processManager,
        &ProcessManager::started,
        this,
        [this]()
        {
            setRunning(true);
            setPaused(false);
        });

    connect(&m_processManager,
        &ProcessManager::paused,
        this,
        [this]()
        {
            setPaused(true);
        });

    connect(&m_processManager,
        &ProcessManager::resumed,
        this,
        [this]()
        {
            setPaused(false);
        });

    connect(&m_processManager,
        &ProcessManager::stopped,
        this,
        [this]()
        {
            setRunning(false);
            setPaused(false);
        });

    connect(&m_processManager, &ProcessManager::finished, this, &AppController::onManagerFinished);

    connect(&m_processManager,
        &ProcessManager::fileChanged,
        this,
        [this](const QString& path)
        {
            setCurrentFile(path);
        });

    connect(&m_processManager,
        &ProcessManager::progressChanged,
        this,
        [this](int32_t percent)
        {
            setCurrentFileProgress(percent);
        });

    connect(&m_processManager,
        &ProcessManager::totalProgressChanged,
        this,
        [this](int32_t percent)
        {
            setTotalProgress(percent);
        });

    connect(&m_processManager,
        &ProcessManager::errorDuringProcessing,
        this,
        [this](const QString& message)
        {
            emit errorOccurred(message);
        });
}

AppController::~AppController()
{
    stop();
}

QString AppController::inputDirectory() const
{
    return m_inputDirectory;
}

void AppController::setInputDirectory(const QString& value)
{
    if (m_inputDirectory == value) {
        return;
    }
    m_inputDirectory = value;
    emit inputDirectoryChanged();
}

QString AppController::outputDirectory() const
{
    return m_outputDirectory;
}

void AppController::setOutputDirectory(const QString& value)
{
    if (m_outputDirectory == value) {
        return;
    }
    m_outputDirectory = value;
    emit outputDirectoryChanged();
}

QString AppController::fileMask() const
{
    return m_fileMask;
}

void AppController::setFileMask(const QString& value)
{
    if (m_fileMask == value) {
        return;
    }
    m_fileMask = value;
    emit fileMaskChanged();
}

QString AppController::xorHexValue() const
{
    return m_xorHexValue;
}

void AppController::setXorHexValue(const QString& value)
{
    if (m_xorHexValue == value) {
        return;
    }
    m_xorHexValue = value.trimmed().toUpper();
    emit xorHexValueChanged();
}

bool AppController::deleteInputFiles() const
{
    return m_deleteInputFiles;
}

void AppController::setDeleteInputFiles(bool value)
{
    if (m_deleteInputFiles == value) {
        return;
    }
    m_deleteInputFiles = value;
    emit deleteInputFilesChanged();
}

bool AppController::overwriteOutputFiles() const
{
    return m_overwriteOutputFiles;
}

void AppController::setOverwriteOutputFiles(bool value)
{
    if (m_overwriteOutputFiles == value) {
        return;
    }
    m_overwriteOutputFiles = value;
    emit overwriteOutputFilesChanged();
}

bool AppController::timerMode() const
{
    return m_timerMode;
}

void AppController::setTimerMode(bool value)
{
    if (m_timerMode == value) {
        return;
    }
    m_timerMode = value;
    emit timerModeChanged();
}

int AppController::pollIntervalMs() const
{
    return m_pollIntervalMs;
}

void AppController::setPollIntervalMs(int value)
{
    if (value < 100) {
        value = 100;
    }
    if (m_pollIntervalMs == value) {
        return;
    }
    m_pollIntervalMs = value;
    m_pollTimer.setInterval(m_pollIntervalMs);
    emit pollIntervalMsChanged();
}

bool AppController::running() const
{
    return m_running;
}

bool AppController::paused() const
{
    return m_paused;
}

int AppController::currentFileProgress() const
{
    return m_currentFileProgress;
}

int AppController::totalProgress() const
{
    return m_totalProgress;
}

QString AppController::currentFile() const
{
    return m_currentFile;
}

void AppController::start()
{
    if (m_running) {
        return;
    }
    if (!validateSettings()) {
        return;
    }
    setCurrentFile(QString());
    setCurrentFileProgress(0);
    setTotalProgress(0);
    scanAndStart();
    if (m_timerMode) {
        m_pollTimer.start();
    }
}

void AppController::pause()
{
    if (!m_running || m_paused) {
        return;
    }
    m_processManager.pause();
}

void AppController::resume()
{
    if (!m_running || !m_paused) {
        return;
    }
    m_processManager.resume();
}

void AppController::stop()
{
    m_pollTimer.stop();
    if (m_running) {
        m_processManager.stop();
    }
}

void AppController::scanAndStart()
{
    if (m_processManager.isRunning()) {
        return;
    }
    const QVector<ProcessingTask> tasks = buildTasks();
    if (tasks.isEmpty()) {
        if (!m_timerMode) {
            setRunning(false);
        }
        return;
    }
    m_processManager.setTasks(tasks);
    m_processManager.start();
}

void AppController::onManagerFinished()
{
    setRunning(false);
    setPaused(false);
    if (!m_timerMode) {
        m_pollTimer.stop();
    }
}

QVector<ProcessingTask> AppController::buildTasks() const
{
    quint64 xorValue = 0;
    if (!parseXorValue(xorValue)) {
        return {};
    }
    const QStringList inputFiles = findInputFiles();
    QVector<ProcessingTask> tasks;
    tasks.reserve(inputFiles.size());
    for (const QString& inputPath : inputFiles) {
        ProcessingTask task;
        task.inputPath = inputPath;
        task.outputPath = buildOutputPath(inputPath);
        task.xorValue = xorValue;
        task.deleteInputFile = m_deleteInputFiles;

        tasks.append(task);
    }
    return tasks;
}

QStringList AppController::findInputFiles() const
{
    QDir dir(m_inputDirectory);

    if (!dir.exists()) {
        return {};
    }
    QString mask = m_fileMask.trimmed();
    if (mask.isEmpty()) {
        mask = QStringLiteral("*.*");
    }
    if (mask.startsWith('.')) {
        mask.prepend('*');
    }
    const QStringList filters{mask};
    const QFileInfoList entries = dir.entryInfoList(filters, QDir::Files | QDir::Readable, QDir::Name);
    QStringList result;
    result.reserve(entries.size());
    for (const QFileInfo& entry : entries) {
        result.append(entry.absoluteFilePath());
    }
    return result;
}

QString AppController::buildOutputPath(const QString& inputPath) const
{
    const QFileInfo inputInfo(inputPath);
    const QString outputPath = QDir(m_outputDirectory).filePath(inputInfo.fileName());
    if (m_overwriteOutputFiles) {
        return outputPath;
    }
    return makeUniqueOutputPath(outputPath);
}

QString AppController::makeUniqueOutputPath(const QString& path) const
{
    if (!QFileInfo::exists(path)) {
        return path;
    }
    const QFileInfo info(path);
    const QDir dir = info.dir();
    const QString baseName = info.completeBaseName();
    const QString suffix = info.suffix();
    for (int i = 1; i < 100000; ++i) {
        const QString fileName = suffix.isEmpty() ? QStringLiteral("%1_%2").arg(baseName).arg(i)
                                                  : QStringLiteral("%1_%2.%3").arg(baseName).arg(i).arg(suffix);
        const QString candidate = dir.filePath(fileName);
        if (!QFileInfo::exists(candidate)) {
            return candidate;
        }
    }
    return path;
}

bool AppController::parseXorValue(quint64& value) const
{
    static const QRegularExpression regex(QStringLiteral("^[0-9A-Fa-f]{16}$"));
    const QString hex = m_xorHexValue.trimmed();
    if (!regex.match(hex).hasMatch()) {
        return false;
    }
    bool ok = false;
    value = hex.toULongLong(&ok, 16);
    return ok;
}

bool AppController::validateSettings()
{
    if (!QDir(m_inputDirectory).exists()) {
        emit errorOccurred(QStringLiteral("Input directory does not exist"));
        return false;
    }
    if (!QDir(m_outputDirectory).exists()) {
        emit errorOccurred(QStringLiteral("Output directory does not exist"));
        return false;
    }
    quint64 value = 0;
    if (!parseXorValue(value)) {
        emit errorOccurred(QStringLiteral("Value must contain exactly 16 symbols"));
        return false;
    }
    return true;
}

void AppController::setRunning(bool value)
{
    if (m_running == value) {
        return;
    }
    m_running = value;
    emit runningChanged();
}

void AppController::setPaused(bool value)
{
    if (m_paused == value) {
        return;
    }
    m_paused = value;
    emit pausedChanged();
}

void AppController::setCurrentFileProgress(int value)
{
    if (m_currentFileProgress == value) {
        return;
    }
    m_currentFileProgress = value;
    emit currentFileProgressChanged();
}

void AppController::setTotalProgress(int value)
{
    if (m_totalProgress == value) {
        return;
    }
    m_totalProgress = value;
    emit totalProgressChanged();
}

void AppController::setCurrentFile(const QString& value)
{
    if (m_currentFile == value) {
        return;
    }
    m_currentFile = value;
    emit currentFileChanged();
}
