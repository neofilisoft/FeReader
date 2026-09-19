#include "pdf_engine.h"

extern "C" {
#include <mupdf/fitz.h>
}

#include <QImage>
#include <QPixmap>
#include <QPainter>
#include <stdexcept>

PdfEngine::PdfEngine() = default;

PdfEngine::~PdfEngine()
{
    close();
}

void PdfEngine::close()
{
    if (m_doc) {
        fz_drop_document(m_ctx, m_doc);
        m_doc = nullptr;
    }
    if (m_ctx) {
        fz_drop_context(m_ctx);
        m_ctx = nullptr;
    }
    m_pageCount = 0;
}

int PdfEngine::load(const QString &path, std::function<QString()> passwordCallback)
{
    close();

    m_ctx = fz_new_context(nullptr, nullptr, FZ_STORE_UNLIMITED);
    if (!m_ctx)
        throw std::runtime_error("Failed to create MuPDF context");

    fz_register_document_handlers(m_ctx);

    fz_try(m_ctx) {
        m_doc = fz_open_document(m_ctx, path.toUtf8().constData());
    } fz_catch(m_ctx) {
        fz_drop_context(m_ctx);
        m_ctx = nullptr;
        throw std::runtime_error(fz_caught_message(m_ctx ? m_ctx : nullptr));
    }

    // Handle encrypted documents
    if (fz_needs_password(m_ctx, m_doc)) {
        if (!passwordCallback) {
            close();
            throw std::runtime_error("Password required");
        }
        QString pw = passwordCallback();
        if (pw.isEmpty() || !fz_authenticate_password(m_ctx, m_doc, pw.toUtf8().constData())) {
            close();
            throw std::runtime_error("Password required or incorrect");
        }
    }

    m_pageCount = fz_count_pages(m_ctx, m_doc);
    return m_pageCount;
}

// ---------------------------------------------------------------------------
// Internal helper: render one page at zoom to QPixmap
// ---------------------------------------------------------------------------
static QPixmap renderPage(fz_context *ctx, fz_document *doc, int index, double zoom)
{
    if (!ctx || !doc || index < 0 || index >= fz_count_pages(ctx, doc))
        return {};

    fz_matrix mat = fz_scale((float)zoom, (float)zoom);
    fz_pixmap *pix = nullptr;

    fz_try(ctx) {
        pix = fz_new_pixmap_from_page_number(ctx, doc, index, mat, fz_device_rgb(ctx), 1);
    } fz_catch(ctx) {
        if (pix) fz_drop_pixmap(ctx, pix);
        return {};
    }

    if (!pix) return {};

    int w = fz_pixmap_width(ctx, pix);
    int h = fz_pixmap_height(ctx, pix);
    int stride = fz_pixmap_stride(ctx, pix);
    unsigned char *samples = fz_pixmap_samples(ctx, pix);

    // MuPDF RGBA -> Qt RGBA8888
    QImage img(samples, w, h, stride, QImage::Format_RGBA8888);
    QPixmap result = QPixmap::fromImage(img.copy());

    fz_drop_pixmap(ctx, pix);
    return result;
}

QPixmap PdfEngine::getPagePixmap(int index, double zoom) const
{
    double z = qMax(0.1, qMin(5.0, zoom));
    return renderPage(m_ctx, m_doc, index, z);
}

QPixmap PdfEngine::getSpreadPixmap(int leftIndex, double zoom) const
{
    double z = qMax(0.1, qMin(5.0, zoom));
    QPixmap left = renderPage(m_ctx, m_doc, leftIndex, z);
    if (left.isNull())
        return {};

    QPixmap right = renderPage(m_ctx, m_doc, leftIndex + 1, z);
    if (right.isNull())
        return left;

    int targetH = qMax(left.height(), right.height());
    QPixmap lScaled = left.scaledToHeight(targetH, Qt::SmoothTransformation);
    QPixmap rScaled = right.scaledToHeight(targetH, Qt::SmoothTransformation);

    QPixmap spread(lScaled.width() + rScaled.width(), targetH);
    spread.fill(Qt::transparent);
    QPainter painter(&spread);
    painter.drawPixmap(0, 0, lScaled);
    painter.drawPixmap(lScaled.width(), 0, rScaled);
    painter.end();
    return spread;
}

double PdfEngine::getInitialZoom(int viewWidth, int viewHeight) const
{
    if (!m_doc || m_pageCount == 0)
        return 1.0;

    fz_page *page = fz_load_page(m_ctx, m_doc, 0);
    fz_rect rect  = fz_bound_page(m_ctx, page);
    fz_drop_page(m_ctx, page);

    double pw = rect.x1 - rect.x0;
    double ph = rect.y1 - rect.y0;

    if (pw <= 0.0 || ph <= 0.0)
        return 1.0;

    if (viewWidth <= 50 || viewHeight <= 50)
        return 1.0;

    double zw = (double)viewWidth  / pw;
    double zh = (double)viewHeight / ph;
    double z  = qMin(zw, zh);
    return qBound(0.1, qRound(z * 100.0) / 100.0, 5.0);
}
