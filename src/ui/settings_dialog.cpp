#include "settings_dialog.h"
#include "../core/i18n.h"

#include <QComboBox>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStringList>

static void addRow(QVBoxLayout *layout, const QString &labelText, QWidget *widget)
{
    QHBoxLayout *row = new QHBoxLayout;
    row->addWidget(new QLabel(labelText));
    row->addWidget(widget);
    layout->addLayout(row);
}

SettingsDialog::SettingsDialog(QWidget *parent,
                               const QStringList &fonts,
                               const QString &currentFont,
                               int             currentSize,
                               const QString  &currentTheme,
                               const QString  &currentLang)
    : QDialog(parent)
{
    setModal(true);
    setWindowTitle(I18n::tr("settings_title", currentLang));

    m_fontCombo = new QComboBox(this);
    m_fontCombo->addItems(fonts);
    if (fonts.contains(currentFont))
        m_fontCombo->setCurrentText(currentFont);

    m_sizeSpin = new QSpinBox(this);
    m_sizeSpin->setRange(8, 48);
    m_sizeSpin->setValue(currentSize);

    m_themeCombo = new QComboBox(this);
    m_themeCombo->addItem(I18n::tr("theme_light", currentLang));
    m_themeCombo->addItem(I18n::tr("theme_dark",  currentLang));
    m_themeCombo->setCurrentIndex(currentTheme.toLower() == "dark" ? 1 : 0);

    m_langCombo = new QComboBox(this);
    m_langCombo->addItem(I18n::tr("language_en", currentLang), QStringLiteral("en"));
    m_langCombo->addItem(I18n::tr("language_th", currentLang), QStringLiteral("th"));
    m_langCombo->setCurrentIndex(currentLang == "th" ? 1 : 0);

    QVBoxLayout *layout = new QVBoxLayout(this);
    addRow(layout, I18n::tr("font", currentLang) + ":",     m_fontCombo);
    addRow(layout, QStringLiteral("Size:"),                   m_sizeSpin);
    addRow(layout, I18n::tr("theme", currentLang) + ":",    m_themeCombo);
    addRow(layout, I18n::tr("language", currentLang) + ":", m_langCombo);

    QHBoxLayout *btnRow = new QHBoxLayout;
    QPushButton *ok     = new QPushButton(QStringLiteral("OK"),     this);
    QPushButton *cancel = new QPushButton(QStringLiteral("Cancel"), this);
    connect(ok,     &QPushButton::clicked, this, &QDialog::accept);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    btnRow->addStretch(1);
    btnRow->addWidget(ok);
    btnRow->addWidget(cancel);
    layout->addLayout(btnRow);
}

SettingsDialog::Values SettingsDialog::getValues() const
{
    return {
        m_fontCombo->currentText(),
        m_sizeSpin->value(),
        m_themeCombo->currentIndex() == 0 ? QStringLiteral("light") : QStringLiteral("dark"),
        m_langCombo->currentData().toString()
    };
}
