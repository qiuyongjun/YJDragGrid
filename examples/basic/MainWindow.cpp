#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "CardWidget.h"
#include <QtDragGrid/DragGridWidget.h>

#include <QAbstractSpinBox>
#include <QCheckBox>
#include <QSlider>
#include <QSpinBox>

using QtDragGrid::DragGridWidget;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_customDraggableGrid = new DragGridWidget(ui->scrollArea_2, this);
    m_customDraggableGrid->setEmptyText(QStringLiteral("暂无卡片，点击“添加卡片”开始演示"));
    connect(m_customDraggableGrid, QOverload<>::of(&DragGridWidget::orderChanged),
            this, &MainWindow::slot_orderChanged);
    ui->gridLayout->addWidget(m_customDraggableGrid);

    setupDemoControls();
    populateInitialCards(7);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_addView_clicked()
{
    addDemoCard();
}

void MainWindow::on_pushButton_clearView_clicked()
{
    if (!m_customDraggableGrid) {
        return;
    }

    m_customDraggableGrid->clear();
    refreshWidgetLabels();
}

void MainWindow::slot_viewRemove()
{
    auto *cardWidget = qobject_cast<CardWidget*>(sender());
    if (!cardWidget || !m_customDraggableGrid) {
        return;
    }

    m_customDraggableGrid->deleteWidget(cardWidget);
    refreshWidgetLabels();
}

void MainWindow::slot_orderChanged()
{
    refreshWidgetLabels();
}

void MainWindow::setupDemoControls()
{
    ui->spinBox_columnCount->setRange(1, 8);
    ui->spinBox_columnCount->setValue(4);
    ui->spinBox_columnCount->setButtonSymbols(QAbstractSpinBox::NoButtons);
    ui->checkBox_dragEnabled->setChecked(true);
    ui->checkBox_compactMode->setChecked(true);
    ui->slider_animationDuration->setRange(0, 600);
    ui->slider_animationDuration->setValue(200);
    ui->spinBox_animationDuration->setRange(0, 600);
    ui->spinBox_animationDuration->setValue(200);
    ui->spinBox_animationDuration->setButtonSymbols(QAbstractSpinBox::NoButtons);

    m_customDraggableGrid->setColumnCount(ui->spinBox_columnCount->value());
    m_customDraggableGrid->setDragEnabled(ui->checkBox_dragEnabled->isChecked());
    m_customDraggableGrid->setFillIncompleteRowEnabled(ui->checkBox_compactMode->isChecked());
    m_customDraggableGrid->setAnimationDuration(ui->slider_animationDuration->value());

    connect(ui->spinBox_columnCount, QOverload<int>::of(&QSpinBox::valueChanged),
            m_customDraggableGrid, &DragGridWidget::setColumnCount);
    connect(ui->checkBox_dragEnabled, &QCheckBox::toggled,
            m_customDraggableGrid, &DragGridWidget::setDragEnabled);
    connect(ui->checkBox_compactMode, &QCheckBox::toggled,
            m_customDraggableGrid, &DragGridWidget::setFillIncompleteRowEnabled);

    connect(ui->slider_animationDuration, &QSlider::valueChanged,
            ui->spinBox_animationDuration, &QSpinBox::setValue);
    connect(ui->spinBox_animationDuration, QOverload<int>::of(&QSpinBox::valueChanged),
            ui->slider_animationDuration, &QSlider::setValue);
    connect(ui->slider_animationDuration, &QSlider::valueChanged,
            m_customDraggableGrid, &DragGridWidget::setAnimationDuration);
}

void MainWindow::populateInitialCards(int count)
{
    for (int index = 0; index < count; ++index) {
        addDemoCard();
    }
}

void MainWindow::addDemoCard()
{
    if (!m_customDraggableGrid) {
        return;
    }

    CardWidget *cardWidget = new CardWidget(this);
    connect(cardWidget, &CardWidget::signal_closeWidget, this, &MainWindow::slot_viewRemove);

    m_customDraggableGrid->addWidget(cardWidget);
    refreshWidgetLabels();
}

void MainWindow::refreshWidgetLabels()
{
    if (!m_customDraggableGrid) {
        return;
    }

    const QList<QWidget *> orderedWidgets = m_customDraggableGrid->widgets();
    for (int index = 0; index < orderedWidgets.size(); ++index) {
        auto *cardWidget = qobject_cast<CardWidget *>(orderedWidgets[index]);
        if (!cardWidget) {
            continue;
        }

        cardWidget->setLabel(QString("%1 / %2")
                                  .arg(index + 1)
                                  .arg(m_customDraggableGrid->count()));
    }
}
