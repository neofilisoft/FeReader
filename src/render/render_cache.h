#pragma once

#include <QString>
#include <QPixmap>
#include <QCache>
#include <cstdint>

// High-performance two-tier (Memory L1 + Disk L2) render cache
// using xxHash (XXH3_64bits) for near-instantaneous key generation.
// No hardcoded paths - uses dynamic QStandardPaths / QDir::tempPath.
class RenderCache
{
public:
    RenderCache();
    ~RenderCache();

    // Compute ultra-fast 64-bit cache key using XXH3
    static uint64_t hashKey(const QString &filePath, qint64 fileSize, qint64 modTime,
                            int pageIndex, double zoom, bool isSpread);

    // Retrieve rendered pixmap from L1 (memory) or L2 (disk)
    bool get(const QString &filePath, qint64 fileSize, qint64 modTime,
             int pageIndex, double zoom, bool isSpread, QPixmap &outPixmap);

    // Store rendered pixmap into L1 and L2
    void put(const QString &filePath, qint64 fileSize, qint64 modTime,
             int pageIndex, double zoom, bool isSpread, const QPixmap &pixmap);

    // Clear all cached files on disk
    void clearDiskCache();

    // Dynamically retrieved cache directory (no hardcoded path)
    QString cacheDir() const;

private:
    void ensureCacheDirExists();
    void pruneDiskCache();

    mutable QCache<QString, QPixmap> m_memCache; // L1: fast RAM cache (max 30 pages)
    QString m_diskCacheDir;
    qint64  m_maxDiskBytes = 256 * 1024 * 1024;  // 256 MB disk limit
};
