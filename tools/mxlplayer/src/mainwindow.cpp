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
#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "playcontrol.h"

#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>
#include <QProgressDialog>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->setWindowFlags(windowFlags()& ~Qt::WindowMaximizeButtonHint);
    this->setFixedSize(this->width(), this->height());
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::do_showError(QString errorTitle, QString errorMsg)
{
    progressDialog -> close();
    QMessageBox::critical(this, errorTitle, errorMsg);
}

void MainWindow::do_completed()
{
    progressDialog->close();
    QMessageBox::information(this, tr("MP4导出"), tr("导出视频完成。"));
    delete progressDialog;
    progressDialog = nullptr;
}

void MainWindow::do_canceled()
{
    progressDialog->close();
    doExport->terminate();
    delete progressDialog;
    progressDialog = nullptr;
}

void MainWindow::on_pushButtonExportMP4_clicked()
{
    QFileInfo mxlInfo(ui->lineEditMXLPath->text());
    if(!mxlInfo.exists())
    {
        QMessageBox::critical(this, tr("载入文件出错"), tr("无法载入MXL文件。"));
        return;
    }

    QFileInfo wavInfo(ui->lineEditWAVPath->text());
    if(!wavInfo.exists())
    {
        if(QMessageBox::question(this, tr("载入文件出错"), tr("无法打开WAV文件，输出文件将没有音频。\n要继续吗？")) == QMessageBox::No)
            return;
        else
            ui->lineEditWAVPath->setText("");
    }

    QString videoPath = QFileDialog::getSaveFileName(this, tr("选择保存MP4文件位置"), QDir::homePath(), tr("H264 MP4视频 (*.mp4)"));

    AVP::AVPSize size = getSize();

    doExport = new TDoExport(this, ui->lineEditMXLPath->text(), ui->lineEditWAVPath->text(), videoPath, size);
    connect(doExport, SIGNAL(showError(QString,QString)), this, SLOT(do_showError(QString,QString)));
    connect(doExport, SIGNAL(completed()), this, SLOT(do_completed()));
    connect(doExport, &QThread::finished, doExport, &QObject::deleteLater);

    progressDialog = new QProgressDialog(tr("导出视频文件..."), tr("取消"), 0, 0, this);
    progressDialogBar = new QProgressBar(progressDialog);
    progressDialogBar->setTextVisible(false);
    progressDialog->setWindowTitle(tr("MP4导出"));
    progressDialog->setWindowModality(Qt::WindowModal);
    progressDialog->setAutoReset(false);
    progressDialog->setAutoClose(false);
    progressDialog->setFixedSize(300, 100);
    progressDialog->setBar(progressDialogBar);
    progressDialog->show();
    connect(doExport, SIGNAL(setProgressText(QString)), progressDialog, SLOT(setLabelText(QString)));
    connect(doExport, SIGNAL(setProgressMax(int)), progressDialog, SLOT(setMaximum(int)));
    connect(doExport, SIGNAL(setProgress(int)), progressDialog, SLOT(setValue(int)));
    connect(progressDialog, SIGNAL(canceled()), this, SLOT(do_canceled()));

    doExport->start();
}

AVP::AVPSize MainWindow::getSize()
{
    if(ui->radioButtonSmall->isChecked())
        return AVP::kAVPSmallSize;
    else if(ui->radioButtonMedium->isChecked())
        return AVP::kAVPMediumSize;
    else if(ui->radioButtonLarge->isChecked())
        return AVP::kAVPLargeSize;
    return AVP::kAVPMediumSize;
}


void MainWindow::on_pushButtonMXLBrowse_clicked()
{
    ui->lineEditMXLPath->setText(QFileDialog::getOpenFileName(this, tr("打开MXL文件"), QDir::homePath(), "Christie PandorasBox MPEG Video (*.mxl)"));
}


void MainWindow::on_pushButtonWAVBrowse_clicked()
{
    ui->lineEditWAVPath->setText(QFileDialog::getOpenFileName(this, tr("打开WAV文件"), QDir::homePath(), "WAVE Audio (*.wav)"));
}


void MainWindow::on_pushButtonPlay_clicked()
{
    PlayControl *playControl = new PlayControl(this, ui->lineEditMXLPath->text(), ui->lineEditWAVPath->text(), getSize());

    playControl->setParent(NULL);
    playControl->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);

    playControl->show();
    this->hide();
}

