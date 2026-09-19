#pragma once
#include <QDialog>
#include <QString>
#include <QStringList>

class QComboBox;
class QLabel;
class QLineEdit;
class QCheckBox;

// File conversion dialog: Text->PDF, Text->EPUB, Images->PDF.
// Matches Python ConvertDialog exactly.
class ConvertDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ConvertDialog(QWidget *parent, const QString &currentLang);

private slots:
    void chooseInput();
    void chooseOutput();
    void performConvert();

private:
    QComboBox  *m_modeCombo     = nullptr;
    QLabel     *m_inputLabel    = nullptr;
    QLabel     *m_outputLabel   = nullptr;
    QCheckBox  *m_passwordCheck = nullptr;
    QLineEdit  *m_passwordEdit  = nullptr;

    QStringList m_inputPaths;
    QString     m_outputPath;
    QString     m_lang;
};
