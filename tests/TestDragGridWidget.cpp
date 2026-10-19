#include <QTest>
#include <QLabel>
#include <QLayout>
#include <QMetaProperty>
#include <QMouseEvent>
#include <QPropertyAnimation>
#include <QScrollBar>
#include <QSignalSpy>
#include <QScrollArea>
#include <QWidget>

#include <DragGridWidget.h>
#include <YJDragGrid/DragGridWidget.h>

using YJDragGrid::DragGridWidget;

class TestDragGridWidget : public QObject
{
    Q_OBJECT

private slots:
    void metaProperties_areExposed();
    void compatibilityHeader_exposesLegacyTypeName();
    void dragEnabled_togglesState();
    void fillIncompleteRowEnabled_roundTripAndCompat();
    void placeholderProperties_roundTrip();
    void dragHandle_canBeSet();
    void emptyText_defaultAndCustom();
    void emptyStateVisible_togglesVisibility();
    void mouseDrag_reordersWidgets();
    void keyboardDrag_emitsDetailedSignals();
    void keyboardDrag_reordersWidgets();
    void escapeDuringDrag_restoresOriginalOrder();
    void deleteWidget_duringDrag_removesDraggedWidget();
    void setDragEnabled_falseDuringDrag_cancelsDrag();
    void zeroAnimationDuration_reordersDirectly();
    void mouseDrag_smallMoveKeepsGhostAnchored();
    void mouseDrag_releaseAtOriginalIndexRestoresGeometry();
    void mouseDrag_reordersWithStretchedLastRow();
    void keyboardDrag_doesNotAutoScroll();
    void keyboardDrag_startDoesNotScrollViewport();
    void externalDelete_duringDrag_cleansUp();
    void externalDelete_whenIdle_showsEmptyState();
    void removeWidget_betweenPressAndMove_doesNotEnterDrag();
    void destroyDuringDrag_doesNotCrash();
    void placeholderOpacity_updatesPulseKeyframes();
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

void TestDragGridWidget::metaProperties_areExposed()
{
    DragGridWidget grid;
    const QMetaObject *meta = grid.metaObject();

    QVERIFY(meta->indexOfProperty("columnCount") >= 0);
    QVERIFY(meta->indexOfProperty("dragEnabled") >= 0);
    QVERIFY(meta->indexOfProperty("animationDuration") >= 0);
    QVERIFY(meta->indexOfProperty("emptyText") >= 0);

    QVERIFY(grid.setProperty("columnCount", 2));
    QCOMPARE(grid.columnCount(), 2);
    QVERIFY(grid.setProperty("dragEnabled", true));
    QVERIFY(grid.dragEnabled());
}

void TestDragGridWidget::compatibilityHeader_exposesLegacyTypeName()
{
    ::DragGridWidget grid;
    grid.setColumnCount(2);
    QCOMPARE(grid.columnCount(), 2);
}

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

void TestDragGridWidget::keyboardDrag_emitsDetailedSignals()
{
    qRegisterMetaType<QList<QWidget *>>();

    DragGridWidget grid;
    auto *first = createItem(QStringLiteral("first"));
    auto *second = createItem(QStringLiteral("second"));
    prepareGrid(&grid, first, second);
    grid.setDragEnabled(true);
    grid.activateWindow();
    first->setFocus();
    QCoreApplication::processEvents();

    QSignalSpy movedSpy(&grid, &DragGridWidget::itemMoved);
    QSignalSpy detailedOrderSpy(&grid, &DragGridWidget::orderUpdated);
    QSignalSpy legacyOrderSpy(&grid, &DragGridWidget::orderChanged);

    QTest::keyClick(&grid, Qt::Key_Space);
    QTest::keyClick(&grid, Qt::Key_Right);
    QTest::keyClick(&grid, Qt::Key_Return);
    QCoreApplication::processEvents();

    QCOMPARE(movedSpy.count(), 1);
    const QList<QVariant> movedArguments = movedSpy.takeFirst();
    QCOMPARE(movedArguments.at(0).toInt(), 0);
    QCOMPARE(movedArguments.at(1).toInt(), 1);

    QCOMPARE(detailedOrderSpy.count(), 1);
    const QList<QWidget *> orderedWidgets = qvariant_cast<QList<QWidget *>>(detailedOrderSpy.takeFirst().at(0));
    QCOMPARE(objectNames(orderedWidgets), QStringList({QStringLiteral("second"), QStringLiteral("first")}));
    QCOMPARE(legacyOrderSpy.count(), 1);
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

void TestDragGridWidget::keyboardDrag_doesNotAutoScroll()
{
    // 特征测试：键盘拖拽不得依据过期鼠标坐标触发边缘自动滚动。
    // 修复前 m_lastMousePos 为零值，网格左上角贴近视口顶边时会持续向上滚动。
    QScrollArea scrollArea;
    auto *grid = new DragGridWidget(&scrollArea);
    scrollArea.setWidget(grid);
    // 布局最小高度（4 行 × 100）大于视口，保证出现垂直滚动条范围。
    scrollArea.setWidgetResizable(true);
    scrollArea.resize(300, 280);
    scrollArea.show();
    QVERIFY(QTest::qWaitForWindowExposed(&scrollArea));

    grid->setColumnCount(3);
    grid->setMinimumCellSize(QSize(100, 100));
    grid->setAnimationDuration(0);
    grid->setDragEnabled(true);

    for (int index = 0; index < 12; ++index) {
        auto *item = createItem(QStringLiteral("item%1").arg(index));
        grid->addWidget(item);
    }
    grid->layout()->activate();
    QCoreApplication::processEvents();

    auto *vBar = scrollArea.verticalScrollBar();

    // 先处理焦点变化（QScrollArea 会自动滚到焦点控件），再设定滚动基准值。
    auto *target = grid->widgets().first();
    grid->activateWindow();
    target->setFocus();
    QCoreApplication::processEvents();

    QVERIFY(vBar->maximum() > 0);
    vBar->setValue(vBar->maximum() / 2);

    QTest::keyClick(grid, Qt::Key_Space);
    QCoreApplication::processEvents();
    // 拾取会隐藏焦点控件，Qt 焦点转移可能引发一次性滚动；
    // 以拾取完成后的值为基准，只隔离定时器驱动的边缘自动滚动。
    const int valueAfterStart = vBar->value();

    QTest::qWait(80); // 足够自动滚动定时器（16ms）运行若干拍
    QCoreApplication::processEvents();

    QCOMPARE(vBar->value(), valueAfterStart);

    QTest::keyClick(grid, Qt::Key_Return);
    QCoreApplication::processEvents();
}

void TestDragGridWidget::externalDelete_duringDrag_cleansUp()
{
    // 特征测试：外部直接 delete 拖拽中的控件时，网格须自动摘除并清理拖拽状态。
    DragGridWidget grid;
    auto *first = createItem(QStringLiteral("first"));
    auto *second = createItem(QStringLiteral("second"));
    prepareGrid(&grid, first, second);
    grid.setDragEnabled(true);
    QSignalSpy orderSpy(&grid, &DragGridWidget::orderChanged);

    auto sendMouse = [&grid](QEvent::Type type, const QPoint &pos,
                             Qt::MouseButton button, Qt::MouseButtons buttons) {
        QMouseEvent event(type, pos, grid.mapToGlobal(pos), button, buttons, Qt::NoModifier);
        QCoreApplication::sendEvent(&grid, &event);
    };

    sendMouse(QEvent::MouseButtonPress, first->geometry().center(), Qt::LeftButton, Qt::LeftButton);
    sendMouse(QEvent::MouseMove, second->geometry().center() + QPoint(60, 0), Qt::NoButton, Qt::LeftButton);
    QCoreApplication::processEvents();
    QVERIFY(grid.findChild<QLabel *>(QStringLiteral("GridDragGhost")));

    delete first; // 外部直接销毁，不经过 deleteWidget()/removeWidget()
    // cleanupDragUi() 通过 deleteLater() 清理镜像，需 flush 后才能断言。
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();

    QCOMPARE(objectNames(grid.widgets()), QStringList({QStringLiteral("second")}));
    QCOMPARE(orderSpy.count(), 0);
    QVERIFY(!grid.findChild<QLabel *>(QStringLiteral("GridDragGhost")));

    auto *placeholder = grid.findChild<QWidget *>(QStringLiteral("GridPlaceholder"));
    QVERIFY(placeholder);
    QVERIFY(!placeholder->isVisible());

    // 清理后网格仍可继续拖拽，不得崩溃或残留悬垂状态。
    grid.activateWindow();
    second->setFocus();
    QCoreApplication::processEvents();
    QTest::keyClick(&grid, Qt::Key_Space);
    QTest::keyClick(&grid, Qt::Key_Return);
    QCoreApplication::processEvents();

    QCOMPARE(objectNames(grid.widgets()), QStringList({QStringLiteral("second")}));
    QVERIFY(second->isVisible());
}

void TestDragGridWidget::placeholderOpacity_updatesPulseKeyframes()
{
    // 特征测试：修改 placeholderOpacity 必须同步脉冲动画关键帧。
    DragGridWidget grid;
    auto *pulse = grid.findChild<QPropertyAnimation *>();
    QVERIFY(pulse);
    QCOMPARE(pulse->propertyName(), QByteArrayLiteral("opacity"));

    grid.setPlaceholderOpacity(0.5);
    QCOMPARE(pulse->keyValueAt(0.0).toDouble(), 0.3); // 0.5 * 0.6
    QCOMPARE(pulse->keyValueAt(0.5).toDouble(), 0.7); // min(1.0, 0.5 * 1.4)
    QCOMPARE(pulse->keyValueAt(1.0).toDouble(), 0.3);
}

void TestDragGridWidget::keyboardDrag_startDoesNotScrollViewport()
{
    // 特征测试：启动拖拽时隐藏获得焦点的卡片，不得因焦点转移引发视口跳转。
    // Qt 6.9 起 QScrollArea 会滚动到焦点转移后的新控件；修复前滚动条从中间跳到 0。
    QScrollArea scrollArea;
    auto *grid = new DragGridWidget(&scrollArea);
    scrollArea.setWidget(grid);
    scrollArea.setWidgetResizable(false);
    scrollArea.resize(300, 300);
    scrollArea.show();
    QVERIFY(QTest::qWaitForWindowExposed(&scrollArea));

    grid->setColumnCount(3);
    grid->setMinimumCellSize(QSize(100, 100));
    grid->setAnimationDuration(0);
    grid->setDragEnabled(true);
    for (int index = 0; index < 12; ++index) {
        auto *item = createItem(QStringLiteral("item%1").arg(index));
        grid->addWidget(item);
    }
    grid->layout()->activate();
    // widgetResizable(false) 下滚动区域不主动撑高内容控件，显式给出高于视口的高度。
    grid->resize(300, 600);
    QCoreApplication::processEvents();

    auto *vBar = scrollArea.verticalScrollBar();
    QVERIFY(vBar->maximum() > 0);
    vBar->setValue(vBar->maximum() / 2);
    const int valueBefore = vBar->value();

    auto *target = grid->widgets().first();
    grid->activateWindow();
    target->setFocus();
    QCoreApplication::processEvents();

    QTest::keyClick(grid, Qt::Key_Space);
    QCoreApplication::processEvents();

    QVERIFY(grid->findChild<QLabel *>(QStringLiteral("GridDragGhost")));
    QCOMPARE(vBar->value(), valueBefore);

    QTest::keyClick(grid, Qt::Key_Return);
    QCoreApplication::processEvents();
}

void TestDragGridWidget::externalDelete_whenIdle_showsEmptyState()
{
    // 特征测试：非拖拽中的外部销毁必须立即刷新空状态，不能等到下一次交互。
    DragGridWidget grid;
    auto *first = createItem(QStringLiteral("first"));
    auto *second = createItem(QStringLiteral("second"));
    prepareGrid(&grid, first, second);

    auto *label = grid.findChild<QLabel *>(QStringLiteral("GridEmptyState"));
    QVERIFY(label);
    QVERIFY(!label->isVisible());

    delete first; // 外部直接销毁，不经过 removeWidget()/deleteWidget()
    QCOMPARE(grid.count(), 1);
    QVERIFY(!label->isVisible());

    delete second;
    QCOMPARE(grid.count(), 0);
    QVERIFY(label->isVisible());
}

void TestDragGridWidget::removeWidget_betweenPressAndMove_doesNotEnterDrag()
{
    // 特征测试：按下后、越过阈值前控件被程序移除（不销毁），不得进入拖拽状态；
    // 修复前状态卡在 Dragging：Esc 无效，且后续移动会让无主的占位符可见。
    DragGridWidget grid;
    auto *first = createItem(QStringLiteral("first"));
    auto *second = createItem(QStringLiteral("second"));
    auto *third = createItem(QStringLiteral("third"));
    prepareGrid(&grid, first, second, third);
    grid.setDragEnabled(true);
    QSignalSpy orderSpy(&grid, &DragGridWidget::orderChanged);

    auto sendMouse = [&grid](QEvent::Type type, const QPoint &pos,
                             Qt::MouseButton button, Qt::MouseButtons buttons) {
        QMouseEvent event(type, pos, grid.mapToGlobal(pos), button, buttons, Qt::NoModifier);
        QCoreApplication::sendEvent(&grid, &event);
    };

    sendMouse(QEvent::MouseButtonPress, first->geometry().center(), Qt::LeftButton, Qt::LeftButton);
    grid.removeWidget(first); // 程序移除按住的控件，所有权回到调用者
    QCoreApplication::processEvents();

    sendMouse(QEvent::MouseMove, second->geometry().center() + QPoint(80, 0),
              Qt::NoButton, Qt::LeftButton);
    QCoreApplication::processEvents();

    QVERIFY(!grid.findChild<QLabel *>(QStringLiteral("GridDragGhost")));

    QTest::keyClick(&grid, Qt::Key_Escape);
    QCoreApplication::processEvents();
    auto *placeholder = grid.findChild<QWidget *>(QStringLiteral("GridPlaceholder"));
    QVERIFY(placeholder);
    QVERIFY(!placeholder->isVisible());

    // 状态机必须仍可正常拖拽其余控件
    dragFromTo(&grid, second->geometry().center(), third->geometry().center() + QPoint(60, 0));
    QCOMPARE(objectNames(grid.widgets()), QStringList({QStringLiteral("third"),
                                                       QStringLiteral("second")}));
    QCOMPARE(orderSpy.count(), 1);

    delete first;
}

void TestDragGridWidget::destroyDuringDrag_doesNotCrash()
{
    // 特征测试：拖拽未结束时直接销毁容器，析构路径不得解引用已销毁的内部对象。
    // 修复前 destroyed 回调在析构期触碰 m_gridLayout（堆状态决定是否段错误）。
    QScrollArea scrollArea;
    auto *grid = new DragGridWidget(&scrollArea);
    scrollArea.setWidget(grid);
    scrollArea.setWidgetResizable(false);
    scrollArea.resize(300, 300);
    scrollArea.show();
    QVERIFY(QTest::qWaitForWindowExposed(&scrollArea));

    grid->setColumnCount(3);
    grid->setMinimumCellSize(QSize(100, 100));
    grid->setAnimationDuration(0);
    grid->setDragEnabled(true);
    for (int index = 0; index < 6; ++index) {
        auto *item = createItem(QStringLiteral("item%1").arg(index));
        grid->addWidget(item);
    }
    grid->layout()->activate();
    QCoreApplication::processEvents();

    auto *target = grid->widgets().first();
    grid->activateWindow();
    target->setFocus();
    QCoreApplication::processEvents();

    QTest::keyClick(grid, Qt::Key_Space);
    QCoreApplication::processEvents();
    QVERIFY(grid->findChild<QLabel *>(QStringLiteral("GridDragGhost")));

    // 不调用 cancelDrag()/finishDrag()，直接离开作用域销毁容器。
}

QTEST_MAIN(TestDragGridWidget)

#include "TestDragGridWidget.moc"
