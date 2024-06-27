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
#include <QNetworkAccessManager>

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

signals:
    void openFile(QString link);
    void toProcess();

private:
    Ui::MainWindow *ui;

    QNetworkAccessManager *updateChecker;
    void checkUpdate();

private slots:
    void do_createContent();
    void do_editContent();
    void do_toProcess();
    void do_toCompleted(bool isError, QString errorStr);
    void do_checkUpdateFinished(QNetworkReply* reply);
    void on_actionAbout_triggered();
    void on_actionExit_triggered();
    void on_actionNewContent_triggered();
    void on_actionOpenFile_triggered();
    void on_actionWavGenerator_triggered();
    void on_actionImageOrganizer_triggered();
    void on_actionMXLPlayer_triggered();
    void on_actionOpenSource_triggered();
    void on_actionCheckUpdate_triggered();
    void on_action_help_triggered();
    void on_action_report_triggered();
    void on_action_follow_triggered();
    void on_action_donate_triggered();
};
#endif // MAINWINDOW_H
