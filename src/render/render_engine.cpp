#include "render_engine.h"

RenderEngine::RenderEngine() = default;
RenderEngine::~RenderEngine() { cleanup(); }

void RenderEngine::cleanup()
{
    m_pdf.close();
    m_epub.close();
    m_bookType = FeReader::BookType::None;
    m_pages.clear();
}

int RenderEngine::loadPdf(const QString &path, std::function<QString()> passwordCallback)
{
    cleanup();
    int count = m_pdf.load(path, passwordCallback);
    m_bookType = FeReader::BookType::Pdf;
    // Build a dummy page index list (integer strings) so m_pages.size() works
    m_pages.clear();
    m_pages.reserve(count);
    for (int i = 0; i < count; ++i)
        m_pages << QString::number(i);
    return count;
}

QStringList RenderEngine::loadEpub(const QString &path)
{
    cleanup();
    m_pages    = m_epub.load(path);
    m_bookType = FeReader::BookType::Epub;
    return m_pages;
}

QPixmap RenderEngine::getPdfPagePixmap(int index, double zoom) const
{
    return m_pdf.getPagePixmap(index, zoom);
}

QPixmap RenderEngine::getPdfSpreadPixmap(int leftIndex, double zoom) const
{
    return m_pdf.getSpreadPixmap(leftIndex, zoom);
}

double RenderEngine::getInitialZoom(int viewWidth, int viewHeight) const
{
    return m_pdf.getInitialZoom(viewWidth, viewHeight);
}

int RenderEngine::pageCount() const
{
    return m_pages.size();
}
