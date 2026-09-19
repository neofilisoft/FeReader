#pragma once
#include <QString>
#include <QPixmap>
#include <functional>

// Wraps MuPDF fz_context + fz_document for PDF rendering.
// Provides page pixmap rendering with zoom, dual-page spread, and password auth.
class PdfEngine
{
public:
    PdfEngine();
    ~PdfEngine();

    // Load a PDF, optionally calling passwordCallback for encrypted docs.
    // Returns page count, or throws std::runtime_error on failure.
    int  load(const QString &path, std::function<QString()> passwordCallback = nullptr);

    void close();

    int pageCount() const { return m_pageCount; }

    // Render single page to QPixmap at given zoom factor.
    QPixmap getPagePixmap(int index, double zoom) const;

    // Render two pages side-by-side (horizontal spread).
    QPixmap getSpreadPixmap(int leftIndex, double zoom) const;

    // Calculate initial zoom to fit first page in given viewport.
    double  getInitialZoom(int viewWidth, int viewHeight) const;

private:
    struct fz_context *m_ctx    = nullptr;
    struct fz_document *m_doc   = nullptr;
    int                 m_pageCount = 0;
};
