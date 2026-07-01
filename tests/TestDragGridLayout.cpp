#include <QTest>
#include <QWidget>

#include <QtDragGrid/DragGridLayout.h>

using QtDragGrid::DragGridLayout;

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
    void placeholderIndex_boundsAreRespected();
    void ignoredWidget_isSkippedInLayoutGeometry();
    void placeholderIndex_withoutIgnoredWidgetDoesNotReserveSlot();
    void placeholderRect_usesStretchedIncompleteRow();
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

QTEST_MAIN(TestDragGridLayout)

#include "TestDragGridLayout.moc"
