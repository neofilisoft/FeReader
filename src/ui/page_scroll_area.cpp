#include "page_scroll_area.h"
#include <QWheelEvent>
#include <QScrollBar>

PageScrollArea::PageScrollArea(QWidget *parent)
    : QScrollArea(parent)
{}

void PageScrollArea::wheelEvent(QWheelEvent *event)
{
    if (onScrollPrev || onScrollNext) {
        int delta = event->angleDelta().y();
        QScrollBar *bar = verticalScrollBar();
        bool atTop    = bar->value() == bar->minimum();
        bool atBottom = bar->value() == bar->maximum();

        if (delta > 0 && atTop && onScrollPrev) {
            onScrollPrev();
            return;
        }
        if (delta < 0 && atBottom && onScrollNext) {
            onScrollNext();
            return;
        }
    }
    QScrollArea::wheelEvent(event);
}
