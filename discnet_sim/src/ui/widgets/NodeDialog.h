/*
 *
 */

#pragma once

#include <QDialog>
#include <QTextEdit>
#include <QStringListModel>


namespace Ui {
class NodeDialog;
}

namespace discnet::sim::models 
{
    class RouteModel;
}

namespace discnet::sim::ui
{
    class NodeDialog : public QDialog
    {
        Q_OBJECT

    public:
        explicit NodeDialog(uint16_t node_id, discnet::sim::models::RouteModel* model, QWidget *parent = nullptr);
        ~NodeDialog();

        QTextEdit* log();

    private:
        Ui::NodeDialog *ui;
        uint16_t m_node_id;
        discnet::sim::models::RouteModel* m_route_model;
    };
} // !namespace discnet::sim::ui
