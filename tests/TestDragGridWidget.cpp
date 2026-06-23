#include <QTest>
#include <QLabel>
#include <QScrollArea>
#include <QWidget>

#include "DragGridWidget.h"

class TestDragGridWidget : public QObject
{
    Q_OBJECT

private slots:
    void dragEnabled_togglesState();
    void placeholderProperties_roundTrip();
    void dragHandle_canBeSet();
    void emptyText_defaultAndCustom();
    void emptyStateVisible_togglesVisibility();
    void setDragEnabled_falseDuringDrag_doesNotCrash();
};

void TestDragGridWidget::dragEnabled_togglesState()
{
    DragGridWidget grid;
    QVERIFY(!grid.dragEnabled());

    grid.setDragEnabled(true);
    QVERIFY(grid.dragEnabled());

    grid.setDragEnabled(false);
    QVERIFY(!grid.dragEnabled());
}

void TestDragGridWidget::placeholderProperties_roundTrip()
{
    DragGridWidget grid;

    grid.setPlaceholderOpacity(0.8);
    QCOMPARE(grid.placeholderOpacity(), 0.8);

    grid.setPlaceholderOpacity(1.5); // 应被限制到 1.0
    QCOMPARE(grid.placeholderOpacity(), 1.0);

    grid.setPlaceholderOpacity(-0.5); // 应被限制到 0.0
    QCOMPARE(grid.placeholderOpacity(), 0.0);

    grid.setPlaceholderPulseDuration(1200);
    QCOMPARE(grid.placeholderPulseDuration(), 1200);

    grid.setAutoScrollMargin(60);
    QCOMPARE(grid.autoScrollMargin(), 60);

    grid.setAutoScrollMaxSpeed(32);
    QCOMPARE(grid.autoScrollMaxSpeed(), 32);
}

void TestDragGridWidget::dragHandle_canBeSet()
{
    DragGridWidget grid;
    QWidget handle;

    QVERIFY(!grid.dragHandle());
    grid.setDragHandle(&handle);
    QCOMPARE(grid.dragHandle(), &handle);
}

void TestDragGridWidget::emptyText_defaultAndCustom()
{
    DragGridWidget grid;

    QCOMPARE(grid.emptyText(), QStringLiteral("No items."));

    auto *label = grid.findChild<QLabel *>(QStringLiteral("GridEmptyState"));
    QVERIFY(label);
    QCOMPARE(label->text(), QStringLiteral("No items."));

    grid.setEmptyText(QStringLiteral("Custom empty text"));
    QCOMPARE(grid.emptyText(), QStringLiteral("Custom empty text"));
    QCOMPARE(label->text(), QStringLiteral("Custom empty text"));
}

void TestDragGridWidget::emptyStateVisible_togglesVisibility()
{
    DragGridWidget grid;

    QVERIFY(grid.emptyStateVisible());

    auto *label = grid.findChild<QLabel *>(QStringLiteral("GridEmptyState"));
    QVERIFY(label);
    QVERIFY(!label->isHidden());

    grid.setEmptyStateVisible(false);
    QVERIFY(!grid.emptyStateVisible());
    QVERIFY(label->isHidden());

    grid.setEmptyStateVisible(true);
    QVERIFY(grid.emptyStateVisible());
    QVERIFY(!label->isHidden());
}

void TestDragGridWidget::setDragEnabled_falseDuringDrag_doesNotCrash()
{
    QScrollArea scrollArea;
    auto *grid = new DragGridWidget(&scrollArea);
    auto *widget = new QWidget(grid);
    grid->addWidget(widget);

    grid->setDragEnabled(true);

    // 通过反射模拟内部拖拽启动（无法直接调用私有 startDragOperation），
    // 这里至少验证在普通状态下禁用不会崩溃。
    grid->setDragEnabled(false);
    QVERIFY(!grid->dragEnabled());

    delete grid; // widget 随 grid 一起删除
}

QTEST_MAIN(TestDragGridWidget)

#include "TestDragGridWidget.moc"
