/*
 *
 */

#pragma once

#include <QDialog>
#include <discnet/adapter.hpp>

namespace Ui {
class AdapterDialog;
}

namespace discnet::sim::ui
{
    class AdapterDialog : public QDialog
    {
        Q_OBJECT

    public:
        explicit AdapterDialog(uint16_t node_id, QWidget *parent = nullptr);
        ~AdapterDialog();

        void setData(const adapter_t& adapter);
    private slots:
        void on_buttonBox_accepted();
        void on_buttonBox_rejected();

    signals:
        void accepted(const uint16_t node_id, const adapter_t adapter);

    private:
        Ui::AdapterDialog *ui;
        uint16_t m_node_id;
    };
} // !namespace discnet::sim::ui