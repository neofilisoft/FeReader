#include "render_cache.h"

#define XXH_INLINE_ALL
#include "thirdparty/xxHash/xxhash.h"

#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QDataStream>
#include <QByteArray>

RenderCache::RenderCache()
{
    // L1 cache: up to 30 pixmaps in memory
    m_memCache.setMaxCost(30);
    m_diskCacheDir = cacheDir();
    ensureCacheDirExists();
}

RenderCache::~RenderCache() = default;

QString RenderCache::cacheDir() const
{
    if (!m_diskCacheDir.isEmpty())
        return m_diskCacheDir;

    QString base = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (base.isEmpty()) {
        base = QDir::cleanPath(QDir::tempPath() + QStringLiteral("/FeReader_cache"));
    } else {
        base = QDir::cleanPath(base + QStringLiteral("/page_cache"));
    }
    return base;
}

void RenderCache::ensureCacheDirExists()
{
    QDir dir(cacheDir());
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }
}

uint64_t RenderCache::hashKey(const QString &filePath, qint64 fileSize, qint64 modTime,
                              int pageIndex, double zoom, bool isSpread)
{
    QByteArray buf;
    QDataStream stream(&buf, QIODevice::WriteOnly);
    // Integer-scaled zoom to prevent subtle floating-point precision differences
    int zoomScaled = qRound(zoom * 1000.0);
    stream << filePath << fileSize << modTime << pageIndex << zoomScaled << (quint8)(isSpread ? 1 : 0);

    return XXH3_64bits(buf.constData(), (size_t)buf.size());
}

bool RenderCache::get(const QString &filePath, qint64 fileSize, qint64 modTime,
                      int pageIndex, double zoom, bool isSpread, QPixmap &outPixmap)
{
    if (filePath.isEmpty()) return false;

    uint64_t key = hashKey(filePath, fileSize, modTime, pageIndex, zoom, isSpread);
    QString keyStr = QString::number(key, 16).rightJustified(16, QLatin1Char('0'));

    // 1. Check L1 memory cache (0 ms)
    if (QPixmap *cached = m_memCache.object(keyStr)) {
        outPixmap = *cached;
        return true;
    }

    // 2. Check L2 disk cache
    QString path = QDir(cacheDir()).filePath(keyStr + QStringLiteral(".png"));
    if (QFile::exists(path)) {
        if (outPixmap.load(path, "PNG")) {
            // Promote to L1 memory cache
            m_memCache.insert(keyStr, new QPixmap(outPixmap));
            return true;
        }
    }

    return false;
}

void RenderCache::put(const QString &filePath, qint64 fileSize, qint64 modTime,
                      int pageIndex, double zoom, bool isSpread, const QPixmap &pixmap)
{
    if (filePath.isEmpty() || pixmap.isNull()) return;

    uint64_t key = hashKey(filePath, fileSize, modTime, pageIndex, zoom, isSpread);
    QString keyStr = QString::number(key, 16).rightJustified(16, QLatin1Char('0'));

    // 1. Store into L1 memory cache
    m_memCache.insert(keyStr, new QPixmap(pixmap));

    // 2. Store into L2 disk cache asynchronously or directly
    ensureCacheDirExists();
    QString path = QDir(cacheDir()).filePath(keyStr + QStringLiteral(".png"));
    pixmap.save(path, "PNG");

    pruneDiskCache();
}

void RenderCache::pruneDiskCache()
{
    QDir dir(cacheDir());
    const auto entries = dir.entryInfoList({ QStringLiteral("*.png") }, QDir::Files, QDir::Time);

    qint64 totalBytes = 0;
    for (const QFileInfo &fi : entries) {
        totalBytes += fi.size();
    }

    // If total exceeds max allowed, remove oldest files until within 75% limit
    if (totalBytes > m_maxDiskBytes) {
        qint64 targetBytes = (m_maxDiskBytes * 3) / 4;
        // entries sorted by Time (most recent first in default or reversed)
        // iterate from the end (oldest)
        for (int i = entries.size() - 1; i >= 0 && totalBytes > targetBytes; --i) {
            totalBytes -= entries[i].size();
            QFile::remove(entries[i].absoluteFilePath());
        }
    }
}

void RenderCache::clearDiskCache()
{
    m_memCache.clear();
    QDir dir(cacheDir());
    const auto entries = dir.entryInfoList({ QStringLiteral("*.png") }, QDir::Files);
    for (const QFileInfo &fi : entries) {
        QFile::remove(fi.absoluteFilePath());
    }
}
