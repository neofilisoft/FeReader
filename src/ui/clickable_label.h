#pragma once
#include <QLabel>

// QLabel subclass that emits clicked() on left mouse press.
// Matches Python ClickableLabel exactly.
class ClickableLabel : public QLabel
{
    Q_OBJECT
public:
    explicit ClickableLabel(const QString &text = QString(), QWidget *parent = nullptr);

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent *event) override;
};
