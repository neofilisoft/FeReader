#pragma once
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <memory>

// Custom EPUB reader engine using libzip (extraction) + gumbo (HTML parsing).
// Replaces Python's ebooklib + BeautifulSoup.
class EpubEngine
{
public:
    EpubEngine();
    ~EpubEngine();

    // Open an EPUB file. Returns list of processed HTML strings (one per spine item).
    // Throws std::runtime_error on failure.
    QStringList load(const QString &path);

    void close();

    // Temp dir path used for extracted assets (valid after load())
    QString tempDir() const;

private:
    // Step 1: extract all zip entries to temp dir
    bool extractZip(const QString &zipPath, const QString &outDir, QString &error);

    // Step 2: parse OPF to get spine item order -> relative XHTML paths
    QStringList parseSpineOrder(const QString &tempDir);

    // Step 3: parse each HTML file with gumbo, rewrite img src, return HTML string
    QString processHtml(const QString &htmlFilePath, const QString &htmlRelDir,
                        const QString &tempDir);

    std::unique_ptr<QTemporaryDir> m_tempDir;
};
