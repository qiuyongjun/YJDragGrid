#ifndef GRIDDRAGCONTROLLER_H
#define GRIDDRAGCONTROLLER_H

#include <QObject>
#include <QPoint>
#include <QSize>

class QWidget;

class GridDragController : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(GridDragController)

public:
    struct DragVisualState {
        QSize normalSize;
        QSize draggedSize;
    };

    explicit GridDragController(QObject *parent = nullptr);

    void beginDrag(QWidget *widget, const QPoint &pointOffset, const DragVisualState &visualState);
    void updateDragPosition(const QPoint &widgetPos);
    void endDrag();

    bool isDragging() const;
    QWidget *draggedWidget() const;
    QPoint dragPointOffset() const;
    DragVisualState visualState() const;

private:
    QWidget *m_draggedWidget = nullptr;
    QPoint m_dragPointOffset;
    DragVisualState m_visualState;
};

#endif // GRIDDRAGCONTROLLER_H
