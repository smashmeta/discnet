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

namespace discnet::sim::ui
{
    class NodeDialog : public QDialog
    {
        Q_OBJECT

    public:
        explicit NodeDialog(uint16_t node_id, QWidget *parent = nullptr);
        ~NodeDialog();

        QTextEdit* log();

    signals:
        void add_adapter(const uint16_t node_id, const std::string& adapter_ip);
        void client_enabled(const uint16_t node_id, const bool enable, NodeDialog* dialog);

    private slots:
        void on_btnAddSwitch_clicked();
        void on_btnEnable_clicked();

    private:
        Ui::NodeDialog *ui;
        uint16_t m_node_id;
        QStringListModel m_adapterModel;
    };
} // !namespace discnet::sim::ui
