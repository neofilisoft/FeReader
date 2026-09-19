#include "epub_engine.h"

#include <zip.h>
#include <gumbo.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QUrl>
#include <QDomDocument>
#include <QTextStream>
#include <stdexcept>
#include <string>
#include <sstream>

// ---------------------------------------------------------------------------
// Helper: normalize a posix-style relative path
// ---------------------------------------------------------------------------
static QString normalizePath(const QString &base, const QString &rel)
{
    // base is the directory of the referencing file (posix / forward slash)
    QString joined = base.isEmpty() ? rel : (base + "/" + rel);
    // Use QUrl to normalize (handle ../ etc.)
    QString clean = QUrl(QStringLiteral("file:///") + joined).path().mid(1); // strip leading /
    if (clean.startsWith('/')) clean = clean.mid(1);
    return clean;
}

static void cleanOrphanedTempDirs(const QString &activeDir = QString())
{
    QDir tempDir = QDir::temp();
    const QStringList filters = { "FeReader-*", "fereader_*" };
    const auto entries = tempDir.entryInfoList(filters, QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &fi : entries) {
        if (activeDir.isEmpty() || fi.absoluteFilePath() != activeDir) {
            QDir(fi.absoluteFilePath()).removeRecursively();
        }
    }
}

// ---------------------------------------------------------------------------
// EpubEngine
// ---------------------------------------------------------------------------
EpubEngine::EpubEngine()
{
    cleanOrphanedTempDirs();
}

EpubEngine::~EpubEngine() { close(); }

void EpubEngine::close()
{
    if (m_tempDir) {
        QString p = m_tempDir->path();
        m_tempDir.reset();
        if (QDir(p).exists())
            QDir(p).removeRecursively();
    }
    cleanOrphanedTempDirs();
}

QString EpubEngine::tempDir() const
{
    return m_tempDir ? m_tempDir->path() : QString();
}

// ---------------------------------------------------------------------------
// Step 1: Extract all zip entries to outDir
// ---------------------------------------------------------------------------
bool EpubEngine::extractZip(const QString &zipPath, const QString &outDir, QString &error)
{
    int err = 0;
    zip_t *za = zip_open(zipPath.toUtf8().constData(), ZIP_RDONLY, &err);
    if (!za) {
        zip_error_t ze;
        zip_error_init_with_code(&ze, err);
        error = QString::fromUtf8(zip_error_strerror(&ze));
        zip_error_fini(&ze);
        return false;
    }

    zip_int64_t count = zip_get_num_entries(za, 0);
    for (zip_int64_t i = 0; i < count; ++i) {
        const char *name = zip_get_name(za, i, 0);
        if (!name) continue;

        QString entryName = QString::fromUtf8(name);
        // Skip directory entries
        if (entryName.endsWith('/')) {
            QDir(outDir).mkpath(entryName);
            continue;
        }

        QString outPath = QDir(outDir).filePath(entryName);
        QFileInfo fi(outPath);
        QDir().mkpath(fi.absolutePath());

        zip_file_t *zf = zip_fopen_index(za, i, 0);
        if (!zf) continue;

        QFile outFile(outPath);
        if (outFile.open(QIODevice::WriteOnly)) {
            char buf[8192];
            zip_int64_t n;
            while ((n = zip_fread(zf, buf, sizeof(buf))) > 0)
                outFile.write(buf, (qint64)n);
            outFile.close();
        }
        zip_fclose(zf);
    }

    zip_close(za);
    return true;
}

// ---------------------------------------------------------------------------
// Step 2: Parse OPF file to get spine reading order -> relative HTML paths
// ---------------------------------------------------------------------------
QStringList EpubEngine::parseSpineOrder(const QString &tmpDir)
{
    QStringList result;

    // Locate container.xml to find OPF path
    QString containerPath = QDir(tmpDir).filePath("META-INF/container.xml");
    if (!QFile::exists(containerPath)) {
        // Fallback: collect XHTML/HTML files in order
        QDir d(tmpDir);
        QStringList filters = {"*.xhtml", "*.html", "*.htm"};
        QFileInfoList list  = d.entryInfoList(filters, QDir::Files, QDir::Name);
        for (const QFileInfo &fi : list)
            result << QDir(tmpDir).relativeFilePath(fi.absoluteFilePath());
        return result;
    }

    QFile containerFile(containerPath);
    if (!containerFile.open(QIODevice::ReadOnly)) return result;

    QDomDocument container;
    container.setContent(&containerFile);
    containerFile.close();

    // rootfile full-path attribute
    QString opfPath;
    QDomNodeList rootfiles = container.elementsByTagName("rootfile");
    if (!rootfiles.isEmpty())
        opfPath = rootfiles.at(0).toElement().attribute("full-path");

    if (opfPath.isEmpty()) return result;

    QString opfAbsPath = QDir(tmpDir).filePath(opfPath);
    QString opfDir     = QFileInfo(opfAbsPath).path();

    QFile opfFile(opfAbsPath);
    if (!opfFile.open(QIODevice::ReadOnly)) return result;

    QDomDocument opf;
    opf.setContent(&opfFile);
    opfFile.close();

    // Build manifest id -> href map
    QMap<QString, QString> manifest;
    QDomNodeList items = opf.elementsByTagName("item");
    for (int i = 0; i < items.count(); ++i) {
        QDomElement el = items.at(i).toElement();
        QString mt = el.attribute("media-type");
        if (mt.contains("html") || mt.contains("xhtml")) {
            manifest[el.attribute("id")] = el.attribute("href");
        }
    }

    // Spine itemref -> ordered list
    QDomNodeList itemrefs = opf.elementsByTagName("itemref");
    for (int i = 0; i < itemrefs.count(); ++i) {
        QString idref = itemrefs.at(i).toElement().attribute("idref");
        if (manifest.contains(idref)) {
            QString relToOpf   = manifest[idref];
            // Resolve relative to OPF directory
            QString absPath    = QDir(opfDir).filePath(relToOpf);
            QString relToTmp   = QDir(tmpDir).relativeFilePath(absPath);
            result << relToTmp;
        }
    }

    // If spine was empty, fall back to manifest order
    if (result.isEmpty()) {
        for (const QString &href : manifest.values()) {
            QString absPath  = QDir(opfDir).filePath(href);
            QString relToTmp = QDir(tmpDir).relativeFilePath(absPath);
            result << relToTmp;
        }
    }

    return result;
}

// ---------------------------------------------------------------------------
// Gumbo helpers: serialize a node tree back to HTML string
// ---------------------------------------------------------------------------
static void serializeNode(GumboNode *node, std::string &out);

static void serializeChildren(GumboNode *node, std::string &out)
{
    GumboVector *children = &node->v.element.children;
    for (unsigned int i = 0; i < children->length; ++i)
        serializeNode(static_cast<GumboNode *>(children->data[i]), out);
}

static void serializeNode(GumboNode *node, std::string &out)
{
    if (!node) return;
    if (node->type == GUMBO_NODE_TEXT) {
        out += node->v.text.text;
        return;
    }
    if (node->type == GUMBO_NODE_WHITESPACE) {
        out += node->v.text.text;
        return;
    }
    if (node->type == GUMBO_NODE_COMMENT) {
        out += "<!--";
        out += node->v.text.text;
        out += "-->";
        return;
    }
    if (node->type != GUMBO_NODE_ELEMENT &&
        node->type != GUMBO_NODE_TEMPLATE)
        return;

    GumboElement &el = node->v.element;
    const char   *tag = gumbo_normalized_tagname(el.tag);
    out += "<";
    out += tag;

    GumboVector *attrs = &el.attributes;
    for (unsigned int i = 0; i < attrs->length; ++i) {
        GumboAttribute *a = static_cast<GumboAttribute *>(attrs->data[i]);
        out += " ";
        out += a->name;
        out += "=\"";
        out += a->value;
        out += "\"";
    }
    out += ">";

    serializeChildren(node, out);

    static const char *voidTags[] = {
        "area","base","br","col","embed","hr","img","input",
        "link","meta","param","source","track","wbr", nullptr
    };
    bool isVoid = false;
    for (int i = 0; voidTags[i]; ++i)
        if (strcmp(tag, voidTags[i]) == 0) { isVoid = true; break; }

    if (!isVoid) {
        out += "</";
        out += tag;
        out += ">";
    }
}

// ---------------------------------------------------------------------------
// Step 3: Parse HTML with gumbo, rewrite img src to file:// URLs
// ---------------------------------------------------------------------------
QString EpubEngine::processHtml(const QString &htmlFilePath,
                                const QString &htmlRelDir,
                                const QString &tmpDir)
{
    QFile file(htmlFilePath);
    if (!file.open(QIODevice::ReadOnly)) return QString();

    QByteArray raw = file.readAll();
    file.close();

    GumboOutput *output = gumbo_parse_with_options(
        &kGumboDefaultOptions,
        raw.constData(),
        (size_t)raw.size());

    if (!output) return QString::fromUtf8(raw);

    // Walk all GUMBO_TAG_IMG nodes and rewrite src attribute
    std::function<void(GumboNode *)> rewriteImgs = [&](GumboNode *node) {
        if (!node) return;
        if (node->type != GUMBO_NODE_ELEMENT) return;

        if (node->v.element.tag == GUMBO_TAG_IMG) {
            GumboVector *attrs = &node->v.element.attributes;
            for (unsigned int i = 0; i < attrs->length; ++i) {
                GumboAttribute *a = static_cast<GumboAttribute *>(attrs->data[i]);
                if (strcmp(a->name, "src") == 0) {
                    QString src = QString::fromUtf8(a->value);
                    if (!src.startsWith("http") && !src.startsWith("file://")) {
                        // Resolve relative path
                        QString rel = normalizePath(htmlRelDir, src);
                        QString abs = QDir(tmpDir).filePath(rel);
                        QString fileUrl = QUrl::fromLocalFile(abs).toString();
                        // Overwrite the value
                        // gumbo doesn't support mutation; store new value
                        // We do string replacement after serialization instead
                        // Mark with a unique prefix to replace after serialize
                        std::string newVal = fileUrl.toStdString();
                        // Use gumbo_attribute directly (safe since we own output)
                        const_cast<char *&>(a->value) = nullptr;
                        // Store in a persistent std::string
                        // Workaround: rebuild attribute value from std::string
                        static std::vector<std::string> storage;
                        storage.push_back(newVal);
                        const_cast<const char *&>(a->value) = storage.back().c_str();
                    }
                    break;
                }
            }
        }

        GumboVector *children = &node->v.element.children;
        for (unsigned int i = 0; i < children->length; ++i)
            rewriteImgs(static_cast<GumboNode *>(children->data[i]));
    };

    rewriteImgs(output->root);

    std::string html;
    serializeChildren(output->root, html);
    gumbo_destroy_output(&kGumboDefaultOptions, output);

    return QString::fromStdString(html);
}

// ---------------------------------------------------------------------------
// Public: load()
// ---------------------------------------------------------------------------
QStringList EpubEngine::load(const QString &path)
{
    close();

    m_tempDir = std::make_unique<QTemporaryDir>();
    m_tempDir->setAutoRemove(true);
    if (!m_tempDir->isValid())
        throw std::runtime_error("Could not create temporary directory");

    QString tmpDirPath = m_tempDir->path();

    // Step 1: Extract
    QString error;
    if (!extractZip(path, tmpDirPath, error))
        throw std::runtime_error(("Failed to open EPUB: " + error).toStdString());

    // Step 2: Parse spine order
    QStringList spine = parseSpineOrder(tmpDirPath);

    // Step 3: Process each HTML file
    QStringList pages;
    for (const QString &relPath : spine) {
        QString absPath = QDir(tmpDirPath).filePath(relPath);
        if (!QFile::exists(absPath)) continue;

        // Get the directory of this HTML file (posix-style, relative to tmpDir)
        QString htmlRelDir = QFileInfo(relPath).path();
        if (htmlRelDir == ".") htmlRelDir.clear();

        QString html = processHtml(absPath, htmlRelDir, tmpDirPath);
        if (!html.isEmpty())
            pages << html;
    }

    if (pages.isEmpty())
        pages << QStringLiteral("<h3>No readable content found.</h3>");

    return pages;
}
