#include <QTest>
#include <QWidget>

#include <YJDragGrid/DragGridLayout.h>

using YJDragGrid::DragGridLayout;

class TestDragGridLayout : public QObject
{
    Q_OBJECT

private:
    // 在测试函数返回前手动清空布局，避免容器析构时 layout item 引用已删除的 widget。
    static void clearLayout(DragGridLayout *layout);

private slots:
    void initTestCase();

    void fillIncompleteRowEnabled_roundTripAndCompat();
    void fillIncompleteRowEnabled_stretchesSparseFirstRow();
    void fillIncompleteRowEnabled_stretchesLastRow();
    void fillIncompleteRowEnabled_keepsFullRowsUnchanged();
    void targetIndexAt_usesStretchedLastRowGeometry();
    void moveItem_reordersWidgets();
    void moveItem_invalidFrom_returnsFalse();
    void targetIndexAt_mapsPositionsReasonably();
    void targetIndexAt_compensatesIgnoredSlotWhenDraggingRightward();
    void targetIndexAt_allowsAppendAfterLastCell();
    void placeholderIndex_boundsAreRespected();
    void ignoredWidget_isSkippedInLayoutGeometry();
    void placeholderIndex_withoutIgnoredWidgetDoesNotReserveSlot();
    void placeholderRect_usesStretchedIncompleteRow();
    void externalDelete_removesItemAndKeepsLayoutUsable();
    void addItem_managesWidgetLifecycle();
    void targetIndexAt_compensatesByCurrentPlaceholderIndex();
    void minimumCellSize_reflectsRuntimeWidgetMinimumSize();
    void minimumSize_widthCoversOccupiedColumns();
};

void TestDragGridLayout::clearLayout(DragGridLayout *layout)
{
    if (!layout) {
        return;
    }

    while (layout->count() > 0) {
        QLayoutItem *item = layout->takeAt(0);
        if (item) {
            QWidget *w = item->widget();
            delete item;
            delete w;
        }
    }
}

void TestDragGridLayout::initTestCase()
{
    // Qt Test 通过 QTEST_MAIN 已创建 QApplication，此处无需额外初始化。
}

namespace {

QWidget *createLayoutItem()
{
    auto *widget = new QWidget;
    widget->setMinimumSize(100, 100);
    return widget;
}

void addItems(DragGridLayout *layout, int count)
{
    for (int index = 0; index < count; ++index) {
        layout->addWidget(createLayoutItem());
    }
}

} // namespace

void TestDragGridLayout::fillIncompleteRowEnabled_roundTripAndCompat()
{
    DragGridLayout layout;

    QVERIFY(!layout.fillIncompleteRowEnabled());
    QVERIFY(!layout.compactWhenSparseEnabled());

    layout.setFillIncompleteRowEnabled(true);
    QVERIFY(layout.fillIncompleteRowEnabled());
    QVERIFY(layout.compactWhenSparseEnabled());

    layout.setCompactWhenSparseEnabled(false);
    QVERIFY(!layout.fillIncompleteRowEnabled());
    QVERIFY(!layout.compactWhenSparseEnabled());
}

void TestDragGridLayout::fillIncompleteRowEnabled_stretchesSparseFirstRow()
{
    QWidget container;
    auto *layout = new DragGridLayout(&container);
    layout->setColumnCount(4);
    layout->setMinimumCellSize(QSize(100, 100));
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setFillIncompleteRowEnabled(true);
    container.setLayout(layout);
    container.setGeometry(0, 0, 400, 100);

    addItems(layout, 2);
    layout->activate();

    QCOMPARE(layout->itemAt(0)->widget()->geometry(), QRect(0, 0, 200, 100));
    QCOMPARE(layout->itemAt(1)->widget()->geometry(), QRect(200, 0, 200, 100));

    clearLayout(layout);
}

void TestDragGridLayout::fillIncompleteRowEnabled_stretchesLastRow()
{
    QWidget container;
    auto *layout = new DragGridLayout(&container);
    layout->setColumnCount(4);
    layout->setMinimumCellSize(QSize(100, 100));
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setFillIncompleteRowEnabled(true);
    container.setLayout(layout);
    container.setGeometry(0, 0, 400, 200);

    addItems(layout, 6);
    layout->activate();

    QCOMPARE(layout->itemAt(4)->widget()->geometry(), QRect(0, 100, 200, 100));
    QCOMPARE(layout->itemAt(5)->widget()->geometry(), QRect(200, 100, 200, 100));

    clearLayout(layout);
}

void TestDragGridLayout::fillIncompleteRowEnabled_keepsFullRowsUnchanged()
{
    QWidget container;
    auto *layout = new DragGridLayout(&container);
    layout->setColumnCount(4);
    layout->setMinimumCellSize(QSize(100, 100));
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setFillIncompleteRowEnabled(true);
    container.setLayout(layout);
    container.setGeometry(0, 0, 400, 200);

    addItems(layout, 8);
    layout->activate();

    QCOMPARE(layout->itemAt(4)->widget()->geometry(), QRect(0, 100, 100, 100));
    QCOMPARE(layout->itemAt(7)->widget()->geometry(), QRect(300, 100, 100, 100));

    clearLayout(layout);
}

void TestDragGridLayout::targetIndexAt_usesStretchedLastRowGeometry()
{
    QWidget container;
    auto *layout = new DragGridLayout(&container);
    layout->setColumnCount(4);
    layout->setMinimumCellSize(QSize(100, 100));
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setFillIncompleteRowEnabled(true);
    container.setLayout(layout);
    container.setGeometry(0, 0, 400, 200);

    addItems(layout, 6);
    layout->activate();

    QCOMPARE(layout->targetIndexAt(QPoint(40, 150)), 4);
    QCOMPARE(layout->targetIndexAt(QPoint(160, 150)), 5);
    QCOMPARE(layout->targetIndexAt(QPoint(240, 150)), 5);
    QCOMPARE(layout->targetIndexAt(QPoint(399, 150)), 5);

    clearLayout(layout);
}

void TestDragGridLayout::moveItem_reordersWidgets()
{
    DragGridLayout layout;
    layout.setColumnCount(4);

    auto *w1 = new QWidget;
    auto *w2 = new QWidget;
    auto *w3 = new QWidget;
    layout.addWidget(w1);
    layout.addWidget(w2);
    layout.addWidget(w3);

    QCOMPARE(layout.indexOf(w1), 0);
    QCOMPARE(layout.indexOf(w2), 1);
    QCOMPARE(layout.indexOf(w3), 2);

    QVERIFY(layout.moveItem(0, 2)); // w1 移动到 w3 之后
    QCOMPARE(layout.indexOf(w2), 0);
    QCOMPARE(layout.indexOf(w3), 1);
    QCOMPARE(layout.indexOf(w1), 2);

    clearLayout(&layout);
}

void TestDragGridLayout::moveItem_invalidFrom_returnsFalse()
{
    DragGridLayout layout;
    auto *w1 = new QWidget;
    layout.addWidget(w1);

    QVERIFY(!layout.moveItem(-1, 0));
    QVERIFY(!layout.moveItem(5, 0));
    QVERIFY(!layout.moveItem(0, 0)); // 相同位置

    clearLayout(&layout);
}

void TestDragGridLayout::targetIndexAt_mapsPositionsReasonably()
{
    QWidget container;
    auto *layout = new DragGridLayout(&container);
    layout->setColumnCount(2);
    layout->setMinimumCellSize(QSize(100, 100));
    layout->setSpacing(0);
    container.setLayout(layout);
    container.setGeometry(0, 0, 200, 200);
    container.setMinimumSize(200, 200);

    auto *w1 = new QWidget;
    auto *w2 = new QWidget;
    auto *w3 = new QWidget;
    layout->addWidget(w1);
    layout->addWidget(w2);
    layout->addWidget(w3);

    layout->activate();

    // (50, 50) 位于第一个单元格
    QCOMPARE(layout->targetIndexAt(QPoint(50, 50)), 0);

    // (150, 50) 位于第二个单元格
    QCOMPARE(layout->targetIndexAt(QPoint(150, 50)), 1);

    // (50, 150) 位于第二行第一个
    QCOMPARE(layout->targetIndexAt(QPoint(50, 150)), 2);

    // 超出边界应被 clamp
    QVERIFY(layout->targetIndexAt(QPoint(999, 999)) >= 0);

    clearLayout(layout);
}

void TestDragGridLayout::targetIndexAt_compensatesIgnoredSlotWhenDraggingRightward()
{
    // 特征测试：向右拖拽时被拖项的占位槽被保留，占位槽右侧项目已左移一格，
    // 光标下的静态单元格索引与插入位错位 +1。
    // 复现修复前故障：拖 w1 悬停 w2 单元格左半区快速释放，落点过冲成 [w2,w1,w3]。
    QWidget container;
    auto *layout = new DragGridLayout(&container);
    layout->setColumnCount(3);
    layout->setMinimumCellSize(QSize(100, 100));
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    container.setLayout(layout);
    container.setGeometry(0, 0, 300, 100);
    container.setMinimumSize(300, 100);

    auto *w1 = new QWidget;
    auto *w2 = new QWidget;
    auto *w3 = new QWidget;
    layout->addWidget(w1);
    layout->addWidget(w2);
    layout->addWidget(w3);
    layout->activate();

    // 向右拖：w1 被忽略，静态槽位 1/2 上的项目实际是 w2/w3。
    layout->setIgnoredWidget(w1);
    layout->activate();
    // w2 左半区 → 插在 w2 之前 → 顺序不变
    QCOMPARE(layout->targetIndexAt(QPoint(140, 50)), 0);
    // w2 右半区 → 插在 w2 之后
    QCOMPARE(layout->targetIndexAt(QPoint(160, 50)), 1);
    // w3 左半区 → 插在 w3 之前
    QCOMPARE(layout->targetIndexAt(QPoint(240, 50)), 1);
    // w3 右半区与越界 → 追加
    QCOMPARE(layout->targetIndexAt(QPoint(260, 50)), 2);
    QCOMPARE(layout->targetIndexAt(QPoint(350, 50)), 2);

    // 向左拖：被拖项右侧无保留槽，无需补偿。
    layout->setIgnoredWidget(w3);
    layout->activate();
    // w1 左半区 → 插在 w1 之前
    QCOMPARE(layout->targetIndexAt(QPoint(40, 50)), 0);
    // w2 左半区 → 插在 w2 之前
    QCOMPARE(layout->targetIndexAt(QPoint(140, 50)), 1);
    // w2 右半区 → 追加（w3 是被拖项，插在 w2 之后即末尾）
    QCOMPARE(layout->targetIndexAt(QPoint(160, 50)), 2);

    layout->setIgnoredWidget(nullptr);
    clearLayout(layout);
}

void TestDragGridLayout::targetIndexAt_allowsAppendAfterLastCell()
{
    // 特征测试：悬停最后一个单元格的右半区应表示"追加"，即使无拖拽进行。
    // 复现修复前故障：返回值被 clamp 到 itemCount-1，末位右半区与左半区语义相同。
    QWidget container;
    auto *layout = new DragGridLayout(&container);
    layout->setColumnCount(2);
    layout->setMinimumCellSize(QSize(100, 100));
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    container.setLayout(layout);
    container.setGeometry(0, 0, 200, 100);
    container.setMinimumSize(200, 100);

    auto *w1 = new QWidget;
    auto *w2 = new QWidget;
    layout->addWidget(w1);
    layout->addWidget(w2);
    layout->activate();

    // 末位单元格左半区 → 插在 w2 之前；右半区 → 插在 w2 之后（追加）
    QCOMPARE(layout->targetIndexAt(QPoint(40, 50)), 0);
    QCOMPARE(layout->targetIndexAt(QPoint(160, 50)), 1);

    clearLayout(layout);
}

void TestDragGridLayout::placeholderIndex_boundsAreRespected()
{
    DragGridLayout layout;
    auto *w1 = new QWidget;
    layout.addWidget(w1);

    layout.setPlaceholderIndex(0);
    QCOMPARE(layout.placeholderIndex(), 0);

    layout.setPlaceholderIndex(100);
    QCOMPARE(layout.placeholderIndex(), 0); // 被限制在 size-1

    layout.setPlaceholderIndex(-1);
    QCOMPARE(layout.placeholderIndex(), -1);

    clearLayout(&layout);
}

void TestDragGridLayout::ignoredWidget_isSkippedInLayoutGeometry()
{
    QWidget container;
    auto *layout = new DragGridLayout(&container);
    layout->setColumnCount(2);
    layout->setMinimumCellSize(QSize(100, 100));
    layout->setSpacing(0);
    container.setLayout(layout);
    container.setGeometry(0, 0, 200, 100);
    container.setMinimumSize(200, 100);

    auto *w1 = new QWidget;
    auto *w2 = new QWidget;
    layout->addWidget(w1);
    layout->addWidget(w2);

    layout->activate();

    // 默认情况下 w1 在第一个单元格，w2 在第二个单元格
    const QPoint w1Pos = w1->geometry().topLeft();
    QCOMPARE(w2->geometry().topLeft(), w1Pos + QPoint(100, 0));

    // 将 w1 设为被忽略项，布局应跳过它，w2 应占据第一个位置
    layout->setIgnoredWidget(w1);
    layout->activate();

    QCOMPARE(w2->geometry().topLeft(), w1Pos);

    layout->setIgnoredWidget(nullptr);
    clearLayout(layout);
}

void TestDragGridLayout::placeholderIndex_withoutIgnoredWidgetDoesNotReserveSlot()
{
    QWidget container;
    auto *layout = new DragGridLayout(&container);
    layout->setColumnCount(4);
    layout->setMinimumCellSize(QSize(100, 100));
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setFillIncompleteRowEnabled(true);
    container.setLayout(layout);
    container.setGeometry(0, 0, 400, 200);

    addItems(layout, 6);
    layout->setPlaceholderIndex(5);
    layout->activate();

    QCOMPARE(layout->itemAt(4)->widget()->geometry(), QRect(0, 100, 200, 100));
    QCOMPARE(layout->itemAt(5)->widget()->geometry(), QRect(200, 100, 200, 100));

    layout->setPlaceholderIndex(-1);
    clearLayout(layout);
}

void TestDragGridLayout::placeholderRect_usesStretchedIncompleteRow()
{
    QWidget container;
    auto *layout = new DragGridLayout(&container);
    layout->setColumnCount(4);
    layout->setMinimumCellSize(QSize(100, 100));
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setFillIncompleteRowEnabled(true);
    container.setLayout(layout);
    container.setGeometry(0, 0, 400, 200);

    addItems(layout, 6);
    layout->setIgnoredWidget(layout->itemAt(0)->widget());
    layout->setPlaceholderIndex(4);
    layout->activate();

    QCOMPARE(layout->placeholderRectAt(4), QRect(0, 100, 200, 100));

    layout->setIgnoredWidget(nullptr);
    layout->setPlaceholderIndex(-1);
    clearLayout(layout);
}

void TestDragGridLayout::externalDelete_removesItemAndKeepsLayoutUsable()
{
    // 特征测试：外部直接 delete 受管控件时，布局须自动摘除布局项，
    // 后续布局计算不得再解引用已销毁的控件。
    QWidget container;
    auto *layout = new DragGridLayout(&container);
    layout->setColumnCount(2);
    layout->setMinimumCellSize(QSize(100, 100));
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    container.setLayout(layout);
    container.setGeometry(0, 0, 200, 100);

    auto *w1 = createLayoutItem();
    auto *w2 = createLayoutItem();
    layout->addWidget(w1);
    layout->addWidget(w2);
    layout->activate();
    QCOMPARE(layout->count(), 2);

    delete w1; // 不经过 takeWidget()/takeAt() 的外部销毁

    QCOMPARE(layout->count(), 1);
    QCOMPARE(layout->indexOf(w2), 0);

    layout->activate();
    QCOMPARE(w2->geometry().topLeft(), QPoint(0, 0));

    clearLayout(layout);
}

void TestDragGridLayout::addItem_managesWidgetLifecycle()
{
    // 特征测试：经基类 QLayout::addWidget()（非虚函数，走 addItem）加入的
    // 控件必须与 insertWidget 路径一样受生命周期管理。
    // 复现修复前故障：外部销毁后布局项残留，count() 虚高、indexOf 命中悬垂项。
    DragGridLayout layout;
    auto *managed = new QWidget;
    layout.addWidget(managed); // 本类 addWidget → insertWidget，已受管

    auto *viaBase = new QWidget;
    static_cast<QLayout *>(&layout)->addWidget(viaBase); // 非虚调用 → addItem
    QCOMPARE(layout.count(), 2);
    QCOMPARE(layout.indexOf(viaBase), 1);
    QCOMPARE(layout.widgets().size(), 2);

    delete viaBase; // 不经过 takeWidget()/takeAt() 的外部销毁

    QCOMPARE(layout.count(), 1);
    QCOMPARE(layout.indexOf(viaBase), -1);
    QCOMPARE(layout.widgets().size(), 1);
    QCOMPARE(layout.indexOf(managed), 0);

    clearLayout(&layout);
}

void TestDragGridLayout::targetIndexAt_compensatesByCurrentPlaceholderIndex()
{
    // 特征测试：拖拽稳态（占位索引 ≠ 被拖项原始索引）下，落点必须等于
    // "光标左侧已渲染项数"，补偿基准是被保留的占位槽（当前占位索引），
    // 而不是被拖项的原始索引。
    // 复现修复前故障：按原始索引补偿——占位符右移后，悬停占位格左半区
    // 会把占位符从光标下向左挤走一格（悬停左半区/右半区行为不对称）；
    // 占位符左移后悬停项目左半区落点过冲一位。
    QWidget container;
    auto *layout = new DragGridLayout(&container);
    layout->setColumnCount(3);
    layout->setMinimumCellSize(QSize(100, 100));
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    container.setLayout(layout);
    container.resize(360, 140);

    auto *first = createLayoutItem();
    auto *second = createLayoutItem();
    auto *third = createLayoutItem();
    first->setParent(&container);
    second->setParent(&container);
    third->setParent(&container);
    layout->addWidget(first);
    layout->addWidget(second);
    layout->addWidget(third);
    const int y = 70; // 单行内任意 y

    // 拖拽起始态（占位 0 = 被拖项原始索引）：快速释放路径的回归保障。
    layout->setIgnoredWidget(first);
    layout->setPlaceholderIndex(0);
    layout->activate();
    QCOMPARE(layout->targetIndexAt(QPoint(150, y)), 0); // second 格左半区：插在 second 前
    QCOMPARE(layout->targetIndexAt(QPoint(400, y)), 2); // 越过右边界：追加到末尾

    // 稳态（向右拖）：first 越过 second，占位 1，second 已移入 0 号格，
    // 占位格即 second 的原槽位 [120, 240)。
    layout->setPlaceholderIndex(1);
    layout->activate();
    QCOMPARE(layout->targetIndexAt(QPoint(150, y)), 1); // 占位格左半区：原地不动
    QCOMPARE(layout->targetIndexAt(QPoint(30, y)), 0);  // second 新位置左半区：插其前

    // 稳态（向左拖）：third 越过 first、second，占位 0，first/second 右移一格。
    layout->setIgnoredWidget(third);
    layout->setPlaceholderIndex(0);
    layout->activate();
    QCOMPARE(layout->targetIndexAt(QPoint(150, y)), 0); // first 新位置左半区：插其前
    QCOMPARE(layout->targetIndexAt(QPoint(270, y)), 1); // second 新位置左半区：插其前
}

void TestDragGridLayout::minimumCellSize_reflectsRuntimeWidgetMinimumSize()
{
    // 特征测试：运行期修改卡片 minimumSize 后，布局必须重新计算最小单元格。
    // 修复前该值来自缓存，而失效通知（LayoutRequest）发送给父控件而非控件自身，
    // 卡片自身尺寸变化不会让缓存失效，布局会继续按旧尺寸压缩卡片。
    QWidget container;
    auto *layout = new DragGridLayout(&container);
    layout->setColumnCount(2);
    layout->setMinimumCellSize(QSize(50, 50));
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    container.setLayout(layout);
    container.resize(400, 400);

    auto *card = new QWidget;
    card->setMinimumSize(100, 100);
    layout->addWidget(card);
    layout->activate();

    QCOMPARE(layout->minimumSize(), QSize(100, 100)); // 单项按单列计算

    card->setMinimumSize(250, 250);
    QCoreApplication::processEvents();
    layout->activate();

    QCOMPARE(layout->minimumSize(), QSize(250, 250));
    QVERIFY(card->width() >= 250);
    QVERIFY(card->height() >= 250);

    clearLayout(layout);
}

void TestDragGridLayout::minimumSize_widthCoversOccupiedColumns()
{
    // 特征测试：最小宽度口径 min(列数, 项数) 等于项目实际占用的列数，
    // 最小宽度下所有槽位完整可见，与高度口径 ceil(项数/列数) 自洽。
    // 顺带钉住"允许压缩"语义：稀疏网格可以收缩到实际占用列数，
    // 均匀与拉伸（不完整行填满）两种模式都不得横向溢出。
    QWidget container;
    auto *layout = new DragGridLayout(&container);
    layout->setColumnCount(4);
    layout->setMinimumCellSize(QSize(100, 100));
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    container.setLayout(layout);

    auto *w1 = createLayoutItem();
    auto *w2 = createLayoutItem();
    layout->addWidget(w1);
    layout->addWidget(w2);

    QCOMPARE(layout->minimumSize().width(), 200); // min(4, 2) 列

    container.resize(200, 100); // 压到最小宽度
    layout->activate();
    // 两项分别占槽位 0/1，都必须完整落在 200 宽内（无横向溢出）。
    QCOMPARE(w1->geometry(), QRect(0, 0, 100, 100));
    QCOMPARE(w2->geometry(), QRect(100, 0, 100, 100));

    // 拉伸模式同样不允许溢出：宽度不足时回退均匀单元格。
    layout->setFillIncompleteRowEnabled(true);
    layout->activate();
    QCOMPARE(w1->geometry(), QRect(0, 0, 100, 100));
    QCOMPARE(w2->geometry(), QRect(100, 0, 100, 100));

    clearLayout(layout);
}

QTEST_MAIN(TestDragGridLayout)

#include "TestDragGridLayout.moc"
