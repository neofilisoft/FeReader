#pragma once
#include "pdf_engine.h"
#include "epub_engine.h"
#include "comic_engine.h"
#include "render_cache.h"
#include "../core/types.h"
#include <QStringList>
#include <QPixmap>
#include <functional>

// Unified document controller - manages PdfEngine, EpubEngine, ComicEngine, and RenderCache.
class RenderEngine
{
public:
    RenderEngine();
    ~RenderEngine();

    void cleanup();

    // Load PDF; returns page count. Throws on error.
    int loadPdf(const QString &path, std::function<QString()> passwordCallback = nullptr);

    // Load EPUB; returns HTML page list. Throws on error.
    QStringList loadEpub(const QString &path);

    // Load Comic Book archive (.cbz, .cbr, .zip, .rar, etc.); returns page count. Throws on error.
    int loadComic(const QString &path);

    // Current state
    FeReader::BookType  bookType()  const { return m_bookType; }
    const QStringList  &pages()     const { return m_pages; }

    // Page rendering delegates (cached via xxHash)
    QPixmap getPagePixmap(int index, double zoom);
    QPixmap getSpreadPixmap(int leftIndex, double zoom);
    double  getInitialZoom(int viewWidth, int viewHeight) const;

    // Backward compatibility aliases
    QPixmap getPdfPagePixmap(int index, double zoom) { return getPagePixmap(index, zoom); }
    QPixmap getPdfSpreadPixmap(int leftIndex, double zoom) { return getSpreadPixmap(leftIndex, zoom); }

    // Page count convenience
    int pageCount() const;

    // Cache management
    void clearCache() { m_cache.clearDiskCache(); }

private:
    PdfEngine   m_pdf;
    EpubEngine  m_epub;
    ComicEngine m_comic;
    RenderCache m_cache;

    QString            m_currentDocPath;
    qint64             m_docFileSize = 0;
    qint64             m_docModTime  = 0;
    FeReader::BookType m_bookType = FeReader::BookType::None;
    QStringList        m_pages;
};
