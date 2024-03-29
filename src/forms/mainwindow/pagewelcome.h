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
#ifndef PAGEWELCOME_H
#define PAGEWELCOME_H

#include <QTreeWidgetItem>
#include <QWidget>

namespace Ui {
class PageWelcome;
}

class PageWelcome : public QWidget
{
    Q_OBJECT

public:
    explicit PageWelcome(QWidget *parent = nullptr);
    ~PageWelcome();

signals:
    void createContent();

private:
    void fillCinemaList();

private slots:
    void on_pushButtonExit_clicked();

    void on_pushButtonNext_clicked();

    void on_treeWidgetCinemas_itemActivated(QTreeWidgetItem *item, int column);

    void on_radioButtonSmall_toggled(bool checked);

    void on_radioButtonMedium_toggled(bool checked);

    void on_radioButtonLarge_toggled(bool checked);

private:
    Ui::PageWelcome *ui;
};

#endif // PAGEWELCOME_H
