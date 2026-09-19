#include "convert_dialog.h"
#include "../core/i18n.h"
#include "../converter/converter.h"

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>

ConvertDialog::ConvertDialog(QWidget *parent, const QString &currentLang)
    : QDialog(parent), m_lang(currentLang)
{
    setModal(true);
    setWindowTitle(I18n::tr("convert_title", currentLang));

    m_modeCombo = new QComboBox(this);
    m_modeCombo->addItem(QStringLiteral("Text -> PDF"),  QStringLiteral("text_pdf"));
    m_modeCombo->addItem(QStringLiteral("Text -> EPUB"), QStringLiteral("text_epub"));
    m_modeCombo->addItem(QStringLiteral("Images -> PDF"),QStringLiteral("images_pdf"));

    m_inputLabel  = new QLabel(QStringLiteral("Input: (none)"),  this);
    m_outputLabel = new QLabel(QStringLiteral("Output: (none)"), this);

    QPushButton *inputBtn  = new QPushButton(QStringLiteral("Browse input"),  this);
    QPushButton *outputBtn = new QPushButton(QStringLiteral("Browse output"), this);
    connect(inputBtn,  &QPushButton::clicked, this, &ConvertDialog::chooseInput);
    connect(outputBtn, &QPushButton::clicked, this, &ConvertDialog::chooseOutput);

    m_passwordCheck = new QCheckBox(QStringLiteral("Protect with password (PDF)"), this);
    m_passwordEdit  = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);

    QPushButton *convertBtn = new QPushButton(QStringLiteral("Convert"), this);
    QPushButton *cancelBtn  = new QPushButton(QStringLiteral("Cancel"),  this);
    connect(convertBtn, &QPushButton::clicked, this, &ConvertDialog::performConvert);
    connect(cancelBtn,  &QPushButton::clicked, this, &QDialog::reject);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(QStringLiteral("Mode:")));
    layout->addWidget(m_modeCombo);
    layout->addWidget(m_inputLabel);
    layout->addWidget(inputBtn);
    layout->addWidget(m_outputLabel);
    layout->addWidget(outputBtn);
    layout->addWidget(m_passwordCheck);
    layout->addWidget(m_passwordEdit);

    QHBoxLayout *btnRow = new QHBoxLayout;
    btnRow->addStretch(1);
    btnRow->addWidget(convertBtn);
    btnRow->addWidget(cancelBtn);
    layout->addLayout(btnRow);
}

void ConvertDialog::chooseInput()
{
    QString mode = m_modeCombo->currentData().toString();
    if (mode == "images_pdf") {
        QStringList paths = QFileDialog::getOpenFileNames(
            this, QStringLiteral("Select images"), QString(),
            QStringLiteral("Images (*.png *.jpg *.jpeg *.bmp)"));
        if (!paths.isEmpty()) {
            m_inputPaths = paths;
            m_inputLabel->setText(QStringLiteral("Input: %1 image(s)").arg(paths.size()));
        }
    } else {
        QString path = QFileDialog::getOpenFileName(
            this, QStringLiteral("Select text file"), QString(),
            QStringLiteral("Text (*.txt);;All (*.*)"));
        if (!path.isEmpty()) {
            m_inputPaths.clear();
            m_inputPaths << path;
            m_inputLabel->setText(QStringLiteral("Input: ") + QFileInfo(path).fileName());
        }
    }
}

void ConvertDialog::chooseOutput()
{
    QString mode = m_modeCombo->currentData().toString();
    QString filter = mode.contains("epub")
        ? QStringLiteral("EPUB (*.epub)")
        : QStringLiteral("PDF (*.pdf)");

    QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("Save file"), QString(), filter);
    if (!path.isEmpty()) {
        m_outputPath = path;
        m_outputLabel->setText(QStringLiteral("Output: ") + QFileInfo(path).fileName());
    }
}

void ConvertDialog::performConvert()
{
    if (m_inputPaths.isEmpty() || m_outputPath.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Error"),
                             QStringLiteral("Selection incomplete."));
        return;
    }

    QString mode = m_modeCombo->currentData().toString();
    QString pw   = m_passwordCheck->isChecked() ? m_passwordEdit->text() : QString();

    try {
        if (mode == "text_pdf")
            Converter::textToPdf(m_inputPaths[0], m_outputPath, pw);
        else if (mode == "text_epub")
            Converter::textToEpub(m_inputPaths[0], m_outputPath);
        else if (mode == "images_pdf")
            Converter::imagesToPdf(m_inputPaths, m_outputPath, pw);

        QMessageBox::information(this, QStringLiteral("Success"),
                                 QStringLiteral("Conversion completed."));
        accept();
    } catch (const std::exception &e) {
        QMessageBox::critical(this, QStringLiteral("Error"),
                              QString::fromStdString(e.what()));
    }
}
