#pragma once
#include <QString>
#include <QStringList>
#include <QPixmap>
#include <QTemporaryDir>
#include <memory>

// Native Comic Book Archive reader (.cbz, .cbr, .zip, .rar, .cb7, .cbt) via libarchive.
// Extracts pages to an auto-cleaned temporary directory and provides single and spread page pixmaps.
class ComicEngine
{
public:
    ComicEngine();
    ~ComicEngine();

    // Open and extract comic archive. Returns total image page count.
    // Throws std::runtime_error on failure.
    int load(const QString &path);

    void close();

    int pageCount() const { return m_pageFiles.size(); }
    const QStringList &pageFiles() const { return m_pageFiles; }

    // Render single comic page to QPixmap at given zoom level.
    QPixmap getPagePixmap(int index, double zoom) const;

    // Render two comic pages side-by-side (dual-page spread).
    QPixmap getSpreadPixmap(int leftIndex, double zoom) const;

    // Calculate initial zoom to fit page comfortably into the viewport.
    double getInitialZoom(int viewWidth, int viewHeight) const;

private:
    std::unique_ptr<QTemporaryDir> m_tempDir;
    QStringList                    m_pageFiles;
};
