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
        QStringLiteral("Files (*.pdf *.epub)"));
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
        } else {
            return;
        }

        m_currentBookTitle = QFileInfo(path).fileName();
        m_currentIndex     = 0;
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

    } else if (m_renderer.bookType() == FeReader::BookType::Pdf) {
        m_stack->setCurrentWidget(m_singleScroll);
        QPixmap pix;
        if (m_viewOrientation == FeReader::ViewOrientation::Horizontal)
            pix = m_renderer.getPdfSpreadPixmap(m_currentIndex, m_currentZoom);
        else
            pix = m_renderer.getPdfPagePixmap(m_currentIndex, m_currentZoom);

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
    int step = (m_renderer.bookType() == FeReader::BookType::Pdf
                && m_viewOrientation == FeReader::ViewOrientation::Horizontal) ? 2 : 1;
    m_currentIndex = qMax(0, m_currentIndex - step);
    updateView();
}

void MainWindow::goNext()
{
    if (m_renderer.pages().isEmpty()) return;
    int step  = (m_renderer.bookType() == FeReader::BookType::Pdf
                 && m_viewOrientation == FeReader::ViewOrientation::Horizontal) ? 2 : 1;
    int limit = m_renderer.pages().size() - 1;
    if (m_renderer.bookType() == FeReader::BookType::Pdf
        && m_viewOrientation == FeReader::ViewOrientation::Horizontal
        && limit % 2 != 0)
        --limit;
    m_currentIndex = qMin(limit, m_currentIndex + step);
    updateView();
}

void MainWindow::zoomIn()
{
    if (m_renderer.bookType() == FeReader::BookType::Pdf)
        m_currentZoom = qMin(5.0, m_currentZoom + 0.1);
    else
        m_currentFontSize = qMin(60, m_currentFontSize + 2);
    updateView();
}

void MainWindow::zoomOut()
{
    if (m_renderer.bookType() == FeReader::BookType::Pdf)
        m_currentZoom = qMax(0.1, m_currentZoom - 0.1);
    else
        m_currentFontSize = qMax(8, m_currentFontSize - 2);
    updateView();
}

void MainWindow::zoomLabelClicked()
{
    bool ok = false;
    int cur = (m_renderer.bookType() == FeReader::BookType::Pdf)
        ? qMax(10, (int)(m_currentZoom * 100))
        : qMax(10, (int)((double)m_currentFontSize / m_baseFontSize * 100));
    int val = QInputDialog::getInt(this, QStringLiteral("Zoom"),
        QStringLiteral("Percent:"), cur, 10, 500, 1, &ok);
    if (ok) {
        if (m_renderer.bookType() == FeReader::BookType::Pdf)
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
    if (m_renderer.bookType() == FeReader::BookType::Pdf)
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
