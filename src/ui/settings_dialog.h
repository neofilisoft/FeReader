#pragma once
#include <QDialog>
#include <QString>

class QComboBox;
class QSpinBox;

// Settings dialog: Font, Size, Theme, Language.
// Matches Python SettingsDialog exactly.
class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent,
                            const QStringList &fonts,
                            const QString &currentFont,
                            int             currentSize,
                            const QString  &currentTheme,
                            const QString  &currentLang);

    struct Values {
        QString fontFamily;
        int     fontSize;
        QString theme;
        QString language;
    };

    Values getValues() const;

private:
    QComboBox *m_fontCombo  = nullptr;
    QSpinBox  *m_sizeSpin   = nullptr;
    QComboBox *m_themeCombo = nullptr;
    QComboBox *m_langCombo  = nullptr;
};
