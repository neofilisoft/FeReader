#pragma once
#include "pdf_engine.h"
#include "epub_engine.h"
#include "../core/types.h"
#include <QStringList>
#include <QPixmap>
#include <functional>

// Unified document controller - manages PdfEngine and EpubEngine.
// Mirrors the Python RenderEngine class.
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

    // Current state
    FeReader::BookType  bookType()  const { return m_bookType; }
    const QStringList  &pages()     const { return m_pages; }

    // PDF rendering delegates
    QPixmap getPdfPagePixmap(int index, double zoom) const;
    QPixmap getPdfSpreadPixmap(int leftIndex, double zoom) const;
    double  getInitialZoom(int viewWidth, int viewHeight) const;

    // PDF page count convenience
    int pageCount() const;

private:
    PdfEngine  m_pdf;
    EpubEngine m_epub;

    FeReader::BookType m_bookType = FeReader::BookType::None;
    QStringList        m_pages;
};
