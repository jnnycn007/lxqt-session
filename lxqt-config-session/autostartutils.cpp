/* This file is part of the LXQt project. <https://lxqt.org>
 * Copyright (C) 2015 Luís Pereira <luis.artur.pereira@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include "autostartutils.h"
#include <XdgDesktopFile>

#include <QString>

using namespace Qt::Literals::StringLiterals;

bool AutostartUtils::showOnlyInLXQt(const XdgDesktopFile &file)
{
    const QString values = file.value("OnlyShowIn"_L1).toString();
    const QStringList desktops = values.split(u';', Qt::SkipEmptyParts);
    return desktops.contains("LXQt"_L1);
}

bool AutostartUtils::isLXQtModule(const XdgDesktopFile& file)
{
    return file.value(QLatin1String("X-LXQt-Module"), false).toBool();
}

