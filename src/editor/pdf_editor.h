#pragma once

#include <QString>
#include <QStringList>
#include <QList>
#include <QRectF>
#include <QColor>

// High-performance PDF editor module using MuPDF C API.
// Strictly operates with dynamic paths (no hardcoded paths).
class PdfEditor
{
public:
    PdfEditor();
    ~PdfEditor();

    // Rotate page by 90, 180, or 270 degrees
    bool rotatePage(const QString &inputPath, const QString &outputPath,
                    int pageIndex, int deltaDegrees, QString &errorMessage);

    // Delete specified pages (0-indexed)
    bool deletePages(const QString &inputPath, const QString &outputPath,
                     const QList<int> &pageIndices, QString &errorMessage);

    // Merge multiple PDF files in order into a single output PDF
    bool mergePdfs(const QStringList &inputPaths, const QString &outputPath,
                   QString &errorMessage);

    // Extract / split selected pages (0-indexed) into a new PDF
    bool extractPages(const QString &inputPath, const QString &outputPath,
                      const QList<int> &pageIndices, QString &errorMessage);

    // Add a text note / annotation to a page
    bool addTextAnnotation(const QString &inputPath, const QString &outputPath,
                           int pageIndex, const QRectF &rect,
                           const QString &text, const QColor &color,
                           QString &errorMessage);
};
