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
#ifndef SETTINGS_H
#define SETTINGS_H

extern "C" {
#include <libavutil/rational.h>
#include <libavutil/pixfmt.h>
}

#include <QString>
#include <QFileInfo>

namespace AVP {

enum AVPSize {
    kAVPSmallSize,
    kAVPMediumSize,
    kAVPLargeSize
};

struct ColorSettings {
    AVColorPrimaries outputColorPrimary = AVCOL_PRI_BT470M;
    AVColorTransferCharacteristic outputVideoColorTrac = AVCOL_TRC_GAMMA22;
    AVColorSpace outputVideoColorSpace = AVCOL_SPC_FCC;
};

class AVPSettings {
public:
    AVPSize size = kAVPMediumSize;
    QString getSizeString();
    QString getSizeResolution();
    QString getRealSize();

    QString inputVideoPath;
    QFileInfo inputVideoInfo;

    double outputVideoBitRate = 20.0;
    AVRational outputFrameRate = av_make_q(24, 1);
    ColorSettings outputColor;
    QString outputFileName = "";
    bool useDolbyNaming = true;
    bool scalePicture = false;
    int outputVolume = 100;

    QString getOutputVideoFinalName();
    QString getOutputAudioFinalName();

    QString outputFilePath;
};
}

extern AVP::AVPSettings settings;

#endif // SETTINGS_H
