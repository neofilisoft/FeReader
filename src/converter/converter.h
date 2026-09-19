#pragma once
#include <QString>
#include <QStringList>

// File format conversion logic - mirrors Python ConverterLogic class.
namespace Converter {

// Convert a plain text file to a PDF document.
// Optionally password-protect with AES encryption.
void textToPdf(const QString &inputPath, const QString &outputPath,
               const QString &password = QString());

// Convert a plain text file to a valid EPUB package using libzip.
void textToEpub(const QString &inputPath, const QString &outputPath);

// Combine image files into a single PDF document.
// Optionally password-protect with AES encryption.
void imagesToPdf(const QStringList &inputPaths, const QString &outputPath,
                 const QString &password = QString());

} // namespace Converter
