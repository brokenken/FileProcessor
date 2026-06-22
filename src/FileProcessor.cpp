#include "FileProcessor.h"

#include <QFileInfo>
#include <QSaveFile>

#include <utility>

FileProcessor::FileProcessor(
    QString inputPath, QString outputPath, const quint64 xorValue, bool deleteFile, QObject* parent) :
    QObject(parent), m_inputPath(std::move(inputPath)), m_outputPath(std::move(outputPath)), m_deleteFile(deleteFile)
{
    saveToByteArray(xorValue);
}

void FileProcessor::processFile()
{
    clear();

    emit started();
    emit statusChanged(QStringLiteral("Opening file"));

    QFile inputFile(m_inputPath);
    if (!inputFile.open(QIODevice::ReadOnly)) {
        emit errorDuringProcessing(QStringLiteral("Input File could not be opened: %1").arg(inputFile.errorString()));
        emit finished(false, QStringLiteral("Input File could not be opened"));
        return;
    }

    QSaveFile outputFile(m_outputPath);
    if (!outputFile.open(QIODevice::WriteOnly)) {
        emit errorDuringProcessing(QStringLiteral("Output File could not be opened: %1").arg(outputFile.errorString()));
        emit finished(false, QStringLiteral("Output File could not be opened"));
        return;
    }
    emit statusChanged(QStringLiteral("Processing file"));
    const qint64 fileSize = inputFile.size();
    qint64 bytesWritten = 0;
    int32_t lastPercentage = 0;
    while (!inputFile.atEnd()) {
        {
            QMutexLocker locker(&m_pauseMutex);
            while (m_pauseRequested.load() && !m_stopRequested.load()) {
                emit statusChanged(QStringLiteral("Processing pause by user"));
                m_pauseCondition.wait(&m_pauseMutex);
            }
        }
        if (m_stopRequested.load()) {
            emit statusChanged(QStringLiteral("Processing stoping"));
            outputFile.cancelWriting();
            emit finished(false, QStringLiteral("Processing stop by user"));
            return;
        }
        QByteArray bytes = inputFile.read(m_bufferSize);
        if (bytes.isEmpty() && inputFile.error() != QFile::NoError) {
            outputFile.cancelWriting();
            emit errorDuringProcessing(QStringLiteral("Error while reading from file: %1").arg(inputFile.errorString()));
            emit finished(false, QStringLiteral("Error while reading file"));
            return;
        }
        applyOperation(bytes);
        const qint64 written = outputFile.write(bytes);
        if (written != bytes.size()) {
            outputFile.cancelWriting();
            emit errorDuringProcessing(QStringLiteral("Error while writing in file: %1").arg(outputFile.errorString()));
            emit finished(false, QStringLiteral("Error while writing in file"));
            return;
        }
        bytesWritten += written;
        const auto percentage = static_cast<int32_t>((100 * bytesWritten) / fileSize);
        if (percentage > lastPercentage) {
            lastPercentage = percentage;
            emit progressChanged(percentage);
        }
    }

    if (!outputFile.commit()) {
        emit errorDuringProcessing(QStringLiteral("Error while saving output file: %1").arg(outputFile.errorString()));
        emit finished(false, QStringLiteral("Error while saving output file"));
        return;
    }

    inputFile.close();
    if (m_deleteFile && !inputFile.remove()) {
        emit errorDuringProcessing(QStringLiteral("Cannot delete input file: %1").arg(inputFile.errorString()));
        emit finished(false, QStringLiteral("Cannot delete input file"));
        return;
    }

    emit progressChanged(100);
    emit statusChanged(QStringLiteral("Ended"));
    emit finished(true, QStringLiteral("File processed successfully"));
}

void FileProcessor::pause()
{
    QMutexLocker locker(&m_pauseMutex);
    m_pauseRequested.store(true);
}

void FileProcessor::resume()
{
    {
        QMutexLocker locker(&m_pauseMutex);
        m_pauseRequested.store(false);
    }
    m_pauseCondition.wakeAll();
}

void FileProcessor::stop()
{
    m_stopRequested.store(true);
    resume();
}

void FileProcessor::applyOperation(QByteArray& arr)
{
    for (auto& i : arr) {
        i = static_cast<char>(i ^ m_byteArray[m_currIdx]);
        nextIdx();
    }
}

void FileProcessor::saveToByteArray(const quint64 xorValue)
{
    m_byteArray.resize(8);
    for (qsizetype i = 0; i < m_byteArray.size(); ++i) {
        const int64_t shift = 56LL - i * 8LL;
        m_byteArray[i] = static_cast<char>((xorValue >> shift) & 0xFF);
    }
}

void FileProcessor::nextIdx()
{
    m_currIdx = (m_currIdx + 1) % m_byteArray.size();
}

void FileProcessor::clear()
{
    m_stopRequested = false;
    m_pauseRequested = false;
    m_currIdx = 0;

}
