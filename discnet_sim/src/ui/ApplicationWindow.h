/*
 *
 */

#pragma once

#include <QAction>
#include <QMainWindow>
#include <QGraphicsView>
#include "ui/widgets/SimulatorScene.h"

namespace Ui {
class ApplicationWindow;
}

namespace discnet::sim::ui
{
    class SimulatorGraphicsView : public QGraphicsView
    {
    public:
        SimulatorGraphicsView(QGraphicsScene *scene, QWidget *parent = nullptr)
            : QGraphicsView(scene, parent)
        {
            // nothing for now   
        }
    protected:
        void keyPressEvent(QKeyEvent *event) override
        {
            if (event->key() == Qt::Key_Control) 
            {
                setDragMode(QGraphicsView::NoDrag);
            }
        }

        void keyReleaseEvent(QKeyEvent *event) override
        {
            if (event->key() == Qt::Key_Control) 
            {
                setDragMode(QGraphicsView::RubberBandDrag);
            }
        }
    };

    class ApplicationWindow : public QMainWindow
    {
        Q_OBJECT

    public:
        explicit ApplicationWindow(QWidget *parent = nullptr);
        ~ApplicationWindow();

    protected:
        void open_scenario();
        void save_scenario();
    
    private:
        Ui::ApplicationWindow *ui;
        SimulatorScene* m_scene;
        SimulatorGraphicsView* m_view;
        QAction* m_open;
        QAction* m_save;
    };
} // !namespace discnet::sim::ui
