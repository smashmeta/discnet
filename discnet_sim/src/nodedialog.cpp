#include "nodedialog.h"
#include "ui_nodedialog.h"

NodeDialog::NodeDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::NodeDialog)
{
    ui->setupUi(this);
}

NodeDialog::~NodeDialog()
{
    delete ui;
}

QTextEdit* NodeDialog::log()
{
    return ui->teLog;
}