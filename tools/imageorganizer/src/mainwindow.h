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

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

namespace AVP {
enum AVPSize {
    kAVPSmallSize,
    kAVPMediumSize,
    kAVPLargeSize
};
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_radioButtonSmallSize_clicked(bool checked);

    void on_radioButtonMediumSize_clicked(bool checked);

    void on_radioButtonLargeSize_clicked(bool checked);

    void on_pushButtonQuit_clicked();

    void on_checkBoxExtend_stateChanged(int arg1);

    void on_checkBoxDolbyNaming_stateChanged(int arg1);

    void on_pushButtonInBrowse_clicked();

    void on_lineEditInPath_editingFinished();

    void on_pushButtonConvert_clicked();

private:
    Ui::MainWindow *ui;

    void doConversion();

    struct
    {
        AVP::AVPSize size = AVP::kAVPMediumSize;
        int width = 4633;
        int height = 1080;
        QString fileInputPath = "";
        QString fileOutputPath = "";
        bool isDolbyNaming = true;
        bool isExtended = true;
    }
    settings;
};
#endif // MAINWINDOW_H
