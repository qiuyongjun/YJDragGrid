#ifndef DRAGGRIDLAYOUT_H
#define DRAGGRIDLAYOUT_H

#include <QHash>
#include <QList>
#include <QLayout>
#include <QPointer>
#include <QSize>

class QEvent;
class QPropertyAnimation;
class QWidget;

class DragGridLayout : public QLayout
{
    Q_DISABLE_COPY(DragGridLayout)

public:
    // 创建拖拽网格布局。
    explicit DragGridLayout(QWidget *parent = nullptr);
    ~DragGridLayout() override;

    // 添加布局项，所有权转移给布局。
    void addItem(QLayoutItem *item) override;
    void addWidget(QWidget *widget, Qt::Alignment alignment = {});
    void insertWidget(int index, QWidget *widget, Qt::Alignment alignment = {});

    QLayoutItem *itemAt(int index) const override;
    QLayoutItem *takeAt(int index) override;
    int count() const override;

    void setGeometry(const QRect &rect) override;
    QSize sizeHint() const override;
    QSize minimumSize() const override;
    bool hasHeightForWidth() const override;
    int heightForWidth(int width) const override;
    Qt::Orientations expandingDirections() const override;

    // 返回配置的最大列数。
    int columnCount() const;
    // 设置最大列数，非法值会被限制为至少 1 列。
    void setColumnCount(int columnCount);

    // 返回配置的单元格最小尺寸。
    QSize minimumCellSize() const;
    // 设置单元格最小尺寸，布局会同时考虑子控件最小尺寸。
    void setMinimumCellSize(const QSize &size);

    // 返回是否启用等宽单元格。
    bool equalCellSizeEnabled() const;
    // 设置是否按可用宽度扩展所有单元格。
    void setEqualCellSizeEnabled(bool enable);

    // 返回是否在项目较少时压缩列数。
    bool compactWhenSparseEnabled() const;
    // 设置稀疏场景是否按项目数收缩列数。
    void setCompactWhenSparseEnabled(bool enable);
    // 返回当前实际用于布局的列数。
    int effectiveColumnCount() const;

    // 返回拖拽中被布局临时忽略的控件。
    QWidget *ignoredWidget() const;
    // 设置拖拽中临时忽略的控件，由占位符补齐它的视觉位置。
    void setIgnoredWidget(QWidget *widget);

    // 返回布局几何动画时长。
    int animationDuration() const;
    // 设置布局几何动画时长，单位毫秒。
    void setAnimationDuration(int ms);

    // 返回当前占位符索引，-1 表示未显示占位符。
    int placeholderIndex() const;
    // 设置占位符索引，越界值会被限制到有效范围。
    void setPlaceholderIndex(int index);

    // 按当前顺序返回布局管理的控件。
    QList<QWidget *> widgets() const;
    // 返回控件在布局中的索引，未找到返回 -1。
    int indexOf(const QWidget *widget) const;
    // 取出指定索引控件并释放布局项，不销毁控件。
    QWidget *takeWidget(int index);
    // 移动布局项到目标索引，索引无效或未变化时返回 false。
    bool moveItem(int from, int to);
    // 返回指定视觉索引对应的单元格矩形。
    QRect cellRectForIndex(int index, const QRect &contentsRect) const;

    // 根据鼠标位置计算目标占位索引（已考虑前后半区、首/尾/空白区域）。
    int targetIndexAt(const QPoint &pos) const;
    // 返回指定占位索引在当前布局中的几何矩形。
    QRect placeholderRectAt(int placeholderIndex) const;

private:
    struct Item {
        QLayoutItem *layoutItem = nullptr;
        Qt::Alignment alignment;
    };

    void invalidate() override;
    bool eventFilter(QObject *watched, QEvent *event) override;

    QSize effectiveCellSize(const QRect &contentRect) const;
    QSize minimumCellSizeForItems() const;
    int minimumColumnCount() const;
    int rowCount() const;
    QSize expandedSizeForItem(const Item &item, const QSize &cellSize) const;
    int clampedInsertIndex(int index) const;

    QRect cellRect(int index, const QRect &contentRect, int columns, const QSize &cellSize) const;

    static int computeTargetIndex(const QPoint &localPos, int columns,
                                  const QSize &cellSize, int layoutSpacing, int itemCount);

    void setWidgetGeometryAnimated(QWidget *widget, const QRect &target);
    void stopAnimationForWidget(QWidget *widget);

private:
    QList<Item> m_items;
    int m_columnCount = 4;
    QSize m_minimumCellSize = QSize(400, 300);
    bool m_equalCellSizeEnabled = true;
    bool m_compactWhenSparseEnabled = false;
    QWidget *m_ignoredWidget = nullptr;
    int m_placeholderIndex = -1;
    int m_animationDuration = 200;

    // 缓存：invalidate 时失效，惰性计算
    mutable QSize m_cachedMinCellSize;
    mutable bool m_minCellSizeDirty = true;

    // 控件几何过渡动画
    QHash<QWidget *, QPointer<QPropertyAnimation>> m_geometryAnimations;
};

#endif // DRAGGRIDLAYOUT_H
