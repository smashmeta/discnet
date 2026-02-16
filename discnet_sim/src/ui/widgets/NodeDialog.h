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
        void client_enabled(const uint16_t node_id, const bool enable, NodeDialog* dialog);

    private slots:
        void on_btnEnable_clicked();

    private:
        Ui::NodeDialog *ui;
        uint16_t m_node_id;
    };
} // !namespace discnet::sim::ui
