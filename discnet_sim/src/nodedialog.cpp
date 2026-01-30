#include "nodedialog.h"
#include "ui_nodedialog.h"

NodeDialog::NodeDialog(uint16_t node_id, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::NodeDialog)
    , m_node_id(node_id)
{
    ui->setupUi(this);
    ui->lwAdapters->setModel(&m_adapterModel);
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

void NodeDialog::on_btnAddSwitch_clicked()
{
    QString adapter_ip = ui->leAdapterIp->text();

    // Get the current number of rows (where the new item will be inserted)
    int row = m_adapterModel.rowCount();

    // Notify views that rows are about to be inserted
    if (m_adapterModel.insertRow(row)) {
        // Get the index of the newly inserted row
        QModelIndex index = m_adapterModel.index(row, 0);

        // Set the data for the new item in the "DisplayRole"
        m_adapterModel.setData(index, adapter_ip, Qt::DisplayRole);
    }

    emit add_adapter(m_node_id, adapter_ip.toStdString());
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