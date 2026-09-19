#include "render_engine.h"
#include <QFileInfo>
#include <QDateTime>

RenderEngine::RenderEngine() = default;
RenderEngine::~RenderEngine() { cleanup(); }

void RenderEngine::cleanup()
{
    m_pdf.close();
    m_epub.close();
    m_comic.close();
    m_currentDocPath.clear();
    m_docFileSize = 0;
    m_docModTime  = 0;
    m_bookType    = FeReader::BookType::None;
    m_pages.clear();
}

int RenderEngine::loadPdf(const QString &path, std::function<QString()> passwordCallback)
{
    cleanup();
    QFileInfo fi(path);
    m_currentDocPath = fi.absoluteFilePath();
    m_docFileSize    = fi.size();
    m_docModTime     = fi.lastModified().toMSecsSinceEpoch();

    int count = m_pdf.load(path, passwordCallback);
    m_bookType = FeReader::BookType::Pdf;
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

int RenderEngine::loadComic(const QString &path)
{
    cleanup();
    QFileInfo fi(path);
    m_currentDocPath = fi.absoluteFilePath();
    m_docFileSize    = fi.size();
    m_docModTime     = fi.lastModified().toMSecsSinceEpoch();

    int count = m_comic.load(path);
    m_bookType = FeReader::BookType::Comic;
    m_pages.clear();
    m_pages.reserve(count);
    for (int i = 0; i < count; ++i)
        m_pages << QString::number(i);
    return count;
}

QPixmap RenderEngine::getPagePixmap(int index, double zoom)
{
    QPixmap pix;
    if (m_cache.get(m_currentDocPath, m_docFileSize, m_docModTime, index, zoom, false, pix)) {
        return pix;
    }
    if (m_bookType == FeReader::BookType::Pdf) {
        pix = m_pdf.getPagePixmap(index, zoom);
    } else if (m_bookType == FeReader::BookType::Comic) {
        pix = m_comic.getPagePixmap(index, zoom);
    }
    if (!pix.isNull()) {
        m_cache.put(m_currentDocPath, m_docFileSize, m_docModTime, index, zoom, false, pix);
    }
    return pix;
}

QPixmap RenderEngine::getSpreadPixmap(int leftIndex, double zoom)
{
    QPixmap pix;
    if (m_cache.get(m_currentDocPath, m_docFileSize, m_docModTime, leftIndex, zoom, true, pix)) {
        return pix;
    }
    if (m_bookType == FeReader::BookType::Pdf) {
        pix = m_pdf.getSpreadPixmap(leftIndex, zoom);
    } else if (m_bookType == FeReader::BookType::Comic) {
        pix = m_comic.getSpreadPixmap(leftIndex, zoom);
    }
    if (!pix.isNull()) {
        m_cache.put(m_currentDocPath, m_docFileSize, m_docModTime, leftIndex, zoom, true, pix);
    }
    return pix;
}

double RenderEngine::getInitialZoom(int viewWidth, int viewHeight) const
{
    if (m_bookType == FeReader::BookType::Pdf) {
        return m_pdf.getInitialZoom(viewWidth, viewHeight);
    } else if (m_bookType == FeReader::BookType::Comic) {
        return m_comic.getInitialZoom(viewWidth, viewHeight);
    }
    return 1.0;
}

int RenderEngine::pageCount() const
{
    return m_pages.size();
}
