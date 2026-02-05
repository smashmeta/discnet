#ifndef APPLICATIONWINDOW_H
#define APPLICATIONWINDOW_H

#include <QMainWindow>
#include <QGraphicsView>
#include "ui/widgets/SimulatorScene.h"

namespace Ui {
class ApplicationWindow;
}

namespace discnet::sim::ui
{
    class ApplicationWindow : public QMainWindow
    {
        Q_OBJECT

    public:
        explicit ApplicationWindow(QWidget *parent = nullptr);
        ~ApplicationWindow();
    private:
        Ui::ApplicationWindow *ui;
        SimulatorScene* m_scene;
        QGraphicsView* m_view;
    };
}


#endif // APPLICATIONWINDOW_H
