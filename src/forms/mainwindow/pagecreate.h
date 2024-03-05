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
#ifndef PAGECREATE_H
#define PAGECREATE_H

#include <QWidget>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>

namespace Ui {
class PageCreate;
}

class PageCreate : public QWidget
{
    Q_OBJECT

protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    void dropEvent(QDropEvent *event);

public:
    explicit PageCreate(QWidget *parent = nullptr);
    ~PageCreate();

signals:
    void editContent();

private slots:
    void do_init();

    void on_labelDragText_linkActivated(const QString &link);

private:
    Ui::PageCreate *ui;

    void rewriteLabelDragText();
};

#endif // PAGECREATE_H
