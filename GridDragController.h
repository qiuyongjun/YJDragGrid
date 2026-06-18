#ifndef GRIDDRAGCONTROLLER_H
#define GRIDDRAGCONTROLLER_H

#include <QPoint>
#include <QSize>

class QWidget;
class QScrollArea;

class GridDragController
{
public:
    struct DragVisualState {
        QSize normalSize;
        QSize draggedSize;
    };

    GridDragController();

    // 绑定滚动环境，供拖拽自动滚动使用。
    void setScrollArea(QScrollArea *scrollArea);

    // 配置占位索引与拖拽中控件状态。
    void beginDrag(QWidget *widget, const QPoint &pointOffset, const DragVisualState &visualState);
    void updateDragPosition(const QPoint &widgetPos);
    void updatePlaceholderIndex(int index);
    void endDrag();

    // 计算拖拽目标索引，统一边界裁剪。
    int calculatePlaceholderIndex(const QPoint &widgetPos,
                                  const QSize &cellSize,
                                  const QPoint &contentOrigin,
                                  int spacing,
                                  int columnCount,
                                  int itemCount) const;

    // 执行自动滚动。
    void autoScroll() const;

    bool isDragging() const;
    QWidget *draggedWidget() const;
    QPoint dragPointOffset() const;
    int placeholderIndex() const;
    DragVisualState visualState() const;

private:
    QWidget *m_draggedWidget = nullptr;
    QPoint m_dragPointOffset;
    int m_placeholderIndex = -1;
    QScrollArea *m_scrollArea = nullptr;
    DragVisualState m_visualState;
};

#endif // GRIDDRAGCONTROLLER_H
