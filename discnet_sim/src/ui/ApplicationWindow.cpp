#include <QPushButton>
#include <QLabel>
#include <QFile>
#include <QIcon>
#include <QFileDialog>
#include <QTextStream>
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

        m_open = new QAction(QIcon(":/images/open.png"), "Open", this);
        connect(m_open, &QAction::triggered, this, &ApplicationWindow::open_scenario);
        ui->toolBar->addAction(m_open);

        m_save = new QAction(QIcon(":/images/save.png"), "Save", this);
        connect(m_save, &QAction::triggered, this, &ApplicationWindow::save_scenario);
        ui->toolBar->addAction(m_save);
        

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

    void ApplicationWindow::open_scenario()
    {
        QString file_name = QFileDialog::getOpenFileName(this, "Open File", QDir::homePath(), "All Files (*.*)");
        if (!file_name.isEmpty()) 
        {
            QFile file(file_name); //
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) 
            {
                return;
            }

            QByteArray json_byte_array = file.readAll(); 
            file.close();
            std::string json(json_byte_array.constData(), json_byte_array.length());
            m_scene->load(json);
        }
    }

    void ApplicationWindow::save_scenario()
    {
        QString file_name = QFileDialog::getSaveFileName(this, "Open File", QDir::homePath(), "All Files (*.*)");
        if (file_name.isEmpty())
        {
            return;
        }
        
        QFile file(file_name);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) 
        {
            return;
        }

        QTextStream out(&file);
        out << QString::fromStdString(m_scene->serialize());
        file.close();
    }
} // !namespace discnet::sim::ui