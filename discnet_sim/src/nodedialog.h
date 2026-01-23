#ifndef NODEDIALOG_H
#define NODEDIALOG_H

#include <QDialog>
#include <QTextEdit>

namespace Ui {
class NodeDialog;
}

class NodeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NodeDialog(QWidget *parent = nullptr);
    ~NodeDialog();

    QTextEdit* log();
private:
    Ui::NodeDialog *ui;
};

#endif // NODEDIALOG_H
