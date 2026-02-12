/*
 *
 */

#pragma once

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
    private:
        Ui::ApplicationWindow *ui;
        SimulatorScene* m_scene;
        SimulatorGraphicsView* m_view;
    };
} // !namespace discnet::sim::ui
