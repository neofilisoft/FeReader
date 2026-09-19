#include "converter.h"

extern "C" {
#include <mupdf/fitz.h>
#include <mupdf/pdf.h>
}

#include <zip.h>

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QByteArray>
#include <QDir>
#include <stdexcept>
#include <string>
#include <cstring>

// ---------------------------------------------------------------------------
// Internal: save a pdf_document (optionally with password)
// ---------------------------------------------------------------------------
static void savePdfDoc(fz_context *ctx, pdf_document *doc,
                       const QString &outputPath, const QString &password)
{
    pdf_write_options opts = pdf_default_write_options;
    std::string pwStd;

    if (!password.isEmpty()) {
        pwStd = password.toStdString();
        opts.do_encrypt  = PDF_ENCRYPT_AES_128;
        opts.permissions = PDF_PERM_PRINT | PDF_PERM_COPY;
        strncpy(opts.opwd_utf8, pwStd.c_str(), sizeof(opts.opwd_utf8) - 1);
        strncpy(opts.upwd_utf8, pwStd.c_str(), sizeof(opts.upwd_utf8) - 1);
    }

    fz_try(ctx) {
        pdf_save_document(ctx, doc, outputPath.toUtf8().constData(), &opts);
    } fz_catch(ctx) {
        throw std::runtime_error(fz_caught_message(ctx));
    }
}

// ---------------------------------------------------------------------------
// Internal helper: add one page of text to a pdf_document
// ---------------------------------------------------------------------------
static void addTextPage(fz_context *ctx, pdf_document *doc,
                        const QStringList &lines, int startLine, int endLine)
{
    const float pageW  = 595.0f, pageH = 842.0f;
    const float margin = 50.0f,  lineH = 14.0f, fontSize = 11.0f;

    // Build content stream as PDF syntax
    std::string stream;
    stream += "BT\n";
    stream += "/F1 11 Tf\n";

    float y = pageH - margin - fontSize;
    for (int li = startLine; li < endLine; ++li) {
        // Position each line
        char pos[64];
        if (li == startLine)
            snprintf(pos, sizeof(pos), "%.1f %.1f Td\n", margin, y);
        else
            snprintf(pos, sizeof(pos), "0 %.1f Td\n", -lineH);
        stream += pos;

        // Escape parentheses and backslash
        std::string lineStr = lines[li].toUtf8().constData();
        std::string escaped;
        for (char c : lineStr) {
            if (c == '(' || c == ')' || c == '\\') escaped += '\\';
            escaped += c;
        }
        stream += "(" + escaped + ") Tj\n";
    }
    stream += "ET\n";

    fz_buffer *contents = fz_new_buffer(ctx, (size_t)stream.size() + 16);
    fz_append_string(ctx, contents, stream.c_str());

    // Build minimal resources with base-14 Helvetica
    pdf_obj *resources = pdf_new_dict(ctx, doc, 1);
    pdf_obj *fonts     = pdf_new_dict(ctx, doc, 1);
    fz_font *helv      = fz_new_base14_font(ctx, "Helvetica");
    pdf_obj *fontRef   = pdf_add_simple_font(ctx, doc, helv, PDF_SIMPLE_ENCODING_LATIN);
    fz_drop_font(ctx, helv);
    pdf_dict_puts(ctx, fonts, "F1", fontRef);
    pdf_drop_obj(ctx, fontRef);
    pdf_dict_puts(ctx, resources, "Font", fonts);
    pdf_drop_obj(ctx, fonts);

    // Create and insert page
    fz_rect mediabox = fz_make_rect(0, 0, pageW, pageH);
    pdf_obj *page = pdf_add_page(ctx, doc, mediabox, 0, resources, contents);
    pdf_drop_obj(ctx, resources);
    fz_drop_buffer(ctx, contents);

    pdf_insert_page(ctx, doc, -1, page); // -1 = append at end
    pdf_drop_obj(ctx, page);
}

// ---------------------------------------------------------------------------
// textToPdf
// ---------------------------------------------------------------------------
void Converter::textToPdf(const QString &inputPath, const QString &outputPath,
                           const QString &password)
{
    QFile f(inputPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        throw std::runtime_error("Cannot open input file");
    QString text = QTextStream(&f).readAll();
    f.close();

    fz_context *ctx = fz_new_context(nullptr, nullptr, FZ_STORE_UNLIMITED);
    if (!ctx) throw std::runtime_error("MuPDF context failed");

    pdf_document *doc = pdf_create_document(ctx);

    const float pageH   = 842.0f;
    const float margin  = 50.0f;
    const float lineH   = 14.0f;
    const int   linesPerPage = (int)((pageH - margin * 2) / lineH);
    const int   wrapWidth    = 95;

    // Word-wrap
    QStringList rawLines = text.split('\n');
    QStringList wrapped;
    for (const QString &line : rawLines) {
        if (line.length() <= wrapWidth) {
            wrapped << line;
        } else {
            QStringList words = line.split(' ');
            QString cur;
            for (const QString &w : words) {
                QString candidate = cur.isEmpty() ? w : (cur + " " + w);
                if (candidate.length() > wrapWidth && !cur.isEmpty()) {
                    wrapped << cur;
                    cur = w;
                } else {
                    cur = candidate;
                }
            }
            if (!cur.isEmpty()) wrapped << cur;
        }
    }
    if (wrapped.isEmpty()) wrapped << QString();

    fz_try(ctx) {
        int total = (wrapped.size() + linesPerPage - 1) / linesPerPage;
        for (int p = 0; p < total; ++p) {
            int start = p * linesPerPage;
            int end   = qMin(start + linesPerPage, (int)wrapped.size());
            addTextPage(ctx, doc, wrapped, start, end);
        }
        savePdfDoc(ctx, doc, outputPath, password);
    } fz_catch(ctx) {
        pdf_drop_document(ctx, doc);
        fz_drop_context(ctx);
        throw std::runtime_error(fz_caught_message(ctx));
    }

    pdf_drop_document(ctx, doc);
    fz_drop_context(ctx);
}

// ---------------------------------------------------------------------------
// textToEpub - build a minimal valid EPUB 2 archive using libzip
// ---------------------------------------------------------------------------
static void zipAddStr(zip_t *za, const char *entryName, const std::string &data)
{
    zip_source_t *src = zip_source_buffer_create(data.c_str(), data.size(), 0, nullptr);
    if (!src) return;
    zip_file_add(za, entryName, src, ZIP_FL_OVERWRITE | ZIP_FL_ENC_UTF_8);
}

void Converter::textToEpub(const QString &inputPath, const QString &outputPath)
{
    QFile f(inputPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        throw std::runtime_error("Cannot open input file");
    QString text = QTextStream(&f).readAll();
    f.close();

    QString title = QFileInfo(inputPath).completeBaseName();
    std::string titleStd = title.toHtmlEscaped().toStdString();

    QString escaped = text.toHtmlEscaped();
    escaped.replace("\n", "<br/>\n");
    std::string escapedStd = escaped.toStdString();

    std::string mimetype      = "application/epub+zip";
    std::string container_xml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<container version=\"1.0\" xmlns=\"urn:oasis:names:tc:opendocument:xmlns:container\">"
        "<rootfiles>"
        "<rootfile full-path=\"OEBPS/content.opf\" media-type=\"application/oebps-package+xml\"/>"
        "</rootfiles></container>";

    std::string content_opf =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<package version=\"2.0\" xmlns=\"http://www.idpf.org/2007/opf\" unique-identifier=\"uid\">"
        "<metadata xmlns:dc=\"http://purl.org/dc/elements/1.1/\">"
        "<dc:title>" + titleStd + "</dc:title>"
        "<dc:language>en</dc:language>"
        "<dc:identifier id=\"uid\">fereader-convert</dc:identifier>"
        "</metadata>"
        "<manifest>"
        "<item id=\"chap1\" href=\"chap_1.xhtml\" media-type=\"application/xhtml+xml\"/>"
        "<item id=\"ncx\"   href=\"toc.ncx\"      media-type=\"application/x-dtbncx+xml\"/>"
        "</manifest>"
        "<spine toc=\"ncx\"><itemref idref=\"chap1\"/></spine>"
        "</package>";

    std::string toc_ncx =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<!DOCTYPE ncx PUBLIC \"-//NISO//DTD ncx 2005-1//EN\" "
        "\"http://www.daisy.org/z3986/2005/ncx-2005-1.dtd\">"
        "<ncx xmlns=\"http://www.daisy.org/z3986/2005/ncx/\" version=\"2005-1\">"
        "<head><meta name=\"dtb:uid\" content=\"fereader-convert\"/></head>"
        "<docTitle><text>" + titleStd + "</text></docTitle>"
        "<navMap><navPoint id=\"navpoint-1\" playOrder=\"1\">"
        "<navLabel><text>Chapter 1</text></navLabel>"
        "<content src=\"chap_1.xhtml\"/>"
        "</navPoint></navMap></ncx>";

    std::string chap_xhtml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\" "
        "\"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">"
        "<html xmlns=\"http://www.w3.org/1999/xhtml\">"
        "<head><title>" + titleStd + "</title></head>"
        "<body><pre>" + escapedStd + "</pre></body></html>";

    int err = 0;
    zip_t *za = zip_open(outputPath.toUtf8().constData(), ZIP_CREATE | ZIP_TRUNCATE, &err);
    if (!za) throw std::runtime_error("Cannot create EPUB file");

    // mimetype: first entry, uncompressed (EPUB spec)
    {
        zip_source_t *src = zip_source_buffer_create(mimetype.c_str(), mimetype.size(), 0, nullptr);
        zip_int64_t idx = zip_file_add(za, "mimetype", src, ZIP_FL_OVERWRITE);
        zip_set_file_compression(za, idx, ZIP_CM_STORE, 0);
    }

    zipAddStr(za, "META-INF/container.xml", container_xml);
    zipAddStr(za, "OEBPS/content.opf",      content_opf);
    zipAddStr(za, "OEBPS/toc.ncx",          toc_ncx);
    zipAddStr(za, "OEBPS/chap_1.xhtml",     chap_xhtml);

    if (zip_close(za) != 0)
        throw std::runtime_error("Failed to write EPUB archive");
}

// ---------------------------------------------------------------------------
// imagesToPdf
// ---------------------------------------------------------------------------
void Converter::imagesToPdf(const QStringList &inputPaths, const QString &outputPath,
                             const QString &password)
{
    if (inputPaths.isEmpty())
        throw std::runtime_error("No input images");

    fz_context *ctx = fz_new_context(nullptr, nullptr, FZ_STORE_UNLIMITED);
    if (!ctx) throw std::runtime_error("MuPDF context failed");
    fz_register_document_handlers(ctx);

    pdf_document *doc = pdf_create_document(ctx);

    fz_try(ctx) {
        for (const QString &imgPath : inputPaths) {
            fz_image *img = nullptr;
            fz_try(ctx) {
                img = fz_new_image_from_file(ctx, imgPath.toUtf8().constData());
            } fz_catch(ctx) {
                continue; // skip unreadable images
            }

            int w = img->w;
            int h = img->h;

            // Build content stream: map image to full page
            char cmd[128];
            snprintf(cmd, sizeof(cmd), "q %d 0 0 %d 0 0 cm /Im1 Do Q\n", w, h);
            fz_buffer *contents = fz_new_buffer(ctx, 128);
            fz_append_string(ctx, contents, cmd);

            // Build resources with the image
            pdf_obj *resources = pdf_new_dict(ctx, doc, 1);
            pdf_obj *xobjs     = pdf_new_dict(ctx, doc, 1);
            pdf_obj *imgRef    = pdf_add_image(ctx, doc, img);
            fz_drop_image(ctx, img);
            pdf_dict_puts(ctx, xobjs, "Im1", imgRef);
            pdf_drop_obj(ctx, imgRef);
            pdf_dict_puts(ctx, resources, "XObject", xobjs);
            pdf_drop_obj(ctx, xobjs);

            fz_rect mediabox = fz_make_rect(0, 0, (float)w, (float)h);
            pdf_obj *page = pdf_add_page(ctx, doc, mediabox, 0, resources, contents);
            pdf_drop_obj(ctx, resources);
            fz_drop_buffer(ctx, contents);

            pdf_insert_page(ctx, doc, -1, page);
            pdf_drop_obj(ctx, page);
        }

        savePdfDoc(ctx, doc, outputPath, password);
    } fz_catch(ctx) {
        pdf_drop_document(ctx, doc);
        fz_drop_context(ctx);
        throw std::runtime_error(fz_caught_message(ctx));
    }

    pdf_drop_document(ctx, doc);
    fz_drop_context(ctx);
}
