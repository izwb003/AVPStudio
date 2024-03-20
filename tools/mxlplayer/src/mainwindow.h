/*
 * Copyright (C) 2024 Steven Song (izwb003)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProgressBar>
#include <QProgressDialog>

#include "doexport.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void do_showError(QString errorTitle, QString errorMsg);

    void do_completed();

    void do_canceled();

    void on_pushButtonExportMP4_clicked();

    void on_pushButtonMXLBrowse_clicked();

    void on_pushButtonWAVBrowse_clicked();

private:
    Ui::MainWindow *ui;

    TDoExport *doExport;

    QProgressBar *progressDialogBar;
    QProgressDialog *progressDialog;

    AVP::AVPSize getSize();
};
#endif // MAINWINDOW_H
