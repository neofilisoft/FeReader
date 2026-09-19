#pragma once
#include <QScrollArea>
#include <functional>

// QScrollArea subclass that detects edge-scroll to trigger page navigation.
// Matches Python PageScrollArea exactly.
class PageScrollArea : public QScrollArea
{
    Q_OBJECT
public:
    explicit PageScrollArea(QWidget *parent = nullptr);

    std::function<void()> onScrollPrev;
    std::function<void()> onScrollNext;

protected:
    void wheelEvent(QWheelEvent *event) override;
};
