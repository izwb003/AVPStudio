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
#ifndef PAGEEDIT_H
#define PAGEEDIT_H

#include <QWidget>
#include <QMediaPlayer>

namespace Ui {
class PageEdit;
}

class PageEdit : public QWidget
{
    Q_OBJECT

public:
    explicit PageEdit(QWidget *parent = nullptr);
    ~PageEdit();

signals:
    void toProcess();

private slots:
    void do_init();

    void do_positionChanged(qint64 position);

    void do_durationChanged(qint64 duration);

    void do_playStateChanged(QMediaPlayer::PlaybackState state);

    void do_playErrorOccured(QMediaPlayer::Error error, const QString &errorString);

    void on_toolButtonPausePlay_clicked(bool checked);

    void on_verticalSliderVolume_valueChanged(int value);

    void on_horizontalSliderPosition_valueChanged(int value);

    void on_checkBoxPadding_clicked(bool checked);

    void on_pushButtonOutput_clicked();

private:
    Ui::PageEdit *ui;

    QMediaPlayer *player;

    QString durationTime = "00:00";
    QString positionTime = "00:00";
};

#endif // PAGEEDIT_H
