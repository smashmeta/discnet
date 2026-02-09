/*
 *
 */

#include <iostream>
#include "AdapterDialog.h"
#include "ui_AdapterDialog.h"


namespace discnet::sim::ui
{
    AdapterDialog::AdapterDialog(uint16_t node_id, QWidget *parent)
        : QDialog(parent)
        , ui(new Ui::AdapterDialog)
        , m_node_id(node_id)
    {
        ui->setupUi(this);
    }

    AdapterDialog::~AdapterDialog()
    {
        delete ui;
    }

    void AdapterDialog::setData(const adapter_t& adapter)
    {
        ui->leGuid->setText(adapter.m_guid.c_str());
        ui->leMac->setText(adapter.m_mac_address.c_str());
        ui->leIndex->setText(QString::number(adapter.m_index));
        ui->leName->setText(adapter.m_name.c_str());
        ui->leDescription->setText(adapter.m_description.c_str());
        auto [address, mask] = adapter.m_address_list.front();
        ui->leAddress->setText(address.to_string().c_str());
        ui->leMask->setText(mask.to_string().c_str());
        ui->leMtu->setText(QString::number(adapter.m_mtu));
        ui->cbEnabled->setChecked(adapter.m_enabled);
        ui->cbLoopback->setChecked(adapter.m_loopback);
    }

    void AdapterDialog::on_buttonBox_accepted()
    {
        try
        {
            adapter_t adapter;
            adapter.m_guid = ui->leGuid->text().toStdString();
            adapter.m_mac_address = ui->leMac->text().toStdString();
            adapter.m_index = static_cast<uint8_t>(ui->leIndex->text().toInt());
            adapter.m_name = ui->leName->text().toStdString();
            adapter.m_description = ui->leDescription->text().toStdString();
            address_t address = boost::asio::ip::make_address_v4(ui->leAddress->text().toStdString());
            address_t mask = boost::asio::ip::make_address_v4(ui->leMask->text().toStdString());
            adapter.m_address_list.push_back(std::make_pair(address, mask));
            adapter.m_mtu = static_cast<uint16_t>(ui->leMtu->text().toInt());
            adapter.m_enabled = ui->cbEnabled->isChecked();
            adapter.m_loopback = ui->cbLoopback->isChecked();
            emit accepted(m_node_id, adapter);
        }
        catch (const std::exception& ex)
        {
            std::cout << "exception in AdapterDialog::on_buttonBox_accepted. message: " << ex.what() << std::endl;
        }
    }

    void AdapterDialog::on_buttonBox_rejected()
    {

    }
}
