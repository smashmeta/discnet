/*
 *
 */

#include "models/RouteModel.h"
#include "ui/widgets/nodedialog.h"
#include "ui_nodedialog.h"

namespace discnet::sim::ui
{
NodeDialog::NodeDialog(const uint16_t node_id, discnet::sim::models::RouteModel* model, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::NodeDialog)
    , m_node_id(node_id)
    , m_route_model(model)
{
    ui->setupUi(this);
    ui->lblNodeId->setText(QString::number(m_node_id));
    ui->tvTopology->setModel(model);
}

NodeDialog::~NodeDialog()
{
    delete ui;
}

QTextEdit* NodeDialog::log()
{
    return ui->teLog;
}
} // !namespace discnet::sim::ui