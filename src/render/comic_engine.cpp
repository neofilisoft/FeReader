#include "comic_engine.h"
#include <archive.h>
#include <archive_entry.h>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QImageReader>
#include <QPainter>
#include <QCollator>
#include <QSet>
#include <stdexcept>
#include <algorithm>

ComicEngine::ComicEngine() = default;

ComicEngine::~ComicEngine()
{
    close();
}

void ComicEngine::close()
{
    m_pageFiles.clear();
    m_tempDir.reset();
}

int ComicEngine::load(const QString &path)
{
    close();

    m_tempDir = std::make_unique<QTemporaryDir>();
    if (!m_tempDir->isValid()) {
        throw std::runtime_error("Failed to create temporary directory for comic extraction.");
    }

    struct archive *a = archive_read_new();
    if (!a) {
        throw std::runtime_error("Failed to initialize libarchive context.");
    }

    archive_read_support_filter_all(a);
    archive_read_support_format_all(a);

    int r = archive_read_open_filename_w(a, reinterpret_cast<const wchar_t *>(path.utf16()), 10240);
    if (r != ARCHIVE_OK) {
        QString err = QString::fromUtf8(archive_error_string(a));
        archive_read_free(a);
        throw std::runtime_error("Failed to open archive: " + err.toStdString());
    }

    static const QSet<QString> validExts = {
        QStringLiteral("jpg"), QStringLiteral("jpeg"), QStringLiteral("png"),
        QStringLiteral("webp"), QStringLiteral("gif"), QStringLiteral("bmp"),
        QStringLiteral("tiff"), QStringLiteral("tif"), QStringLiteral("avif")
    };

    struct archive_entry *entry = nullptr;
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        if (archive_entry_filetype(entry) == AE_IFDIR) {
            continue;
        }

        const wchar_t *entryPathW = archive_entry_pathname_w(entry);
        QString entryRelPath;
        if (entryPathW) {
            entryRelPath = QString::fromWCharArray(entryPathW);
        } else {
            const char *utf8 = archive_entry_pathname(entry);
            entryRelPath = utf8 ? QString::fromUtf8(utf8) : QString();
        }

        if (entryRelPath.isEmpty()) continue;

        // Skip metadata and hidden system files
        if (entryRelPath.contains(QStringLiteral("__MACOSX")) ||
            entryRelPath.contains(QStringLiteral(".DS_Store")) ||
            entryRelPath.contains(QStringLiteral("Thumbs.db"))) {
            continue;
        }

        QString ext = QFileInfo(entryRelPath).suffix().toLower();
        if (!validExts.contains(ext)) {
            continue;
        }

        // Sanitize path against directory traversal
        entryRelPath.replace(QLatin1Char('\\'), QLatin1Char('/'));
        QStringList parts = entryRelPath.split(QLatin1Char('/'), Qt::SkipEmptyParts);
        QString safeRelPath;
        for (const QString &p : parts) {
            if (p == QStringLiteral("..") || p == QStringLiteral(".")) continue;
            if (!safeRelPath.isEmpty()) safeRelPath.append(QLatin1Char('/'));
            safeRelPath.append(p);
        }
        if (safeRelPath.isEmpty()) continue;

        QString destFilePath = QDir(m_tempDir->path()).filePath(safeRelPath);
        QFileInfo fi(destFilePath);
        QDir().mkpath(fi.absolutePath());

        QFile outFile(destFilePath);
        if (outFile.open(QIODevice::WriteOnly)) {
            const void *buff = nullptr;
            size_t size = 0;
            la_int64_t offset = 0;
            int status = 0;
            while ((status = archive_read_data_block(a, &buff, &size, &offset)) == ARCHIVE_OK) {
                outFile.write(reinterpret_cast<const char *>(buff), static_cast<qint64>(size));
            }
            outFile.close();
            if (status == ARCHIVE_OK || status == ARCHIVE_EOF) {
                m_pageFiles.append(destFilePath);
            }
        }
    }

    archive_read_close(a);
    archive_read_free(a);

    if (m_pageFiles.isEmpty()) {
        close();
        throw std::runtime_error("No readable comic page images found inside the archive.");
    }

    // Natural alphanumeric sort so page 10 comes after page 9
    QCollator collator;
    collator.setNumericMode(true);
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    std::sort(m_pageFiles.begin(), m_pageFiles.end(), [&collator](const QString &f1, const QString &f2) {
        return collator.compare(f1, f2) < 0;
    });

    return m_pageFiles.size();
}

QPixmap ComicEngine::getPagePixmap(int index, double zoom) const
{
    if (index < 0 || index >= m_pageFiles.size()) return QPixmap();
    QImage img(m_pageFiles.at(index));
    if (img.isNull()) return QPixmap();

    if (qFuzzyCompare(zoom, 1.0)) {
        return QPixmap::fromImage(img);
    }

    int targetW = qMax(1, qRound(img.width() * zoom));
    int targetH = qMax(1, qRound(img.height() * zoom));
    img = img.scaled(targetW, targetH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    return QPixmap::fromImage(img);
}

QPixmap ComicEngine::getSpreadPixmap(int leftIndex, double zoom) const
{
    if (leftIndex < 0 || leftIndex >= m_pageFiles.size()) return QPixmap();

    QImage imgLeft(m_pageFiles.at(leftIndex));
    if (imgLeft.isNull()) return QPixmap();

    if (leftIndex + 1 >= m_pageFiles.size()) {
        return getPagePixmap(leftIndex, zoom);
    }

    QImage imgRight(m_pageFiles.at(leftIndex + 1));
    if (imgRight.isNull()) {
        return getPagePixmap(leftIndex, zoom);
    }

    // Normalize right page height to left page height
    int targetH = imgLeft.height();
    if (imgRight.height() != targetH && targetH > 0) {
        imgRight = imgRight.scaledToHeight(targetH, Qt::SmoothTransformation);
    }

    int spreadW = imgLeft.width() + imgRight.width();
    int spreadH = targetH;
    QImage spread(spreadW, spreadH, QImage::Format_ARGB32_Premultiplied);
    spread.fill(Qt::white);

    QPainter painter(&spread);
    painter.drawImage(0, 0, imgLeft);
    painter.drawImage(imgLeft.width(), 0, imgRight);
    painter.end();

    if (qFuzzyCompare(zoom, 1.0)) {
        return QPixmap::fromImage(spread);
    }

    int w = qMax(1, qRound(spread.width() * zoom));
    int h = qMax(1, qRound(spread.height() * zoom));
    spread = spread.scaled(w, h, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    return QPixmap::fromImage(spread);
}

double ComicEngine::getInitialZoom(int viewWidth, int viewHeight) const
{
    if (m_pageFiles.isEmpty() || viewWidth <= 0 || viewHeight <= 0) {
        return 1.0;
    }

    QImageReader reader(m_pageFiles.first());
    QSize sz = reader.size();
    if (!sz.isValid()) {
        QImage img(m_pageFiles.first());
        sz = img.size();
    }

    if (!sz.isValid() || sz.width() <= 0 || sz.height() <= 0) {
        return 1.0;
    }

    double scaleW = static_cast<double>(viewWidth)  / sz.width();
    double scaleH = static_cast<double>(viewHeight) / sz.height();
    double zoom   = qMin(scaleW, scaleH);
    return qBound(0.2, zoom, 3.0);
}
