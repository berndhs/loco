
#include "loco-global.h"

/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (C) 2010, Bernd Stramm
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/


uint qHash(const QPoint& p)
{
  return p.x() * 17 ^ p.y();
}

namespace loco
{

QPointF tileForCoordinate(qreal lat, qreal lng, int zoom)
{
  qreal zn = static_cast<qreal>(1 << zoom);
  qreal tx = (lng + 180.0) / 360.0;
  qreal ty = (1.0 - log(tan(lat * M_PI / 180.0) + 1.0 / cos(lat * M_PI / 180.0)) / M_PI) / 2.0;
  return QPointF(tx * zn, ty * zn);
}

qreal longitudeFromTile(qreal tx, int zoom)
{
  qreal zn = static_cast<qreal>(1 << zoom);
  qreal lat = tx / zn * 360.0 - 180.0;
  return lat;
}

qreal latitudeFromTile(qreal ty, int zoom)
{
  qreal zn = static_cast<qreal>(1 << zoom);
  qreal n = M_PI - 2 * M_PI * ty / zn;
  qreal lng = 180.0 / M_PI * atan(0.5 * (exp(n) - exp(-n)));
  return lng;
}
} // namespace
