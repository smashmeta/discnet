#include "ui/widgets/nodedialog.h"
#include "ui_nodedialog.h"

namespace discnet::sim::ui
{
NodeDialog::NodeDialog(const uint16_t node_id, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::NodeDialog)
    , m_node_id(node_id)
{
    ui->setupUi(this);
    ui->lblNodeId->setText(QString::number(m_node_id));
}

NodeDialog::~NodeDialog()
{
    delete ui;
}

QTextEdit* NodeDialog::log()
{
    return ui->teLog;
}

void NodeDialog::on_btnEnable_clicked()
{
    if (ui->btnEnable->text() == "Start")
    {
        ui->btnEnable->setText("Stop");
        emit client_enabled(m_node_id, true, this);       
    }
    else
    {
        ui->btnEnable->setText("Start");
        emit client_enabled(m_node_id, false, this); 
    }
}
} // !namespace discnet::sim::ui