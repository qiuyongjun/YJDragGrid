#include <QTest>
#include <QAbstractAnimation>
#include <QFrame>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMetaProperty>
#include <QMouseEvent>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QScrollBar>
#include <QSignalSpy>
#include <QScrollArea>
#include <QSpacerItem>
#include <QVBoxLayout>
#include <QWidget>

#include <DragGridWidget.h>
#include <YJDragGrid/DragGridLayout.h>
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
    void mouseDrag_fastReleaseLeftHalfKeepsOrder();
    void mouseDrag_steadyStateDropMatchesCursor();
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
    void zeroPlaceholderPulseDuration_skipsPulseAnimation();
    void mouseDrag_ghostSizeHonorsScale();
    void mouseDrag_reorderUsesSingleGeometryAnimation();
    void spaceOnCardButton_startsDragInsteadOfClick();
    void placeholder_isVisibleWithoutStyleSheet();
    void clear_withSpacerItemCompletes();
    void layout_isQObjectCastable();
    void takeWidget_skipsNonWidgetItems();
    void externalDelete_otherCardDuringDrag_keepsPlaceholderInRange();
    void spaceInCardLineEdit_typesSpaceInsteadOfDragging();
    void nestedGrid_keepsKeyHandlingOnInnerGrid();
    void mousePressDuringKeyboardDrag_doesNotHideCard();
    void geometryAnimations_areReleasedAfterReorder();
    void spaceOnLateAddedCardButton_startsDrag();
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

void dragThroughPoints(DragGridWidget *grid, const QPoint &from,
                       const QList<QPoint> &waypoints, const QPoint &to)
{
    // 与 dragFromTo 相同的事件序列，但支持任意多个中间 move 事件：
    // 稳态落点行为（占位符已离位、被拖项原始索引 ≠ 当前占位索引）
    // 需要至少一次使占位符移动的 move 才能覆盖。
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
    for (const QPoint &waypoint : waypoints) {
        sendMouse(QEvent::MouseMove, waypoint, Qt::NoButton, Qt::LeftButton);
        QCoreApplication::processEvents();
    }
    sendMouse(QEvent::MouseMove, to, Qt::NoButton, Qt::LeftButton);
    QCoreApplication::processEvents();
    sendMouse(QEvent::MouseButtonRelease, to, Qt::LeftButton, Qt::NoButton);
    QCoreApplication::processEvents();
}

void dragFromTo(DragGridWidget *grid, const QPoint &from, const QPoint &to)
{
    dragThroughPoints(grid, from, {}, to);
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

void TestDragGridWidget::mouseDrag_fastReleaseLeftHalfKeepsOrder()
{
    // 特征测试：向右快速拖拽（一两个 move 事件即释放）时，落点须按静态
    // 单元格计算并对被保留的占位槽做补偿。悬停某单元格左半区表示
    // "插在该项目之前"——目标顺序不变时不得重排。
    // 复现修复前故障：拖 first 到 second 单元格左半区释放，结果过冲成
    // [second, first, third]，且向左拖正确、向右拖偏后一位，方向不对称。
    DragGridWidget grid;
    auto *first = createItem(QStringLiteral("first"));
    auto *second = createItem(QStringLiteral("second"));
    auto *third = createItem(QStringLiteral("third"));
    prepareGrid(&grid, first, second, third);
    grid.setDragEnabled(true);
    QSignalSpy orderSpy(&grid, &DragGridWidget::orderChanged);

    const QRect secondCell = second->geometry();
    const QPoint target(secondCell.left() + secondCell.width() / 4, secondCell.center().y());
    dragFromTo(&grid, first->geometry().center(), target);

    QCOMPARE(objectNames(grid.widgets()), QStringList({QStringLiteral("first"),
                                                       QStringLiteral("second"),
                                                       QStringLiteral("third")}));
    QCOMPARE(orderSpy.count(), 0);
    QVERIFY(first->isVisible());
}

void TestDragGridWidget::mouseDrag_steadyStateDropMatchesCursor()
{
    // 特征测试：占位符移动后（被拖项原始索引 ≠ 当前占位索引），落点补偿
    // 必须以当前占位索引为基准，落点 = 光标左侧已渲染项数。
    // 复现修复前故障：按被拖项原始索引补偿——拖 first 越过 second 进入
    // 稳态后，悬停占位格（second 原槽位）左半区会把占位符从光标下向左
    // 挤走一格，释放结果仍是 [first, second, third]。
    DragGridWidget grid;
    auto *first = createItem(QStringLiteral("first"));
    auto *second = createItem(QStringLiteral("second"));
    auto *third = createItem(QStringLiteral("third"));
    prepareGrid(&grid, first, second, third);
    grid.setDragEnabled(true);
    QSignalSpy orderSpy(&grid, &DragGridWidget::orderChanged);

    const QRect secondCell = second->geometry();
    const QRect thirdCell = third->geometry();

    // first → third 左半区（占位符进入 second 与 third 之间，进入稳态），
    // 再悬停占位格左半区释放：占位符不得逃逸，落点钉在 second 之前。
    const QList<QPoint> waypoints{
        QPoint(thirdCell.left() + thirdCell.width() / 4, thirdCell.center().y())};
    dragThroughPoints(&grid, first->geometry().center(), waypoints,
                      QPoint(secondCell.left() + secondCell.width() / 4,
                             secondCell.center().y()));

    QCOMPARE(objectNames(grid.widgets()), QStringList({QStringLiteral("second"),
                                                       QStringLiteral("first"),
                                                       QStringLiteral("third")}));
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

    // 落点取第六项单元格右半区（center + width/4），明确表达"插到 sixth 之后"。
    // 修复落点补偿前用 center 也能通过：奇数宽度的拉伸单元格中点恰好落在
    // 左半区（(width/2)*2 < width），旧实现靠末位 clamp 碰巧判成追加。
    const QRect sixthCell = sixth->geometry();
    dragFromTo(&grid, first->geometry().center(),
               sixthCell.center() + QPoint(sixthCell.width() / 4, 0));

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

void TestDragGridWidget::zeroPlaceholderPulseDuration_skipsPulseAnimation()
{
    // 特征测试：duration=0 配合 loopCount=-1 的 QPropertyAnimation 会立即
    // 完成并无限重启，占满事件循环 tick。0 时长表示"不要脉冲"：
    // 拖拽启动时跳过，拖拽中改 0 时立即停止。
    DragGridWidget grid;
    auto *first = createItem(QStringLiteral("first"));
    auto *second = createItem(QStringLiteral("second"));
    prepareGrid(&grid, first, second);
    grid.setDragEnabled(true);
    grid.setPlaceholderPulseDuration(0);

    auto pulseRunning = [&grid]() {
        for (QPropertyAnimation *animation : grid.findChildren<QPropertyAnimation *>()) {
            if (animation->propertyName() == QByteArrayLiteral("opacity")
                && animation->state() == QAbstractAnimation::Running) {
                return true;
            }
        }
        return false;
    };

    auto startDrag = [&grid, first]() {
        const QPoint from = first->geometry().center();
        QMouseEvent press(QEvent::MouseButtonPress, from, grid.mapToGlobal(from),
                          Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(&grid, &press);
        QMouseEvent move(QEvent::MouseMove, from + QPoint(30, 0),
                         grid.mapToGlobal(from + QPoint(30, 0)),
                         Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(&grid, &move);
        QCoreApplication::processEvents();
    };

    startDrag();
    QVERIFY(!pulseRunning());

    // 反向验证：改回非 0 时长后脉冲确实会启动，避免上面断言空过。
    grid.setDragEnabled(false);
    grid.setPlaceholderPulseDuration(150);
    grid.setDragEnabled(true);
    startDrag();
    QVERIFY(pulseRunning());

    // 拖拽进行中把时长改成 0 也必须立即停止。
    grid.setPlaceholderPulseDuration(0);
    QVERIFY(!pulseRunning());

    grid.setDragEnabled(false);
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

void TestDragGridWidget::mouseDrag_ghostSizeHonorsScale()
{
    // 特征测试：镜像的逻辑尺寸必须等于源控件尺寸 * ghostScale，与设备像素比无关。
    // 修复前 scaled() 把逻辑尺寸当作设备像素，DPR=2 时镜像只剩一半、锚点同时偏移。
    DragGridWidget grid;
    auto *first = createItem(QStringLiteral("first"));
    auto *second = createItem(QStringLiteral("second"));
    prepareGrid(&grid, first, second);
    grid.setGhostScale(1.5);
    grid.setDragEnabled(true);

    auto sendMouse = [&grid](QEvent::Type type, const QPoint &pos,
                             Qt::MouseButton button, Qt::MouseButtons buttons) {
        QMouseEvent event(type, pos, grid.mapToGlobal(pos), button, buttons, Qt::NoModifier);
        QCoreApplication::sendEvent(&grid, &event);
    };

    const QPoint pressPos = first->geometry().center();
    sendMouse(QEvent::MouseButtonPress, pressPos, Qt::LeftButton, Qt::LeftButton);
    sendMouse(QEvent::MouseMove, pressPos + QPoint(30, 0), Qt::NoButton, Qt::LeftButton);
    QCoreApplication::processEvents();

    auto *ghost = grid.findChild<QLabel *>(QStringLiteral("GridDragGhost"));
    QVERIFY(ghost);

    const int expectedWidth = qRound(first->width() * 1.5);
    const int expectedHeight = qRound(first->height() * 1.5);
    QVERIFY2(qAbs(ghost->width() - expectedWidth) <= 1,
             qPrintable(QStringLiteral("ghost width %1, expected %2")
                            .arg(ghost->width()).arg(expectedWidth)));
    QVERIFY(qAbs(ghost->height() - expectedHeight) <= 1);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    const QPixmap ghostPixmap = ghost->pixmap(Qt::ReturnByValue);
#else
    const QPixmap ghostPixmap = *ghost->pixmap();
#endif
    QCOMPARE(ghostPixmap.devicePixelRatio(), ghost->devicePixelRatioF());

    QTest::keyClick(&grid, Qt::Key_Escape);
    QCoreApplication::processEvents();
}

void TestDragGridWidget::mouseDrag_reorderUsesSingleGeometryAnimation()
{
    // 特征测试：落位过程中只允许一条动画驱动被拖控件的 geometry。
    // 修复前 finishDrag 自建 settleAnimation，随后的 activate() 又建布局动画，
    // 两条动画写同一属性，落位过程会互相覆盖。
    DragGridWidget grid;
    auto *first = createItem(QStringLiteral("first"));
    auto *second = createItem(QStringLiteral("second"));
    auto *third = createItem(QStringLiteral("third"));
    prepareGrid(&grid, first, second, third);
    grid.setAnimationDuration(200);
    grid.setDragEnabled(true);

    dragFromTo(&grid, first->geometry().center(), third->geometry().center() + QPoint(60, 0));

    int runningGeometryAnimations = 0;
    const QList<QPropertyAnimation *> animations = grid.findChildren<QPropertyAnimation *>();
    for (QPropertyAnimation *animation : animations) {
        if (animation->targetObject() == first
            && animation->propertyName() == QByteArrayLiteral("geometry")
            && animation->state() == QAbstractAnimation::Running) {
            ++runningGeometryAnimations;
        }
    }
    QCOMPARE(runningGeometryAnimations, 1);

    // 等落位动画结束，避免遗留动画影响后续用例与析构顺序。
    QTest::qWait(300);
}

void TestDragGridWidget::spaceOnCardButton_startsDragInsteadOfClick()
{
    // 特征测试：焦点落在卡片内按钮上时，Space 属于网格的拾取键。
    // 修复前 QPushButton 会吞掉 Space，示例中焦点停在关闭按钮上会直接删除卡片。
    DragGridWidget grid;
    auto *card = new QWidget;
    card->setObjectName(QStringLiteral("card"));
    card->setMinimumSize(100, 100);
    card->resize(100, 100);
    auto *cardLayout = new QVBoxLayout(card);
    auto *button = new QPushButton(QStringLiteral("close"), card);
    cardLayout->addWidget(button);

    auto *second = createItem(QStringLiteral("second"));
    prepareGrid(&grid, card, second);
    grid.setDragEnabled(true);
    grid.activateWindow();

    button->setFocus();
    QCoreApplication::processEvents();
    QCOMPARE(QApplication::focusWidget(), static_cast<QWidget *>(button));

    QSignalSpy clickedSpy(button, &QPushButton::clicked);
    QTest::keyClick(button, Qt::Key_Space);
    QCoreApplication::processEvents();

    QCOMPARE(clickedSpy.count(), 0);
    QVERIFY(grid.findChild<QLabel *>(QStringLiteral("GridDragGhost")));

    QTest::keyClick(button, Qt::Key_Return);
    QCoreApplication::processEvents();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(!grid.findChild<QLabel *>(QStringLiteral("GridDragGhost")));
    QVERIFY(card->isVisible());
}

void TestDragGridWidget::placeholder_isVisibleWithoutStyleSheet()
{
    // 特征测试：未提供 QSS 时占位符也必须可见，否则使用方看不到拖拽落点。
    DragGridWidget grid;
    auto *first = createItem(QStringLiteral("first"));
    auto *second = createItem(QStringLiteral("second"));
    prepareGrid(&grid, first, second);
    grid.setDragEnabled(true);

    auto sendMouse = [&grid](QEvent::Type type, const QPoint &pos,
                             Qt::MouseButton button, Qt::MouseButtons buttons) {
        QMouseEvent event(type, pos, grid.mapToGlobal(pos), button, buttons, Qt::NoModifier);
        QCoreApplication::sendEvent(&grid, &event);
    };

    const QPoint pressPos = first->geometry().center();
    sendMouse(QEvent::MouseButtonPress, pressPos, Qt::LeftButton, Qt::LeftButton);
    sendMouse(QEvent::MouseMove, pressPos + QPoint(30, 0), Qt::NoButton, Qt::LeftButton);
    QCoreApplication::processEvents();

    auto *placeholder = grid.findChild<QFrame *>(QStringLiteral("GridPlaceholder"));
    QVERIFY(placeholder);
    QVERIFY(placeholder->isVisible());

    const QImage image = placeholder->grab().toImage();
    QVERIFY(!image.isNull());
    const QColor centerColor = image.pixelColor(image.width() / 2, image.height() / 2);
    QVERIFY2(centerColor.alpha() > 0,
             qPrintable(QStringLiteral("placeholder center alpha=%1").arg(centerColor.alpha())));

    QTest::keyClick(&grid, Qt::Key_Escape);
    QCoreApplication::processEvents();
}

void TestDragGridWidget::clear_withSpacerItemCompletes()
{
    // 特征测试：布局中存在非 widget 项时 clear() 也必须结束。
    // 修复前 count() 永远大于 0、takeWidget(0) 恒为 nullptr，循环会卡死。
    DragGridWidget grid;
    auto *first = createItem(QStringLiteral("first"));
    auto *second = createItem(QStringLiteral("second"));
    grid.addWidget(first);
    grid.addWidget(second);
    grid.layout()->addItem(new QSpacerItem(20, 20));

    QCOMPARE(grid.count(), 3);
    grid.clear();
    QCOMPARE(grid.count(), 0);
    QVERIFY(grid.widgets().isEmpty());
}

void TestDragGridWidget::layout_isQObjectCastable()
{
    // 特征测试：布局类必须有 Q_OBJECT，qobject_cast 才能识别派生类型。
    DragGridWidget grid;
    QVERIFY(qobject_cast<YJDragGrid::DragGridLayout *>(grid.layout()) != nullptr);
}

void TestDragGridWidget::takeWidget_skipsNonWidgetItems()
{
    // 特征测试：布局中存在非 widget 项时，takeWidget(index) 的索引按 widgets() 计算，
    // 不能直接当作布局索引使用（修复前会摘错对象或直接返回空）。
    DragGridWidget grid;
    auto *first = createItem(QStringLiteral("first"));
    auto *second = createItem(QStringLiteral("second"));
    grid.addWidget(first);
    grid.addWidget(second);
    grid.layout()->addItem(new QSpacerItem(10, 10));

    auto *layout = qobject_cast<YJDragGrid::DragGridLayout *>(grid.layout());
    QVERIFY(layout);
    QVERIFY(layout->moveItem(2, 0)); // spacer 前置，制造索引错位

    QWidget *taken = grid.takeWidget(0);
    QCOMPARE(taken, static_cast<QWidget *>(first));

    const QList<QWidget *> remaining = grid.widgets();
    QCOMPARE(remaining.size(), 1);
    QCOMPARE(remaining.first(), static_cast<QWidget *>(second));
    QCOMPARE(grid.count(), 2); // second + spacer

    taken->deleteLater();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}

void TestDragGridWidget::externalDelete_otherCardDuringDrag_keepsPlaceholderInRange()
{
    // 特征测试：拖拽期间删除其他卡片后，占位索引必须收回有效槽位。
    // 修复前索引保持旧值，占位槽不再被预留，卡片会挤进同一格。
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

    sendMouse(QEvent::MouseButtonPress, first->geometry().center(), Qt::LeftButton, Qt::LeftButton);
    sendMouse(QEvent::MouseMove, third->geometry().center() + QPoint(60, 0),
              Qt::NoButton, Qt::LeftButton);
    QCoreApplication::processEvents();

    auto *layout = qobject_cast<YJDragGrid::DragGridLayout *>(grid.layout());
    QVERIFY(layout);
    QCOMPARE(layout->placeholderIndex(), 2);

    delete third; // 删除非拖拽目标
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();

    QVERIFY(layout->placeholderIndex() <= grid.widgets().size() - 1);

    // 特征断言：删除后占位符几何必须立即跟随布局重排。
    // 修复前占位符仍画在删除前的旧矩形，直到下一次鼠标移动才纠正。
    auto *placeholder = grid.findChild<QWidget *>(QStringLiteral("GridPlaceholder"));
    QVERIFY(placeholder);
    QVERIFY(placeholder->isVisible());
    QCOMPARE(placeholder->geometry(),
             layout->placeholderRectAt(layout->placeholderIndex()));

    sendMouse(QEvent::MouseMove, second->geometry().center(), Qt::NoButton, Qt::LeftButton);
    QCoreApplication::processEvents();
    sendMouse(QEvent::MouseButtonRelease, second->geometry().center(),
              Qt::LeftButton, Qt::NoButton);
    QCoreApplication::processEvents();

    QCOMPARE(grid.widgets().size(), 2);
    QVERIFY(first->isVisible());
    QVERIFY(!first->geometry().intersects(second->geometry()));
}

void TestDragGridWidget::spaceInCardLineEdit_typesSpaceInsteadOfDragging()
{
    // 特征测试：卡片内的文本输入控件保留 Space 语义。
    // 按键过滤器若无条件抢占 Space，输入框将无法输入空格。
    DragGridWidget grid;
    auto *card = new QWidget;
    card->setObjectName(QStringLiteral("card"));
    card->setMinimumSize(100, 100);
    card->resize(100, 100);
    auto *cardLayout = new QVBoxLayout(card);
    auto *edit = new QLineEdit(card);
    cardLayout->addWidget(edit);

    auto *second = createItem(QStringLiteral("second"));
    prepareGrid(&grid, card, second);
    grid.setDragEnabled(true);
    grid.activateWindow();

    edit->setFocus();
    QCoreApplication::processEvents();
    QCOMPARE(QApplication::focusWidget(), static_cast<QWidget *>(edit));

    QTest::keyClicks(edit, QStringLiteral("a b"));
    QCoreApplication::processEvents();

    QCOMPARE(edit->text(), QStringLiteral("a b"));
    QVERIFY(!grid.findChild<QLabel *>(QStringLiteral("GridDragGhost")));
}

void TestDragGridWidget::nestedGrid_keepsKeyHandlingOnInnerGrid()
{
    // 特征测试：卡片内嵌套另一个网格时，外层不得抢占内层的 Space 拾取。
    // 修复前外层过滤器会装到内层子树上（后安装先调用），内层无法键盘拖拽。
    DragGridWidget outer;
    outer.setColumnCount(1);
    outer.setMinimumCellSize(QSize(200, 200));
    outer.setAnimationDuration(0);
    outer.resize(260, 260);
    outer.show();
    QVERIFY(QTest::qWaitForWindowExposed(&outer));

    auto *inner = new DragGridWidget; // 作为外层的一张卡片存在
    inner->setColumnCount(2);
    inner->setMinimumCellSize(QSize(80, 80));
    inner->setAnimationDuration(0);
    inner->setDragEnabled(true);

    auto *innerFirst = createItem(QStringLiteral("innerFirst"));
    auto *innerSecond = createItem(QStringLiteral("innerSecond"));
    innerFirst->setMinimumSize(80, 80);
    innerSecond->setMinimumSize(80, 80);
    inner->addWidget(innerFirst);
    inner->addWidget(innerSecond);

    outer.addWidget(inner);
    outer.setDragEnabled(true);
    outer.layout()->activate();
    inner->layout()->activate();
    QCoreApplication::processEvents();

    outer.activateWindow();
    innerFirst->setFocus();
    QCoreApplication::processEvents();

    QTest::keyClick(innerFirst, Qt::Key_Space);
    QCoreApplication::processEvents();

    auto *ghost = inner->findChild<QLabel *>(QStringLiteral("GridDragGhost"));
    QVERIFY(ghost);
    QCOMPARE(ghost->parentWidget(), static_cast<QWidget *>(inner));

    QTest::keyClick(innerFirst, Qt::Key_Return);
    QCoreApplication::processEvents();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}

void TestDragGridWidget::mousePressDuringKeyboardDrag_doesNotHideCard()
{
    // 特征测试：键盘拖拽进行中按下鼠标左键，不得覆盖拖拽状态机。
    // 修复前 mousePressEvent 会把状态改成 Pressed，m_draggedWidget 成为孤儿，
    // 卡片永久隐藏、镜像残留，Escape/Enter 都失效。
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
    QCoreApplication::processEvents();
    QVERIFY(grid.findChild<QLabel *>(QStringLiteral("GridDragGhost")));

    auto sendMouse = [&grid](QEvent::Type type, const QPoint &pos,
                             Qt::MouseButton button, Qt::MouseButtons buttons) {
        QMouseEvent event(type, pos, grid.mapToGlobal(pos), button, buttons, Qt::NoModifier);
        QCoreApplication::sendEvent(&grid, &event);
    };
    const QPoint clickPos = second->geometry().center();
    sendMouse(QEvent::MouseButtonPress, clickPos, Qt::LeftButton, Qt::LeftButton);
    sendMouse(QEvent::MouseButtonRelease, clickPos, Qt::LeftButton, Qt::NoButton);
    QCoreApplication::processEvents();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();

    QVERIFY(first->isVisible());
    QCOMPARE(orderSpy.count(), 0);
    QVERIFY(!grid.findChild<QLabel *>(QStringLiteral("GridDragGhost")));
    auto *placeholder = grid.findChild<QWidget *>(QStringLiteral("GridPlaceholder"));
    QVERIFY(placeholder);
    QVERIFY(!placeholder->isVisible());

    // 取消后仍可继续键盘拖拽。
    first->setFocus();
    QCoreApplication::processEvents();
    QTest::keyClick(&grid, Qt::Key_Space);
    QTest::keyClick(&grid, Qt::Key_Right);
    QTest::keyClick(&grid, Qt::Key_Return);
    QCoreApplication::processEvents();
    QCOMPARE(objectNames(grid.widgets()), QStringList({QStringLiteral("second"),
                                                       QStringLiteral("first")}));
}

void TestDragGridWidget::geometryAnimations_areReleasedAfterReorder()
{
    // 特征测试：几何动画结束后必须释放动画对象，不能只摘除表项。
    // 修复前动画对象以卡片为父对象留存，每次重排都会新增一批，长期运行持续增长。
    DragGridWidget grid;
    auto *first = createItem(QStringLiteral("first"));
    auto *second = createItem(QStringLiteral("second"));
    auto *third = createItem(QStringLiteral("third"));
    grid.setColumnCount(3);
    grid.setMinimumCellSize(QSize(100, 100));
    grid.setAnimationDuration(120);
    grid.resize(360, 140);
    grid.show();
    QVERIFY(QTest::qWaitForWindowExposed(&grid));
    grid.addWidget(first);
    grid.addWidget(second);
    grid.addWidget(third);
    grid.setDragEnabled(true);
    grid.layout()->activate();
    QCoreApplication::processEvents();

    auto geometryAnimations = [](QWidget *widget) {
        int count = 0;
        const QList<QPropertyAnimation *> animations = widget->findChildren<QPropertyAnimation *>();
        for (QPropertyAnimation *animation : animations) {
            if (animation->propertyName() == QByteArrayLiteral("geometry")) {
                ++count;
            }
        }
        return count;
    };

    for (int round = 0; round < 3; ++round) {
        QWidget *mover = grid.widgets().first();
        QWidget *target = grid.widgets().last();
        dragFromTo(&grid, mover->geometry().center(), target->geometry().center() + QPoint(60, 0));
        QTest::qWait(300);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QCoreApplication::processEvents();

        QVERIFY2(geometryAnimations(mover) <= 1,
                 qPrintable(QStringLiteral("round %1: %2 leaked animations")
                                .arg(round).arg(geometryAnimations(mover))));
    }
}

void TestDragGridWidget::spaceOnLateAddedCardButton_startsDrag()
{
    // 特征测试：卡片加入网格之后才创建的子控件，也必须被按键过滤器覆盖。
    // 修复前过滤器只在 addWidget() 时安装，运行期新增的按钮会吞掉 Space 拾取。
    DragGridWidget grid;
    auto *card = new QWidget;
    card->setObjectName(QStringLiteral("card"));
    card->setMinimumSize(100, 100);
    card->resize(100, 100);
    auto *cardLayout = new QVBoxLayout(card);

    auto *second = createItem(QStringLiteral("second"));
    prepareGrid(&grid, card, second);

    // 先加入网格，再补齐子控件（模拟运行期构建界面的卡片）。
    auto *button = new QPushButton(QStringLiteral("close"), card);
    cardLayout->addWidget(button);
    grid.setDragEnabled(true);
    grid.activateWindow();

    button->setFocus();
    QCoreApplication::processEvents();
    QCOMPARE(QApplication::focusWidget(), static_cast<QWidget *>(button));

    QSignalSpy clickedSpy(button, &QPushButton::clicked);
    QTest::keyClick(button, Qt::Key_Space);
    QCoreApplication::processEvents();

    QCOMPARE(clickedSpy.count(), 0);
    QVERIFY(grid.findChild<QLabel *>(QStringLiteral("GridDragGhost")));

    QTest::keyClick(button, Qt::Key_Return);
    QCoreApplication::processEvents();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(!grid.findChild<QLabel *>(QStringLiteral("GridDragGhost")));
    QVERIFY(card->isVisible());
}

QTEST_MAIN(TestDragGridWidget)

#include "TestDragGridWidget.moc"
