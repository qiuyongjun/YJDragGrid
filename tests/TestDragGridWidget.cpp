#include <QTest>
#include <QLabel>
#include <QLayout>
#include <QSignalSpy>
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
    void mouseDrag_reordersWidgets();
    void keyboardDrag_reordersWidgets();
    void escapeDuringDrag_restoresOriginalOrder();
    void deleteWidget_duringDrag_removesDraggedWidget();
    void setDragEnabled_falseDuringDrag_cancelsDrag();
    void zeroAnimationDuration_reordersDirectly();
};

namespace {

QWidget *createItem(const QString &name)
{
    auto *widget = new QWidget;
    widget->setObjectName(name);
    widget->setMinimumSize(100, 100);
    widget->resize(100, 100);
    return widget;
}

void prepareGrid(DragGridWidget *grid, QWidget *first, QWidget *second, QWidget *third = nullptr)
{
    grid->setColumnCount(3);
    grid->setMinimumCellSize(QSize(100, 100));
    grid->setAnimationDuration(0);
    grid->resize(360, 140);
    grid->show();
    QVERIFY(QTest::qWaitForWindowExposed(grid));

    grid->addWidget(first);
    grid->addWidget(second);
    if (third) {
        grid->addWidget(third);
    }
    grid->layout()->activate();
    QCoreApplication::processEvents();
}

QStringList objectNames(const QList<QWidget *> &widgets)
{
    QStringList names;
    for (QWidget *widget : widgets) {
        names << widget->objectName();
    }
    return names;
}

void dragFromTo(DragGridWidget *grid, const QPoint &from, const QPoint &to)
{
    QTest::mousePress(grid, Qt::LeftButton, Qt::NoModifier, from);
    QTest::mouseMove(grid, from + QPoint(30, 0));
    QTest::mouseMove(grid, to);
    QTest::mouseRelease(grid, Qt::LeftButton, Qt::NoModifier, to);
    QCoreApplication::processEvents();
}

} // namespace

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

void TestDragGridWidget::mouseDrag_reordersWidgets()
{
    DragGridWidget grid;
    auto *first = createItem(QStringLiteral("first"));
    auto *second = createItem(QStringLiteral("second"));
    auto *third = createItem(QStringLiteral("third"));
    prepareGrid(&grid, first, second, third);
    grid.setDragEnabled(true);
    QSignalSpy orderSpy(&grid, &DragGridWidget::orderChanged);

    dragFromTo(&grid, first->geometry().center(), third->geometry().center() + QPoint(60, 0));

    QCOMPARE(objectNames(grid.widgets()), QStringList({QStringLiteral("second"),
                                                       QStringLiteral("third"),
                                                       QStringLiteral("first")}));
    QCOMPARE(orderSpy.count(), 1);
    QVERIFY(first->isVisible());
}

void TestDragGridWidget::keyboardDrag_reordersWidgets()
{
    DragGridWidget grid;
    auto *first = createItem(QStringLiteral("first"));
    auto *second = createItem(QStringLiteral("second"));
    prepareGrid(&grid, first, second);
    grid.setDragEnabled(true);
    first->setFocus();
    QSignalSpy orderSpy(&grid, &DragGridWidget::orderChanged);

    QTest::keyClick(&grid, Qt::Key_Space);
    QTest::keyClick(&grid, Qt::Key_Right);
    QTest::keyClick(&grid, Qt::Key_Return);
    QCoreApplication::processEvents();

    QCOMPARE(objectNames(grid.widgets()), QStringList({QStringLiteral("second"),
                                                       QStringLiteral("first")}));
    QCOMPARE(orderSpy.count(), 1);
    QVERIFY(first->isVisible());
}

void TestDragGridWidget::escapeDuringDrag_restoresOriginalOrder()
{
    DragGridWidget grid;
    auto *first = createItem(QStringLiteral("first"));
    auto *second = createItem(QStringLiteral("second"));
    auto *third = createItem(QStringLiteral("third"));
    prepareGrid(&grid, first, second, third);
    grid.setDragEnabled(true);
    QSignalSpy orderSpy(&grid, &DragGridWidget::orderChanged);

    QTest::mousePress(&grid, Qt::LeftButton, Qt::NoModifier, first->geometry().center());
    QTest::mouseMove(&grid, third->geometry().center() + QPoint(60, 0));
    QTest::keyClick(&grid, Qt::Key_Escape);
    QCoreApplication::processEvents();

    QCOMPARE(objectNames(grid.widgets()), QStringList({QStringLiteral("first"),
                                                       QStringLiteral("second"),
                                                       QStringLiteral("third")}));
    QCOMPARE(orderSpy.count(), 0);
    QVERIFY(first->isVisible());
}

void TestDragGridWidget::deleteWidget_duringDrag_removesDraggedWidget()
{
    DragGridWidget grid;
    auto *first = createItem(QStringLiteral("first"));
    auto *second = createItem(QStringLiteral("second"));
    prepareGrid(&grid, first, second);
    grid.setDragEnabled(true);
    QSignalSpy orderSpy(&grid, &DragGridWidget::orderChanged);

    QTest::mousePress(&grid, Qt::LeftButton, Qt::NoModifier, first->geometry().center());
    QTest::mouseMove(&grid, second->geometry().center() + QPoint(60, 0));
    grid.deleteWidget(first);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();

    QCOMPARE(objectNames(grid.widgets()), QStringList({QStringLiteral("second")}));
    QCOMPARE(orderSpy.count(), 0);
    QVERIFY(second->isVisible());
}

void TestDragGridWidget::setDragEnabled_falseDuringDrag_cancelsDrag()
{
    DragGridWidget grid;
    auto *first = createItem(QStringLiteral("first"));
    auto *second = createItem(QStringLiteral("second"));
    auto *third = createItem(QStringLiteral("third"));
    prepareGrid(&grid, first, second, third);
    grid.setDragEnabled(true);
    QSignalSpy orderSpy(&grid, &DragGridWidget::orderChanged);

    QTest::mousePress(&grid, Qt::LeftButton, Qt::NoModifier, first->geometry().center());
    QTest::mouseMove(&grid, third->geometry().center() + QPoint(60, 0));
    grid.setDragEnabled(false);
    QCoreApplication::processEvents();

    QCOMPARE(objectNames(grid.widgets()), QStringList({QStringLiteral("first"),
                                                       QStringLiteral("second"),
                                                       QStringLiteral("third")}));
    QCOMPARE(orderSpy.count(), 0);
    QVERIFY(!grid.dragEnabled());
    QVERIFY(first->isVisible());
}

void TestDragGridWidget::zeroAnimationDuration_reordersDirectly()
{
    DragGridWidget grid;
    auto *first = createItem(QStringLiteral("first"));
    auto *second = createItem(QStringLiteral("second"));
    prepareGrid(&grid, first, second);
    grid.setDragEnabled(true);
    grid.setAnimationDuration(0);

    dragFromTo(&grid, first->geometry().center(), second->geometry().center() + QPoint(60, 0));

    QCOMPARE(objectNames(grid.widgets()), QStringList({QStringLiteral("second"),
                                                       QStringLiteral("first")}));
    QCOMPARE(first->geometry().top(), second->geometry().top());
    QVERIFY(first->geometry().left() > second->geometry().left());
}

QTEST_MAIN(TestDragGridWidget)

#include "TestDragGridWidget.moc"
