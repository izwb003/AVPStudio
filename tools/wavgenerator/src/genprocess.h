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
#ifndef TGENPROCESS_H
#define TGENPROCESS_H

#include <QThread>

class TGenProcess : public QThread
{
    Q_OBJECT
public:
    explicit TGenProcess(QObject *parent = nullptr, QString inputFilePath = "", QString outputFilePath = "", int volumePercent = 100);

    QString inputFilePath;
    QString outputFilePath;
    int volumePercent;

protected:
    void run();

signals:
    void setProgressMax(int64_t num);
    void setProgress(int64_t num);
    void showError(QString errorStr, QString title);
    void completed();
};

#endif // TGENPROCESS_H
