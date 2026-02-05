// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "ui/ApplicationWindow.h"
#include "mainwindow.h"
#include <QApplication>

int main(int argv, char *args[])
{
    QApplication app(argv, args);
    // MainWindow mainWindow;
    discnet::sim::ui::ApplicationWindow mainWindow;
    mainWindow.setGeometry(100, 100, 800, 500);
    mainWindow.show();

    return app.exec();
}
