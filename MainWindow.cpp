#include "MainWindow.h"
#include "./ui_MainWindow.h"

#include "ChartWidget.h"
#include "YJGridWidget.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_customDraggableGrid = new YJGridWidget(ui->scrollArea_2,this);
    m_customDraggableGrid->setDragEnabled(true);
    m_customDraggableGrid->setColumnCount(4);
    connect(m_customDraggableGrid, &YJGridWidget::orderChanged, this, &MainWindow::slot_orderChanged);
    ui->gridLayout->addWidget(m_customDraggableGrid);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_addView_clicked()
{
    ChartWidget *chartWidget = new ChartWidget(this);
    connect(chartWidget, &ChartWidget::signal_closeWidget, this, &MainWindow::slot_viewRemove);

    m_customDraggableGrid->addWidget(chartWidget);
    refreshWidgetLabels();
}

void MainWindow::slot_viewRemove()
{
    auto *chartWidget = qobject_cast<ChartWidget*>(sender());
    if (!chartWidget || !m_customDraggableGrid) {
        return;
    }

    m_customDraggableGrid->deleteWidget(chartWidget);
    refreshWidgetLabels();
}

void MainWindow::slot_orderChanged()
{
    refreshWidgetLabels();
}

void MainWindow::refreshWidgetLabels()
{
    if (!m_customDraggableGrid) {
        return;
    }

    const QList<QWidget *> orderedWidgets = m_customDraggableGrid->widgets();
    for (int index = 0; index < orderedWidgets.size(); ++index) {
        auto *chartWidget = qobject_cast<ChartWidget *>(orderedWidgets[index]);
        if (!chartWidget) {
            continue;
        }

        chartWidget->setLabel(QString("%1 / %2")
                                  .arg(index + 1)
                                  .arg(m_customDraggableGrid->count()));
    }
}

