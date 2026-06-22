#pragma once

#include "ProcessManager.h"

#include <QObject>
#include <QTimer>

class AppController final : public QObject{
    Q_OBJECT

    Q_PROPERTY(QString inputDirectory READ inputDirectory WRITE setInputDirectory NOTIFY inputDirectoryChanged)
    Q_PROPERTY(QString outputDirectory READ outputDirectory WRITE setOutputDirectory NOTIFY outputDirectoryChanged)
    Q_PROPERTY(QString fileMask READ fileMask WRITE setFileMask NOTIFY fileMaskChanged)
    Q_PROPERTY(QString xorHexValue READ xorHexValue WRITE setXorHexValue NOTIFY xorHexValueChanged)

    Q_PROPERTY(bool deleteInputFiles READ deleteInputFiles WRITE setDeleteInputFiles NOTIFY deleteInputFilesChanged)
    Q_PROPERTY(bool overwriteOutputFiles READ overwriteOutputFiles WRITE setOverwriteOutputFiles NOTIFY overwriteOutputFilesChanged)
    Q_PROPERTY(bool timerMode READ timerMode WRITE setTimerMode NOTIFY timerModeChanged)
    Q_PROPERTY(int pollIntervalMs READ pollIntervalMs WRITE setPollIntervalMs NOTIFY pollIntervalMsChanged)

    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(bool paused READ paused NOTIFY pausedChanged)

    Q_PROPERTY(int currentFileProgress READ currentFileProgress NOTIFY currentFileProgressChanged)
    Q_PROPERTY(int totalProgress READ totalProgress NOTIFY totalProgressChanged)
    Q_PROPERTY(QString currentFile READ currentFile NOTIFY currentFileChanged)
public:
    explicit AppController(QObject *parent = nullptr);
    ~AppController() override;

    QString inputDirectory() const;
    void setInputDirectory(const QString &value);

    QString outputDirectory() const;
    void setOutputDirectory(const QString &value);

    QString fileMask() const;
    void setFileMask(const QString &value);

    QString xorHexValue() const;
    void setXorHexValue(const QString &value);

    bool deleteInputFiles() const;
    void setDeleteInputFiles(bool value);

    bool overwriteOutputFiles() const;
    void setOverwriteOutputFiles(bool value);

    bool timerMode() const;
    void setTimerMode(bool value);

    int pollIntervalMs() const;
    void setPollIntervalMs(int value);

    bool running() const;
    bool paused() const;

    int currentFileProgress() const;
    int totalProgress() const;

    QString currentFile() const;

    Q_INVOKABLE void start();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void resume();
    Q_INVOKABLE void stop();
signals:
    void inputDirectoryChanged();
    void outputDirectoryChanged();
    void fileMaskChanged();
    void xorHexValueChanged();
    void deleteInputFilesChanged();
    void overwriteOutputFilesChanged();
    void timerModeChanged();
    void pollIntervalMsChanged();
    void runningChanged();
    void pausedChanged();
    void currentFileProgressChanged();
    void totalProgressChanged();
    void currentFileChanged();
    void errorOccurred(const QString &message);
private slots:
    void scanAndStart();
    void onManagerFinished();
private:
    QVector<ProcessingTask> buildTasks() const;
    QStringList findInputFiles() const;

    QString buildOutputPath(const QString &inputPath) const;
    QString makeUniqueOutputPath(const QString &path) const;

    bool parseXorValue(quint64 &value) const;
    bool validateSettings();

    void setRunning(bool value);
    void setPaused(bool value);
    void setCurrentFileProgress(int value);
    void setTotalProgress(int value);
    void setCurrentFile(const QString &value);
private:
    QString m_inputDirectory;
    QString m_outputDirectory;
    QString m_fileMask;
    QString m_xorHexValue;
    QString m_currentFile;

    QTimer m_pollTimer;

    ProcessManager m_processManager;

    int32_t m_pollIntervalMs = 1000;
    int32_t m_currentFileProgress = 0;
    int32_t m_totalProgress = 0;

    bool m_deleteInputFiles {false};
    bool m_overwriteOutputFiles {false};
    bool m_timerMode {false};
    bool m_running = false;
    bool m_paused = false;
};

