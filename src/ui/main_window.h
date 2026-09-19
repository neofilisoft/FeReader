#pragma once
#include <QMainWindow>
#include <QString>
#include "../core/types.h"
#include "../render/render_engine.h"
#include "../core/config_manager.h"

class QStackedWidget;
class QTextBrowser;
class QLabel;
class QScrollArea;
class QWidget;
class QVBoxLayout;
class QToolButton;
class QMenu;
class QAction;
class PageScrollArea;
class ClickableLabel;

// Main application window - exact C++ equivalent of Python FeReaderWindow.
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    // Returns the config manager (used by main() to read display_mode)
    ConfigManager &configManager() { return m_cfg; }

    void loadFile(const QString &path);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void openFile();
    void goPrev();
    void goNext();
    void zoomIn();
    void zoomOut();
    void zoomLabelClicked();
    void setViewOrientation(FeReader::ViewOrientation orientation);
    void toggleFullscreen();
    void openSettingsDialog();
    void openConvertDialog();
    void printDocument();
    void rotatePageCw();
    void rotatePageCcw();
    void deleteCurrentPage();
    void mergePdfFiles();
    void extractPdfPages();
    void addPdfNote();
    void savePdfCopy();

private:
    void createActions();
    void createToolbar();
    void updateView();
    void updateStatusBar();
    void updateZoomLabel();
    void applyTheme();
    void applyLanguage();
    void saveSettings();
    void loadUserFonts();
    QString tr_(const QString &key) const; // avoids clash with QObject::tr

    // Core
    ConfigManager m_cfg;
    RenderEngine  m_renderer;

    // Settings state
    QString m_language;
    QString m_theme;
    QString m_fontFamily;
    int     m_baseFontSize    = 16;
    int     m_currentFontSize = 16;
    double  m_currentZoom     = 1.0;
    FeReader::ViewOrientation m_viewOrientation = FeReader::ViewOrientation::Vertical;
    QString m_currentBookTitle;
    QString m_currentFilePath;
    int     m_currentIndex = 0;

    // UI
    QStackedWidget  *m_stack         = nullptr;
    QTextBrowser    *m_textView      = nullptr;
    QLabel          *m_singleImageLabel = nullptr;
    PageScrollArea  *m_singleScroll  = nullptr;
    QWidget         *m_multiContainer = nullptr;
    QVBoxLayout     *m_multiLayout   = nullptr;
    QScrollArea     *m_multiScroll   = nullptr;

    QToolButton     *m_menuBtn  = nullptr;
    QToolButton     *m_viewBtn  = nullptr;
    QToolButton     *m_editBtn  = nullptr;
    QMenu           *m_mainMenu = nullptr;
    QMenu           *m_viewMenu = nullptr;
    QMenu           *m_editMenu = nullptr;
    QAction         *m_prevAction      = nullptr;
    QAction         *m_nextAction      = nullptr;
    QAction         *m_fullscreenAction = nullptr;
    QAction         *m_printAction     = nullptr;
    QAction         *m_rotateCwAction  = nullptr;
    QAction         *m_rotateCcwAction = nullptr;
    QAction         *m_deletePageAction = nullptr;
    QAction         *m_mergePdfAction  = nullptr;
    QAction         *m_extractPagesAction = nullptr;
    QAction         *m_addNoteAction   = nullptr;
    QAction         *m_saveAsAction    = nullptr;
    QAction         *m_vAct  = nullptr;
    QAction         *m_hAct  = nullptr;
    ClickableLabel  *m_zoomLabel = nullptr;
};
