#include "pdf_editor.h"

extern "C" {
#include <mupdf/fitz.h>
#include <mupdf/pdf.h>
}

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <algorithm>
#include <vector>

PdfEditor::PdfEditor() = default;
PdfEditor::~PdfEditor() = default;

// Helper: safe write that allows inputPath == outputPath
struct SafeOutputHandler {
    QString finalPath;
    QString writePath;
    bool isSame;

    SafeOutputHandler(const QString &inPath, const QString &outPath) {
        finalPath = outPath;
        isSame = (QFileInfo(inPath).canonicalFilePath() == QFileInfo(outPath).canonicalFilePath());
        if (isSame || inPath == outPath) {
            writePath = outPath + QStringLiteral(".tmp.pdf");
        } else {
            writePath = outPath;
        }
    }

    bool commit(QString &err) {
        if (isSame || writePath != finalPath) {
            if (QFile::exists(finalPath)) {
                if (!QFile::remove(finalPath)) {
                    err = QStringLiteral("Failed to replace original file");
                    QFile::remove(writePath);
                    return false;
                }
            }
            if (!QFile::rename(writePath, finalPath)) {
                err = QStringLiteral("Failed to rename temporary file to destination");
                return false;
            }
        }
        return true;
    }

    void rollback() {
        if (writePath != finalPath && QFile::exists(writePath)) {
            QFile::remove(writePath);
        }
    }
};

bool PdfEditor::rotatePage(const QString &inputPath, const QString &outputPath,
                           int pageIndex, int deltaDegrees, QString &errorMessage)
{
    fz_context *ctx = fz_new_context(nullptr, nullptr, FZ_STORE_UNLIMITED);
    if (!ctx) {
        errorMessage = QStringLiteral("Failed to initialize MuPDF context");
        return false;
    }

    fz_register_document_handlers(ctx);

    SafeOutputHandler handler(inputPath, outputPath);
    pdf_document *doc = nullptr;

    fz_try(ctx) {
        doc = pdf_open_document(ctx, inputPath.toUtf8().constData());
        int count = pdf_count_pages(ctx, doc);
        if (pageIndex < 0 || pageIndex >= count) {
            fz_throw(ctx, FZ_ERROR_GENERIC, "Page index out of range");
        }

        pdf_obj *page_obj = pdf_lookup_page_obj(ctx, doc, pageIndex);
        int curRot = pdf_to_int(ctx, pdf_dict_get_inheritable(ctx, page_obj, PDF_NAME(Rotate)));
        int newRot = (curRot + deltaDegrees) % 360;
        if (newRot < 0) newRot += 360;
        pdf_dict_put_int(ctx, page_obj, PDF_NAME(Rotate), newRot);

        pdf_write_options opts = pdf_default_write_options;
        pdf_save_document(ctx, doc, handler.writePath.toUtf8().constData(), &opts);
    } fz_catch(ctx) {
        errorMessage = QString::fromUtf8(fz_caught_message(ctx));
        if (doc) pdf_drop_document(ctx, doc);
        fz_drop_context(ctx);
        handler.rollback();
        return false;
    }

    if (doc) pdf_drop_document(ctx, doc);
    fz_drop_context(ctx);

    return handler.commit(errorMessage);
}

bool PdfEditor::deletePages(const QString &inputPath, const QString &outputPath,
                            const QList<int> &pageIndices, QString &errorMessage)
{
    if (pageIndices.isEmpty()) {
        errorMessage = QStringLiteral("No pages specified for deletion");
        return false;
    }

    fz_context *ctx = fz_new_context(nullptr, nullptr, FZ_STORE_UNLIMITED);
    if (!ctx) {
        errorMessage = QStringLiteral("Failed to initialize MuPDF context");
        return false;
    }

    fz_register_document_handlers(ctx);

    SafeOutputHandler handler(inputPath, outputPath);
    pdf_document *doc = nullptr;

    fz_try(ctx) {
        doc = pdf_open_document(ctx, inputPath.toUtf8().constData());
        int total = pdf_count_pages(ctx, doc);

        // Sort descending to keep earlier indices valid during deletion
        std::vector<int> sorted(pageIndices.begin(), pageIndices.end());
        std::sort(sorted.begin(), sorted.end(), std::greater<int>());

        for (int idx : sorted) {
            if (idx >= 0 && idx < total) {
                pdf_delete_page(ctx, doc, idx);
                total--;
            }
        }

        if (total <= 0) {
            fz_throw(ctx, FZ_ERROR_GENERIC, "Cannot delete all pages in a document");
        }

        pdf_write_options opts = pdf_default_write_options;
        pdf_save_document(ctx, doc, handler.writePath.toUtf8().constData(), &opts);
    } fz_catch(ctx) {
        errorMessage = QString::fromUtf8(fz_caught_message(ctx));
        if (doc) pdf_drop_document(ctx, doc);
        fz_drop_context(ctx);
        handler.rollback();
        return false;
    }

    if (doc) pdf_drop_document(ctx, doc);
    fz_drop_context(ctx);

    return handler.commit(errorMessage);
}

bool PdfEditor::mergePdfs(const QStringList &inputPaths, const QString &outputPath,
                          QString &errorMessage)
{
    if (inputPaths.size() < 2) {
        errorMessage = QStringLiteral("Please select at least 2 PDF files to merge");
        return false;
    }

    fz_context *ctx = fz_new_context(nullptr, nullptr, FZ_STORE_UNLIMITED);
    if (!ctx) {
        errorMessage = QStringLiteral("Failed to initialize MuPDF context");
        return false;
    }

    fz_register_document_handlers(ctx);

    pdf_document *merged = nullptr;

    fz_try(ctx) {
        merged = pdf_create_document(ctx);

        for (const QString &path : inputPaths) {
            if (!QFile::exists(path)) continue;
            pdf_document *src = pdf_open_document(ctx, path.toUtf8().constData());
            int pages = pdf_count_pages(ctx, src);
            for (int p = 0; p < pages; ++p) {
                pdf_graft_page(ctx, merged, -1, src, p);
            }
            pdf_drop_document(ctx, src);
        }

        pdf_write_options opts = pdf_default_write_options;
        pdf_save_document(ctx, merged, outputPath.toUtf8().constData(), &opts);
    } fz_catch(ctx) {
        errorMessage = QString::fromUtf8(fz_caught_message(ctx));
        if (merged) pdf_drop_document(ctx, merged);
        fz_drop_context(ctx);
        return false;
    }

    if (merged) pdf_drop_document(ctx, merged);
    fz_drop_context(ctx);

    return true;
}

bool PdfEditor::extractPages(const QString &inputPath, const QString &outputPath,
                             const QList<int> &pageIndices, QString &errorMessage)
{
    if (pageIndices.isEmpty()) {
        errorMessage = QStringLiteral("No pages specified for extraction");
        return false;
    }

    fz_context *ctx = fz_new_context(nullptr, nullptr, FZ_STORE_UNLIMITED);
    if (!ctx) {
        errorMessage = QStringLiteral("Failed to initialize MuPDF context");
        return false;
    }

    fz_register_document_handlers(ctx);

    pdf_document *src = nullptr;
    pdf_document *dst = nullptr;

    fz_try(ctx) {
        src = pdf_open_document(ctx, inputPath.toUtf8().constData());
        dst = pdf_create_document(ctx);
        int total = pdf_count_pages(ctx, src);

        for (int idx : pageIndices) {
            if (idx >= 0 && idx < total) {
                pdf_graft_page(ctx, dst, -1, src, idx);
            }
        }

        pdf_write_options opts = pdf_default_write_options;
        pdf_save_document(ctx, dst, outputPath.toUtf8().constData(), &opts);
    } fz_catch(ctx) {
        errorMessage = QString::fromUtf8(fz_caught_message(ctx));
        if (dst) pdf_drop_document(ctx, dst);
        if (src) pdf_drop_document(ctx, src);
        fz_drop_context(ctx);
        return false;
    }

    if (dst) pdf_drop_document(ctx, dst);
    if (src) pdf_drop_document(ctx, src);
    fz_drop_context(ctx);

    return true;
}

bool PdfEditor::addTextAnnotation(const QString &inputPath, const QString &outputPath,
                                  int pageIndex, const QRectF &rect,
                                  const QString &text, const QColor &color,
                                  QString &errorMessage)
{
    fz_context *ctx = fz_new_context(nullptr, nullptr, FZ_STORE_UNLIMITED);
    if (!ctx) {
        errorMessage = QStringLiteral("Failed to initialize MuPDF context");
        return false;
    }

    fz_register_document_handlers(ctx);

    SafeOutputHandler handler(inputPath, outputPath);
    pdf_document *doc = nullptr;

    fz_try(ctx) {
        doc = pdf_open_document(ctx, inputPath.toUtf8().constData());
        int count = pdf_count_pages(ctx, doc);
        if (pageIndex < 0 || pageIndex >= count) {
            fz_throw(ctx, FZ_ERROR_GENERIC, "Page index out of range");
        }

        pdf_page *page = pdf_load_page(ctx, doc, pageIndex);
        pdf_annot *annot = pdf_create_annot(ctx, page, PDF_ANNOT_TEXT);

        fz_rect r = fz_make_rect((float)rect.left(), (float)rect.top(),
                                 (float)rect.right(), (float)rect.bottom());
        pdf_set_annot_rect(ctx, annot, r);
        pdf_set_annot_contents(ctx, annot, text.toUtf8().constData());

        float col[3] = { (float)color.redF(), (float)color.greenF(), (float)color.blueF() };
        pdf_set_annot_color(ctx, annot, 3, col);

        pdf_update_annot(ctx, annot);
        pdf_drop_annot(ctx, annot);
        pdf_drop_page(ctx, page);

        pdf_write_options opts = pdf_default_write_options;
        pdf_save_document(ctx, doc, handler.writePath.toUtf8().constData(), &opts);
    } fz_catch(ctx) {
        errorMessage = QString::fromUtf8(fz_caught_message(ctx));
        if (doc) pdf_drop_document(ctx, doc);
        fz_drop_context(ctx);
        handler.rollback();
        return false;
    }

    if (doc) pdf_drop_document(ctx, doc);
    fz_drop_context(ctx);

    return handler.commit(errorMessage);
}
