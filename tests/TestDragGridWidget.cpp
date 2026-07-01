#include <QTest>
#include <QLabel>
#include <QLayout>
#include <QMouseEvent>
#include <QSignalSpy>
#include <QScrollArea>
#include <QWidget>

#include <QtDragGrid/DragGridWidget.h>

using QtDragGrid::DragGridWidget;

class TestDragGridWidget : public QObject
{
    Q_OBJECT

private slots:
    void dragEnabled_togglesState();
    void fillIncompleteRowEnabled_roundTripAndCompat();
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
    void mouseDrag_smallMoveKeepsGhostAnchored();
    void mouseDrag_releaseAtOriginalIndexRestoresGeometry();
    void mouseDrag_reordersWithStretchedLastRow();
};

namespace {

QWidget *createItem(const QString &name)
{
    auto *widget = new QWidget;
    widget->setObjectName(name);
    widget->setMinimumSize(100, 100);
    widget->resize(100, 100);
    widget->setFocusPolicy(Qt::StrongFocus);
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
    // CI 的 offscreen 后端对 QTest::mouseMove 的按键状态处理不稳定，这里显式携带 LeftButton。
    auto sendMouse = [grid](QEvent::Type type, const QPoint &pos,
                            Qt::MouseButton button, Qt::MouseButtons buttons) {
        QMouseEvent event(type, pos, grid->mapToGlobal(pos), button, buttons, Qt::NoModifier);
        QCoreApplication::sendEvent(grid, &event);
    };

    grid->activateWindow();
    QCoreApplication::processEvents();

    sendMouse(QEvent::MouseButtonPress, from, Qt::LeftButton, Qt::LeftButton);
    sendMouse(QEvent::MouseMove, from + QPoint(30, 0), Qt::NoButton, Qt::LeftButton);
    QCoreApplication::processEvents();
    sendMouse(QEvent::MouseMove, to, Qt::NoButton, Qt::LeftButton);
    QCoreApplication::processEvents();
    sendMouse(QEvent::MouseButtonRelease, to, Qt::LeftButton, Qt::NoButton);
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

void TestDragGridWidget::fillIncompleteRowEnabled_roundTripAndCompat()
{
    DragGridWidget grid;

    QVERIFY(!grid.fillIncompleteRowEnabled());
    QVERIFY(!grid.compactWhenSparseEnabled());

    grid.setFillIncompleteRowEnabled(true);
    QVERIFY(grid.fillIncompleteRowEnabled());
    QVERIFY(grid.compactWhenSparseEnabled());

    grid.setCompactWhenSparseEnabled(false);
    QVERIFY(!grid.fillIncompleteRowEnabled());
    QVERIFY(!grid.compactWhenSparseEnabled());
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
    grid.activateWindow();
    first->setFocus();
    QCoreApplication::processEvents();
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
    // 目标点需越过 cell 中心，避免 Qt 5.15.2 offscreen 取整导致占位符判断落在原 cell。
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
    // 目标点需越过 cell 中心，避免 Qt 5.15.2 offscreen 取整导致占位符判断落在原 cell。
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
    // 目标点需越过 cell 中心，避免 Qt 5.15.2 offscreen 取整导致占位符判断落在原 cell。
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
    // 使用键盘拖拽验证 zero-animation 下的落位：鼠标拖拽在 Qt 5.15.2 offscreen
    // 上坐标不稳定，键盘拖拽只依赖占位符逻辑，不受鼠标取整影响。
    DragGridWidget grid;
    auto *first = createItem(QStringLiteral("first"));
    auto *second = createItem(QStringLiteral("second"));
    prepareGrid(&grid, first, second);
    grid.setDragEnabled(true);
    grid.setAnimationDuration(0);
    grid.activateWindow();
    first->setFocus();
    QCoreApplication::processEvents();

    QTest::keyClick(&grid, Qt::Key_Space);
    QTest::keyClick(&grid, Qt::Key_Right);
    QTest::keyClick(&grid, Qt::Key_Return);
    QCoreApplication::processEvents();

    QCOMPARE(objectNames(grid.widgets()), QStringList({QStringLiteral("second"),
                                                       QStringLiteral("first")}));
    QCOMPARE(first->geometry().top(), second->geometry().top());
    QVERIFY(first->geometry().left() > second->geometry().left());
}

void TestDragGridWidget::mouseDrag_smallMoveKeepsGhostAnchored()
{
    DragGridWidget grid;
    auto *first = createItem(QStringLiteral("first"));
    auto *second = createItem(QStringLiteral("second"));
    auto *third = createItem(QStringLiteral("third"));
    prepareGrid(&grid, first, second, third);
    grid.setDragEnabled(true);

    auto sendMouse = [&grid](QEvent::Type type, const QPoint &pos,
                             Qt::MouseButton button, Qt::MouseButtons buttons) {
        QMouseEvent event(type, pos, grid.mapToGlobal(pos), button, buttons, Qt::NoModifier);
        QCoreApplication::sendEvent(&grid, &event);
    };

    const QPoint pressPos = second->geometry().topLeft() + QPoint(24, 24);
    const QPoint movePos = pressPos + QPoint(-16, 0);
    sendMouse(QEvent::MouseButtonPress, pressPos, Qt::LeftButton, Qt::LeftButton);
    sendMouse(QEvent::MouseMove, movePos, Qt::NoButton, Qt::LeftButton);
    QCoreApplication::processEvents();

    auto *ghost = grid.findChild<QLabel *>(QStringLiteral("GridDragGhost"));
    QVERIFY(ghost);

    const QPoint centerOffset((ghost->width() - second->width()) / 2,
                              (ghost->height() - second->height()) / 2);
    QCOMPARE(ghost->pos(), movePos - QPoint(24, 24) - centerOffset);

    QTest::keyClick(&grid, Qt::Key_Escape);
    QCoreApplication::processEvents();
}

void TestDragGridWidget::mouseDrag_releaseAtOriginalIndexRestoresGeometry()
{
    DragGridWidget grid;
    auto *first = createItem(QStringLiteral("first"));
    auto *second = createItem(QStringLiteral("second"));
    auto *third = createItem(QStringLiteral("third"));
    prepareGrid(&grid, first, second, third);
    grid.setAnimationDuration(200);
    grid.setDragEnabled(true);

    auto sendMouse = [&grid](QEvent::Type type, const QPoint &pos,
                             Qt::MouseButton button, Qt::MouseButtons buttons) {
        QMouseEvent event(type, pos, grid.mapToGlobal(pos), button, buttons, Qt::NoModifier);
        QCoreApplication::sendEvent(&grid, &event);
    };

    const QRect originalGeometry = second->geometry();
    const QPoint pressPos = second->geometry().topLeft() + QPoint(24, 24);
    const QPoint releasePos = pressPos + QPoint(-16, 0);
    sendMouse(QEvent::MouseButtonPress, pressPos, Qt::LeftButton, Qt::LeftButton);
    sendMouse(QEvent::MouseMove, releasePos, Qt::NoButton, Qt::LeftButton);
    QCoreApplication::processEvents();
    sendMouse(QEvent::MouseButtonRelease, releasePos, Qt::LeftButton, Qt::NoButton);
    QCoreApplication::processEvents();

    QCOMPARE(objectNames(grid.widgets()), QStringList({QStringLiteral("first"),
                                                       QStringLiteral("second"),
                                                       QStringLiteral("third")}));
    QCOMPARE(second->geometry(), originalGeometry);
    QVERIFY(second->isVisible());
    QVERIFY(!grid.findChild<QLabel *>(QStringLiteral("GridDragGhost")));
}

void TestDragGridWidget::mouseDrag_reordersWithStretchedLastRow()
{
    DragGridWidget grid;
    grid.setColumnCount(4);
    grid.setMinimumCellSize(QSize(100, 100));
    grid.setAnimationDuration(0);
    grid.setFillIncompleteRowEnabled(true);
    grid.resize(400, 240);
    grid.show();
    QVERIFY(QTest::qWaitForWindowExposed(&grid));

    auto *first = createItem(QStringLiteral("first"));
    auto *second = createItem(QStringLiteral("second"));
    auto *third = createItem(QStringLiteral("third"));
    auto *fourth = createItem(QStringLiteral("fourth"));
    auto *fifth = createItem(QStringLiteral("fifth"));
    auto *sixth = createItem(QStringLiteral("sixth"));
    grid.addWidget(first);
    grid.addWidget(second);
    grid.addWidget(third);
    grid.addWidget(fourth);
    grid.addWidget(fifth);
    grid.addWidget(sixth);
    grid.layout()->activate();
    QCoreApplication::processEvents();

    grid.setDragEnabled(true);
    QSignalSpy orderSpy(&grid, &DragGridWidget::orderChanged);

    dragFromTo(&grid, first->geometry().center(), sixth->geometry().center());

    QCOMPARE(objectNames(grid.widgets()), QStringList({QStringLiteral("second"),
                                                       QStringLiteral("third"),
                                                       QStringLiteral("fourth"),
                                                       QStringLiteral("fifth"),
                                                       QStringLiteral("sixth"),
                                                       QStringLiteral("first")}));
    QCOMPARE(orderSpy.count(), 1);
    QVERIFY(first->isVisible());
}

QTEST_MAIN(TestDragGridWidget)

#include "TestDragGridWidget.moc"
