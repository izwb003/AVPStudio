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

#include "settings.h"

AVP::AVPSettings settings;

QString AVP::AVPSettings::getSizeString()
{
    switch(this->size) {
    case kAVPSmallSize:
        return "Small";
    case kAVPMediumSize:
        return "Medium";
    case kAVPLargeSize:
        return "Large";
    default:
        return "";
    }
}

QString AVP::AVPSettings::getSizeResolution()
{
    switch(this->size) {
    case kAVPSmallSize:
        return "2830x1080";
    case kAVPMediumSize:
        return "4633x1080";
    case kAVPLargeSize:
        return "6167x1080";
    default:
        return "";
    }
}

QString AVP::AVPSettings::getRealSize()
{
    switch(this->size) {
    case kAVPSmallSize:
        return "5.5Mx2.1M";
    case kAVPMediumSize:
        return "9Mx2.1M";
    case kAVPLargeSize:
        return "12Mx2.1M";
    default:
        return "";
    }
}
