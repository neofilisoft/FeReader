#include "main_window.h"
#include "page_scroll_area.h"
#include "clickable_label.h"
#include "settings_dialog.h"
#include "convert_dialog.h"
#include "../core/i18n.h"
#include "../core/types.h"

#include <QApplication>
#include <QStackedWidget>
#include <QTextBrowser>
#include <QLabel>
#include <QScrollArea>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QToolButton>
#include <QMenu>
#include <QAction>
#include <QStatusBar>
#include <QFileDialog>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QFont>
#include <QFontDatabase>
#include <QKeySequence>
#include <QSettings>
#include <QDir>
#include <QFileInfo>
#include <QCloseEvent>
#include <QIcon>
#include <QPrinter>
#include <QPrintDialog>
#include <QPainter>
#include "../editor/pdf_editor.h"

// ---------------------------------------------------------------------------
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // Load settings
    m_language       = m_cfg.get("language", "en");
    m_theme          = m_cfg.get("theme",    "light");
    m_fontFamily     = m_cfg.get("font_family", "Segoe UI");
    m_baseFontSize   = m_cfg.getInt("font_size", 16);
    m_currentFontSize = m_baseFontSize;

    loadUserFonts();

    setWindowIcon(QIcon(QStringLiteral(":/icon.ico")));
    setWindowTitle(QStringLiteral("FeReader - Version ") + FeReader::APP_VERSION);
    resize(1600, 900);

    // ---------- Central widget: stacked ----------
    m_stack = new QStackedWidget(this);

    // EPUB text view
    m_textView = new QTextBrowser(this);
    m_textView->setOpenExternalLinks(true);

    // Single page PDF view
    m_singleImageLabel = new QLabel(this);
    m_singleImageLabel->setAlignment(Qt::AlignCenter);
    m_singleScroll = new PageScrollArea(this);
    m_singleScroll->setWidgetResizable(true);
    m_singleScroll->setWidget(m_singleImageLabel);
    m_singleScroll->onScrollPrev = [this]{ goPrev(); };
    m_singleScroll->onScrollNext = [this]{ goNext(); };

    // Multi/continuous page container
    m_multiContainer = new QWidget(this);
    m_multiLayout    = new QVBoxLayout(m_multiContainer);
    m_multiLayout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    m_multiScroll = new QScrollArea(this);
    m_multiScroll->setWidgetResizable(true);
    m_multiScroll->setWidget(m_multiContainer);

    m_stack->addWidget(m_textView);
    m_stack->addWidget(m_singleScroll);
    m_stack->addWidget(m_multiScroll);
    setCentralWidget(m_stack);

    // ---------- Actions + Toolbar + Statusbar ----------
    createActions();
    createToolbar();
    setStatusBar(new QStatusBar(this));
    updateStatusBar();

    applyTheme();
    applyLanguage();

    // Restore window geometry
    QSettings wSettings(FeReader::ORG_NAME, FeReader::APP_NAME);
    QByteArray geom = wSettings.value("window/geometry").toByteArray();
    if (!geom.isEmpty()) restoreGeometry(geom);
}

MainWindow::~MainWindow() = default;

// ---------------------------------------------------------------------------
void MainWindow::closeEvent(QCloseEvent *event)
{
    m_renderer.cleanup();
    saveSettings();
    QSettings wSettings(FeReader::ORG_NAME, FeReader::APP_NAME);
    wSettings.setValue("window/geometry", saveGeometry());
    event->accept();
}

void MainWindow::saveSettings()
{
    m_cfg.set("theme",       m_theme);
    m_cfg.set("font_family", m_fontFamily);
    m_cfg.set("font_size",   QString::number(m_baseFontSize));
    m_cfg.set("language",    m_language);

    QString mode = "0";
    if (isFullScreen())   mode = "2";
    else if (isMaximized()) mode = "1";
    m_cfg.set("display_mode", mode);
    m_cfg.save();
}

void MainWindow::loadUserFonts()
{
    QString appDir = QCoreApplication::applicationDirPath();
    QDir dir(appDir);
    const QStringList filters = { "*.ttf", "*.otf" };
    const auto entries = dir.entryList(filters, QDir::Files);
    for (const QString &name : entries)
        QFontDatabase::addApplicationFont(dir.filePath(name));
}

// ---------------------------------------------------------------------------
QString MainWindow::tr_(const QString &key) const
{
    return I18n::tr(key, m_language);
}

void MainWindow::applyLanguage()
{
    m_menuBtn->setText(tr_("menu"));
    m_prevAction->setText(tr_("prev"));
    m_nextAction->setText(tr_("next"));
    m_viewBtn->setText(tr_("view"));
    if (m_editBtn) m_editBtn->setText(tr_("edit"));
    m_vAct->setText(tr_("vertical"));
    m_hAct->setText(tr_("horizontal"));
    if (m_printAction) m_printAction->setText(tr_("print"));
    if (m_rotateCwAction) m_rotateCwAction->setText(tr_("rotate_cw"));
    if (m_rotateCcwAction) m_rotateCcwAction->setText(tr_("rotate_ccw"));
    if (m_deletePageAction) m_deletePageAction->setText(tr_("delete_page"));
    if (m_mergePdfAction) m_mergePdfAction->setText(tr_("merge_pdf"));
    if (m_extractPagesAction) m_extractPagesAction->setText(tr_("extract_pages"));
    if (m_addNoteAction) m_addNoteAction->setText(tr_("add_note"));
    if (m_saveAsAction) m_saveAsAction->setText(tr_("save_as"));
    updateStatusBar();
}

void MainWindow::applyTheme()
{
    bool dark = (m_theme == "dark");
    QString bg  = dark ? "#202020" : "#ffffff";
    QString fg  = dark ? "#f0f0f0" : "#000000";
    QString tbBg = "#f5f5f5"; // same in both modes - match original

    setStyleSheet(QStringLiteral(
        "QMainWindow, QTextBrowser, QScrollArea { background-color: %1; color: %2; }"
        "QLabel { color: %2; }"
        "QToolBar { background: %3; border: none; spacing: 6px; }"
        "QToolButton::menu-indicator { image: none; }")
        .arg(bg, fg, tbBg));
}

// ---------------------------------------------------------------------------
void MainWindow::createActions()
{
    m_fullscreenAction = new QAction(QStringLiteral("Fullscreen"), this);
    m_fullscreenAction->setShortcut(QKeySequence(Qt::Key_F11));
    connect(m_fullscreenAction, &QAction::triggered, this, &MainWindow::toggleFullscreen);
    addAction(m_fullscreenAction);

    m_prevAction = new QAction(tr_("prev"), this);
    connect(m_prevAction, &QAction::triggered, this, &MainWindow::goPrev);

    m_nextAction = new QAction(tr_("next"), this);
    connect(m_nextAction, &QAction::triggered, this, &MainWindow::goNext);
}

void MainWindow::createToolbar()
{
    QToolBar *tb = new QToolBar(QStringLiteral("Main"), this);
    tb->setMovable(false);
    addToolBar(tb);

    // File menu button
    m_menuBtn = new QToolButton(this);
    m_menuBtn->setPopupMode(QToolButton::InstantPopup);
    m_mainMenu = new QMenu(this);
    m_mainMenu->addAction(tr_("open"),     this, &MainWindow::openFile,
                          QKeySequence(QStringLiteral("Ctrl+O")));
    m_printAction = m_mainMenu->addAction(tr_("print"), this, &MainWindow::printDocument,
                                          QKeySequence(QStringLiteral("Ctrl+P")));
    m_saveAsAction = m_mainMenu->addAction(tr_("save_as"), this, &MainWindow::savePdfCopy,
                                           QKeySequence(QStringLiteral("Ctrl+Shift+S")));
    m_mainMenu->addSeparator();
    m_mainMenu->addAction(tr_("settings"), this, &MainWindow::openSettingsDialog,
                          QKeySequence(Qt::Key_F1));
    m_mainMenu->addAction(tr_("convert"),  this, &MainWindow::openConvertDialog,
                          QKeySequence(Qt::Key_F2));
    m_mainMenu->addSeparator();
    m_mainMenu->addAction(tr_("exit"),
                          QApplication::instance(), &QCoreApplication::quit,
                          QKeySequence(QStringLiteral("Alt+F4")));
    m_menuBtn->setMenu(m_mainMenu);
    m_menuBtn->setText(tr_("menu"));
    tb->addWidget(m_menuBtn);

    // View menu button
    m_viewBtn = new QToolButton(this);
    m_viewBtn->setPopupMode(QToolButton::InstantPopup);
    m_viewMenu = new QMenu(this);
    m_vAct = m_viewMenu->addAction(tr_("vertical"),
        [this]{ setViewOrientation(FeReader::ViewOrientation::Vertical); });
    m_hAct = m_viewMenu->addAction(tr_("horizontal"),
        [this]{ setViewOrientation(FeReader::ViewOrientation::Horizontal); });
    m_vAct->setCheckable(true);
    m_hAct->setCheckable(true);
    m_vAct->setChecked(true);
    m_viewBtn->setMenu(m_viewMenu);
    m_viewBtn->setText(tr_("view"));
    tb->addWidget(m_viewBtn);

    // Edit menu button
    m_editBtn = new QToolButton(this);
    m_editBtn->setPopupMode(QToolButton::InstantPopup);
    m_editMenu = new QMenu(this);
    m_rotateCwAction = m_editMenu->addAction(tr_("rotate_cw"), this, &MainWindow::rotatePageCw,
                                             QKeySequence(QStringLiteral("Ctrl+R")));
    m_rotateCcwAction = m_editMenu->addAction(tr_("rotate_ccw"), this, &MainWindow::rotatePageCcw,
                                              QKeySequence(QStringLiteral("Ctrl+Shift+R")));
    m_editMenu->addSeparator();
    m_deletePageAction = m_editMenu->addAction(tr_("delete_page"), this, &MainWindow::deleteCurrentPage,
                                               QKeySequence(QKeySequence::Delete));
    m_extractPagesAction = m_editMenu->addAction(tr_("extract_pages"), this, &MainWindow::extractPdfPages);
    m_mergePdfAction = m_editMenu->addAction(tr_("merge_pdf"), this, &MainWindow::mergePdfFiles);
    m_editMenu->addSeparator();
    m_addNoteAction = m_editMenu->addAction(tr_("add_note"), this, &MainWindow::addPdfNote);
    m_editBtn->setMenu(m_editMenu);
    m_editBtn->setText(tr_("edit"));
    tb->addWidget(m_editBtn);

    // Print button on toolbar
    QAction *tbPrintAct = tb->addAction(QStringLiteral("\U0001F5B6"), this, &MainWindow::printDocument);
    tbPrintAct->setToolTip(QStringLiteral("Print (Ctrl+P)"));

    tb->addSeparator();
    tb->addAction(m_prevAction);
    tb->addAction(m_nextAction);

    tb->addSeparator();
    tb->addAction(QStringLiteral("\U0001F50D+"), this, &MainWindow::zoomIn);
    tb->addAction(QStringLiteral("\U0001F50D-"), this, &MainWindow::zoomOut);

    m_zoomLabel = new ClickableLabel(QStringLiteral("100%"), this);
    m_zoomLabel->setMinimumWidth(60);
    connect(m_zoomLabel, &ClickableLabel::clicked, this, &MainWindow::zoomLabelClicked);
    tb->addWidget(m_zoomLabel);
}

// ---------------------------------------------------------------------------
void MainWindow::openFile()
{
    QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("Open"), QString(),
        QStringLiteral("All Supported (*.pdf *.epub *.cbz *.cbr *.zip *.rar *.cb7 *.cbt);;Comic Books (*.cbz *.cbr *.zip *.rar *.cb7 *.cbt);;PDF Files (*.pdf);;EPUB Files (*.epub);;All Files (*.*)"));
    if (!path.isEmpty()) {
        loadFile(path);
    }
}

void MainWindow::loadFile(const QString &path)
{
    if (path.isEmpty()) return;

    QString ext = QFileInfo(path).suffix().toLower();
    try {
        if (ext == "pdf") {
            auto pwCb = [this]() -> QString {
                bool ok = false;
                QString pw = QInputDialog::getText(
                    this, QStringLiteral("Password"),
                    QStringLiteral("Enter:"), QLineEdit::Password, QString(), &ok);
                return ok ? pw : QString();
            };
            m_renderer.loadPdf(path, pwCb);
            m_stack->setCurrentWidget(m_singleScroll);

            int vw = m_singleScroll->viewport()->width() - 25;
            int vh = m_singleScroll->viewport()->height() - 25;
            if (vw <= 100 || vh <= 100) {
                vw = m_stack->width() - 30;
                vh = m_stack->height() - 30;
            }
            if (vw <= 100 || vh <= 100) {
                vw = width() - 30;
                vh = height() - 80;
            }
            m_currentZoom = m_renderer.getInitialZoom(vw, vh);
        } else if (ext == "epub") {
            m_renderer.loadEpub(path);
            m_currentFontSize = m_baseFontSize;
        } else if (ext == "cbz" || ext == "cbr" || ext == "zip" || ext == "rar" || ext == "cb7" || ext == "cbt") {
            m_renderer.loadComic(path);
            m_stack->setCurrentWidget(m_singleScroll);

            int vw = m_singleScroll->viewport()->width() - 25;
            int vh = m_singleScroll->viewport()->height() - 25;
            if (vw <= 100 || vh <= 100) {
                vw = m_stack->width() - 30;
                vh = m_stack->height() - 30;
            }
            if (vw <= 100 || vh <= 100) {
                vw = width() - 30;
                vh = height() - 80;
            }
            m_currentZoom = m_renderer.getInitialZoom(vw, vh);
        } else {
            return;
        }

        m_currentFilePath  = QFileInfo(path).absoluteFilePath();
        m_currentBookTitle = QFileInfo(path).fileName();
        m_currentIndex     = 0;
        if (m_editBtn) m_editBtn->setEnabled(m_renderer.bookType() == FeReader::BookType::Pdf);
        updateView();

    } catch (const std::exception &e) {
        QMessageBox::critical(this, QStringLiteral("Error"),
                              QString::fromStdString(e.what()));
    }
}

// ---------------------------------------------------------------------------
void MainWindow::updateView()
{
    if (m_renderer.pages().isEmpty()) {
        m_stack->setCurrentWidget(m_textView);
        m_textView->setPlainText(QString());
        updateStatusBar();
        return;
    }

    if (m_renderer.bookType() == FeReader::BookType::Epub) {
        m_stack->setCurrentWidget(m_textView);
        m_textView->setHtml(m_renderer.pages().at(m_currentIndex));
        m_textView->setFont(QFont(m_fontFamily, m_currentFontSize));

    } else if (m_renderer.bookType() == FeReader::BookType::Pdf ||
               m_renderer.bookType() == FeReader::BookType::Comic) {
        m_stack->setCurrentWidget(m_singleScroll);
        QPixmap pix;
        if (m_viewOrientation == FeReader::ViewOrientation::Horizontal)
            pix = m_renderer.getSpreadPixmap(m_currentIndex, m_currentZoom);
        else
            pix = m_renderer.getPagePixmap(m_currentIndex, m_currentZoom);

        if (!pix.isNull()) {
            m_singleImageLabel->setPixmap(pix);
            m_singleImageLabel->adjustSize();
        }
    }

    updateStatusBar();
    updateZoomLabel();
}

// ---------------------------------------------------------------------------
void MainWindow::goPrev()
{
    if (m_renderer.pages().isEmpty()) return;
    bool isImageDoc = (m_renderer.bookType() == FeReader::BookType::Pdf ||
                       m_renderer.bookType() == FeReader::BookType::Comic);
    int step = (isImageDoc && m_viewOrientation == FeReader::ViewOrientation::Horizontal) ? 2 : 1;
    m_currentIndex = qMax(0, m_currentIndex - step);
    updateView();
}

void MainWindow::goNext()
{
    if (m_renderer.pages().isEmpty()) return;
    bool isImageDoc = (m_renderer.bookType() == FeReader::BookType::Pdf ||
                       m_renderer.bookType() == FeReader::BookType::Comic);
    int step  = (isImageDoc && m_viewOrientation == FeReader::ViewOrientation::Horizontal) ? 2 : 1;
    int limit = m_renderer.pages().size() - 1;
    if (isImageDoc && m_viewOrientation == FeReader::ViewOrientation::Horizontal && limit % 2 != 0)
        --limit;
    m_currentIndex = qMin(limit, m_currentIndex + step);
    updateView();
}

void MainWindow::zoomIn()
{
    if (m_renderer.bookType() == FeReader::BookType::Pdf ||
        m_renderer.bookType() == FeReader::BookType::Comic)
        m_currentZoom = qMin(5.0, m_currentZoom + 0.1);
    else
        m_currentFontSize = qMin(60, m_currentFontSize + 2);
    updateView();
}

void MainWindow::zoomOut()
{
    if (m_renderer.bookType() == FeReader::BookType::Pdf ||
        m_renderer.bookType() == FeReader::BookType::Comic)
        m_currentZoom = qMax(0.1, m_currentZoom - 0.1);
    else
        m_currentFontSize = qMax(8, m_currentFontSize - 2);
    updateView();
}

void MainWindow::zoomLabelClicked()
{
    bool ok = false;
    bool isImageDoc = (m_renderer.bookType() == FeReader::BookType::Pdf ||
                       m_renderer.bookType() == FeReader::BookType::Comic);
    int cur = isImageDoc
        ? qMax(10, (int)(m_currentZoom * 100))
        : qMax(10, (int)((double)m_currentFontSize / m_baseFontSize * 100));
    int val = QInputDialog::getInt(this, QStringLiteral("Zoom"),
        QStringLiteral("Percent:"), cur, 10, 500, 1, &ok);
    if (ok) {
        if (isImageDoc)
            m_currentZoom = val / 100.0;
        else
            m_currentFontSize = (int)(m_baseFontSize * (val / 100.0));
        updateView();
    }
}

void MainWindow::setViewOrientation(FeReader::ViewOrientation orientation)
{
    m_viewOrientation = orientation;
    m_vAct->setChecked(orientation == FeReader::ViewOrientation::Vertical);
    m_hAct->setChecked(orientation == FeReader::ViewOrientation::Horizontal);
    updateView();
}

void MainWindow::toggleFullscreen()
{
    if (isFullScreen()) showNormal();
    else showFullScreen();
}

// ---------------------------------------------------------------------------
void MainWindow::updateStatusBar()
{
    int count = m_renderer.pages().size();
    QString msg;
    if (count > 0)
        msg = QStringLiteral("%1 | Page %2/%3")
              .arg(m_currentBookTitle)
              .arg(m_currentIndex + 1)
              .arg(count);
    else
        msg = tr_("no_document");
    statusBar()->showMessage(msg);
}

void MainWindow::updateZoomLabel()
{
    if (m_renderer.bookType() == FeReader::BookType::Pdf ||
        m_renderer.bookType() == FeReader::BookType::Comic)
        m_zoomLabel->setText(QStringLiteral("%1%").arg((int)(m_currentZoom * 100)));
    else
        m_zoomLabel->setText(QStringLiteral("%1%").arg(
            (int)((double)m_currentFontSize / m_baseFontSize * 100)));
}

// ---------------------------------------------------------------------------
void MainWindow::openSettingsDialog()
{
    QFontDatabase db;
    QStringList fonts = db.families();
    fonts.sort();

    SettingsDialog dlg(this, fonts, m_fontFamily, m_baseFontSize, m_theme, m_language);
    if (dlg.exec() == QDialog::Accepted) {
        auto v = dlg.getValues();
        m_fontFamily     = v.fontFamily;
        m_baseFontSize   = v.fontSize;
        m_theme          = v.theme;
        m_language       = v.language;
        m_currentFontSize = m_baseFontSize;
        applyTheme();
        applyLanguage();
        saveSettings();
        updateView();
    }
}

void MainWindow::openConvertDialog()
{
    ConvertDialog dlg(this, m_language);
    dlg.exec();
}

// ---------------------------------------------------------------------------
// Print Support (Qt5::PrintSupport)
// ---------------------------------------------------------------------------
void MainWindow::printDocument()
{
    if (m_renderer.bookType() == FeReader::BookType::None) {
        QMessageBox::information(this, tr_("print"), tr_("no_document"));
        return;
    }

    QPrinter printer(QPrinter::HighResolution);
    QPrintDialog printDlg(&printer, this);
    printDlg.setWindowTitle(tr_("print"));

    if (m_renderer.bookType() == FeReader::BookType::Pdf ||
        m_renderer.bookType() == FeReader::BookType::Comic) {
        printer.setFromTo(1, m_renderer.pageCount());
    }

    if (printDlg.exec() != QDialog::Accepted) {
        return;
    }

    if (m_renderer.bookType() == FeReader::BookType::Epub) {
        m_textView->print(&printer);
        return;
    }

    // PDF and Comic printing
    int fromPage = printer.fromPage();
    int toPage   = printer.toPage();
    if (printer.printRange() == QPrinter::AllPages || fromPage == 0) {
        fromPage = 1;
        toPage   = m_renderer.pageCount();
    } else {
        fromPage = qMax(1, fromPage);
        toPage   = qMin(m_renderer.pageCount(), toPage);
    }

    QPainter painter(&printer);
    QRect pageRect = printer.pageLayout().paintRectPixels(printer.resolution());

    for (int p = fromPage; p <= toPage; ++p) {
        if (p > fromPage) {
            printer.newPage();
        }
        // Render at high resolution for printing
        QPixmap pix = m_renderer.getPagePixmap(p - 1, 2.0);
        if (!pix.isNull()) {
            QPixmap scaled = pix.scaled(pageRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
            int x = pageRect.left() + (pageRect.width() - scaled.width()) / 2;
            int y = pageRect.top()  + (pageRect.height() - scaled.height()) / 2;
            painter.drawPixmap(x, y, scaled);
        }
    }
    painter.end();
}

// ---------------------------------------------------------------------------
// PDF Editing features (Page operations, annotations, merge, extract)
// ---------------------------------------------------------------------------
void MainWindow::rotatePageCw()
{
    if (m_renderer.bookType() != FeReader::BookType::Pdf || m_currentFilePath.isEmpty()) return;
    PdfEditor editor;
    QString err;
    int curIndex = m_currentIndex;
    m_renderer.cleanup();

    if (!editor.rotatePage(m_currentFilePath, m_currentFilePath, curIndex, 90, err)) {
        QMessageBox::critical(this, tr_("edit"), err);
    }
    m_renderer.clearCache();
    loadFile(m_currentFilePath);
    m_currentIndex = curIndex;
    updateView();
}

void MainWindow::rotatePageCcw()
{
    if (m_renderer.bookType() != FeReader::BookType::Pdf || m_currentFilePath.isEmpty()) return;
    PdfEditor editor;
    QString err;
    int curIndex = m_currentIndex;
    m_renderer.cleanup();

    if (!editor.rotatePage(m_currentFilePath, m_currentFilePath, curIndex, 270, err)) {
        QMessageBox::critical(this, tr_("edit"), err);
    }
    m_renderer.clearCache();
    loadFile(m_currentFilePath);
    m_currentIndex = curIndex;
    updateView();
}

void MainWindow::deleteCurrentPage()
{
    if (m_renderer.bookType() != FeReader::BookType::Pdf || m_currentFilePath.isEmpty()) return;
    if (m_renderer.pageCount() <= 1) {
        QMessageBox::warning(this, tr_("edit"), QStringLiteral("Cannot delete the only page in the document."));
        return;
    }

    auto ret = QMessageBox::question(this, tr_("delete_page"),
        QStringLiteral("Delete page %1?").arg(m_currentIndex + 1),
        QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) return;

    PdfEditor editor;
    QString err;
    int curIndex = m_currentIndex;
    m_renderer.cleanup();

    if (!editor.deletePages(m_currentFilePath, m_currentFilePath, { curIndex }, err)) {
        QMessageBox::critical(this, tr_("edit"), err);
    }
    m_renderer.clearCache();
    loadFile(m_currentFilePath);
    m_currentIndex = qMin(curIndex, m_renderer.pageCount() - 1);
    updateView();
}

void MainWindow::mergePdfFiles()
{
    QStringList files = QFileDialog::getOpenFileNames(
        this, tr_("merge_pdf"), QString(),
        QStringLiteral("PDF Files (*.pdf)"));
    if (files.size() < 2) return;

    QString outPath = QFileDialog::getSaveFileName(
        this, tr_("save_as"), QString(),
        QStringLiteral("PDF Files (*.pdf)"));
    if (outPath.isEmpty()) return;
    if (!outPath.endsWith(QStringLiteral(".pdf"), Qt::CaseInsensitive))
        outPath += QStringLiteral(".pdf");

    PdfEditor editor;
    QString err;
    if (!editor.mergePdfs(files, outPath, err)) {
        QMessageBox::critical(this, tr_("edit"), err);
        return;
    }

    auto ret = QMessageBox::question(this, tr_("merge_pdf"),
        QStringLiteral("Merge complete! Open merged file?"),
        QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        loadFile(outPath);
    }
}

void MainWindow::extractPdfPages()
{
    if (m_renderer.bookType() != FeReader::BookType::Pdf || m_currentFilePath.isEmpty()) return;

    bool ok = false;
    QString rangeStr = QInputDialog::getText(
        this, tr_("extract_pages"),
        QStringLiteral("Enter page numbers/ranges (e.g. 1, 3-5):"),
        QLineEdit::Normal, QString::number(m_currentIndex + 1), &ok);
    if (!ok || rangeStr.trimmed().isEmpty()) return;

    // Parse ranges (1-indexed to 0-indexed)
    QList<int> pages;
    const QStringList parts = rangeStr.split(QLatin1Char(','), Qt::SkipEmptyParts);
    for (const QString &part : parts) {
        QString s = part.trimmed();
        if (s.contains(QLatin1Char('-'))) {
            QStringList bounds = s.split(QLatin1Char('-'));
            if (bounds.size() == 2) {
                int start = bounds[0].trimmed().toInt() - 1;
                int end   = bounds[1].trimmed().toInt() - 1;
                for (int p = qMin(start, end); p <= qMax(start, end); ++p) {
                    if (p >= 0 && p < m_renderer.pageCount() && !pages.contains(p))
                        pages << p;
                }
            }
        } else {
            int p = s.toInt() - 1;
            if (p >= 0 && p < m_renderer.pageCount() && !pages.contains(p))
                pages << p;
        }
    }

    if (pages.isEmpty()) {
        QMessageBox::warning(this, tr_("extract_pages"), QStringLiteral("No valid pages found in range."));
        return;
    }

    QString outPath = QFileDialog::getSaveFileName(
        this, tr_("save_as"), QString(),
        QStringLiteral("PDF Files (*.pdf)"));
    if (outPath.isEmpty()) return;
    if (!outPath.endsWith(QStringLiteral(".pdf"), Qt::CaseInsensitive))
        outPath += QStringLiteral(".pdf");

    PdfEditor editor;
    QString err;
    if (!editor.extractPages(m_currentFilePath, outPath, pages, err)) {
        QMessageBox::critical(this, tr_("edit"), err);
        return;
    }

    auto ret = QMessageBox::question(this, tr_("extract_pages"),
        QStringLiteral("Pages extracted! Open new file?"),
        QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        loadFile(outPath);
    }
}

void MainWindow::addPdfNote()
{
    if (m_renderer.bookType() != FeReader::BookType::Pdf || m_currentFilePath.isEmpty()) return;

    bool ok = false;
    QString text = QInputDialog::getMultiLineText(
        this, tr_("add_note"),
        QStringLiteral("Note content:"), QString(), &ok);
    if (!ok || text.trimmed().isEmpty()) return;

    PdfEditor editor;
    QString err;
    int curIndex = m_currentIndex;
    m_renderer.cleanup();

    // Place note in top area of page
    QRectF rect(72.0, 72.0, 150.0, 100.0);
    QColor yellow(255, 255, 0);

    if (!editor.addTextAnnotation(m_currentFilePath, m_currentFilePath, curIndex, rect, text, yellow, err)) {
        QMessageBox::critical(this, tr_("edit"), err);
    }
    m_renderer.clearCache();
    loadFile(m_currentFilePath);
    m_currentIndex = curIndex;
    updateView();
}

void MainWindow::savePdfCopy()
{
    if (m_currentFilePath.isEmpty()) return;

    QString ext = QFileInfo(m_currentFilePath).suffix().toLower();
    QString filter = (ext == "epub") ? QStringLiteral("EPUB Files (*.epub)") : QStringLiteral("PDF Files (*.pdf)");
    QString outPath = QFileDialog::getSaveFileName(this, tr_("save_as"), QString(), filter);
    if (outPath.isEmpty()) return;

    if (QFile::exists(outPath)) QFile::remove(outPath);
    if (QFile::copy(m_currentFilePath, outPath)) {
        QMessageBox::information(this, tr_("save_as"), QStringLiteral("File saved successfully!"));
    } else {
        QMessageBox::critical(this, tr_("save_as"), QStringLiteral("Failed to save copy."));
    }
}
