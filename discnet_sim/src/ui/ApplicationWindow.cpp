#include <QPushButton>
#include <QLabel>
#include "ui/ApplicationWindow.h"
#include "ui_applicationwindow.h"
#include "ui/widgets/ToolBoxItem.h"


namespace discnet::sim::ui
{
    ApplicationWindow::ApplicationWindow(QWidget *parent)
        : QMainWindow(parent)
        , ui(new Ui::ApplicationWindow)
    {
        ui->setupUi(this);
        setAcceptDrops(true);

        auto layout = new QVBoxLayout;
        auto node = new ToolBoxItem(ToolBoxItemType::Node, this);
        auto router = new ToolBoxItem(ToolBoxItemType::Router, this);
        layout->addWidget(node);
        layout->addWidget(router);
        layout->addStretch();
        ui->tbPage1->setLayout(layout);

        m_scene = new SimulatorScene(this);
        m_scene->setSceneRect(QRectF(0, 0, 5000, 5000));
        m_view = new SimulatorGraphicsView(m_scene, this);
        m_view->setDragMode(QGraphicsView::RubberBandDrag);
        ui->scene->layout()->addWidget(m_view);
    }

    ApplicationWindow::~ApplicationWindow()
    {
        delete ui;
    }
} // !namespace discnet::sim::ui