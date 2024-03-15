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

#include "genprocess.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->setWindowFlags(windowFlags()& ~Qt::WindowMaximizeButtonHint);
    this->setFixedSize(this->width(), this->height());

    ui->statusbar->showMessage(tr("就绪"));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::do_setProgressMax(int64_t num)
{
    ui->progressBar->setMaximum(num);
}

void MainWindow::do_setProgress(int64_t num)
{
    ui->progressBar->setValue(num);
}

void MainWindow::do_showError(QString errorStr, QString title)
{
    ui->statusbar->showMessage(tr("错误"));
    this->setWindowTitle("AVPStudio WAVGenerator");
    ui->pushButtonConvert->setEnabled(true);
    ui->pushButtonCancel->setEnabled(false);
    QMessageBox::critical(this, title, errorStr);
}

void MainWindow::do_processFinished()
{
    ui->statusbar->showMessage(tr("完成"));
    this->setWindowTitle("AVPStudio WAVGenerator");
    ui->pushButtonConvert->setEnabled(true);
    ui->pushButtonCancel->setEnabled(false);
}

void MainWindow::on_pushButtonBrowseInputFile_clicked()
{
    ui->lineEditInputFile->setText(QFileDialog::getOpenFileName(this, tr("选择输入音频文件..."), QDir::homePath(), tr("音频文件 (*.mp3 *.m4a *.wav *.aac *.flac *.ec3 *.eac3 *.wma)")));
}


void MainWindow::on_pushButtonBrowseOutputFile_clicked()
{
    ui->lineEditOutputFile->setText(QFileDialog::getSaveFileName(this, tr("选择输出WAV文件保存位置..."), QDir::homePath(), tr("WAV音频文件 (*.wav)")));
    if(ui->checkBoxDolbyNaming->isChecked() && !ui->lineEditOutputFile->text().isEmpty())
    {
        QFileInfo fileInfo = QFileInfo(ui->lineEditOutputFile->text());
        ui->lineEditOutputFile->setText(fileInfo.absolutePath() + "/" + fileInfo.baseName() + "_audio_all.wav");
    }
}


void MainWindow::on_pushButtonConvert_clicked()
{
    QFileInfo inputFileInfo = QFileInfo(ui->lineEditInputFile->text());
    QFileInfo outputFileInfo = QFileInfo(ui->lineEditOutputFile->text());
    if(!inputFileInfo.exists())
    {
        QMessageBox::critical(this, tr("打开输入文件出错"), tr("未指定输入文件或输入文件不存在。"));
        return;
    }
    if(outputFileInfo.exists())
    {
        if(QMessageBox::question(this, tr("打开输出文件出错"), tr("输出文件已存在。要覆盖吗？")) != QMessageBox::Yes)
            return;
    }

    TGenProcess *genProcess = new TGenProcess(this, ui->lineEditInputFile->text(), ui->lineEditOutputFile->text(), ui->spinBoxVolume->value());

    connect(genProcess, SIGNAL(setProgressMax(int64_t)), this, SLOT(do_setProgressMax(int64_t)));
    connect(genProcess, SIGNAL(setProgress(int64_t)), this, SLOT(do_setProgress(int64_t)));
    connect(genProcess, SIGNAL(finished()), this, SLOT(do_processFinished()));
    connect(genProcess, SIGNAL(showError(QString,QString)), this, SLOT(do_showError(QString,QString)));
    connect(genProcess, &QThread::finished, genProcess, &QObject::deleteLater);

    ui->statusbar->showMessage(tr("正在生成") + outputFileInfo.fileName() + "...");
    this->setWindowTitle("AVPStudio WAVGenerator - Generating");
    ui->pushButtonConvert->setEnabled(false);
    ui->pushButtonCancel->setEnabled(true);

    genProcess->start();
}


void MainWindow::on_checkBoxDolbyNaming_stateChanged(int arg1)
{
    if(!ui->lineEditOutputFile->text().isEmpty())
    {
        if(arg1)
        {
            QFileInfo fileInfo = QFileInfo(ui->lineEditOutputFile->text());
            ui->lineEditOutputFile->setText(fileInfo.absolutePath() + "/" + fileInfo.baseName() + "_audio_all.wav");
        }
        else
        {
            QFileInfo fileInfo = QFileInfo(ui->lineEditOutputFile->text());
            ui->lineEditOutputFile->setText(fileInfo.absolutePath() + "/" + fileInfo.baseName().section("_", 0, 0) + ".wav");
        }
    }
}

