
#include "locator.h"

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

#include <QDebug>
#include <QtLocation/QGeoCoordinate>
#include "deliberate.h"
#include <math.h>

using namespace QtMobility;
using namespace deliberate;

static const int LocalMoveStep (334);

namespace loco
{
Locator::Locator (QObject *parent)
  :QGeoPositionInfoSource (parent),
   timer (0),
   moveStep (LocalMoveStep)
{
  timer = new QTimer (this);
  connect (timer, SIGNAL (timeout()), this, SLOT (getPosition()));
  InitCircuit ();
}

QGeoPositionInfo
Locator::lastKnownPosition (bool fromSatOnly) const
{
  return QGeoPositionInfo();
}

Locator::PositioningMethods
Locator::supportedPositioningMethods () const
{
  return AllPositioningMethods;
}

int
Locator::minimumUpdateInterval () const
{
  return 50;
}

void
Locator::startUpdates ()
{
  int delay = updateInterval();
  if (delay < minimumUpdateInterval()) {
    delay = minimumUpdateInterval();
  }
  timer->start (delay);
  getPosition ();
qDebug () << " Starting updates every " << delay;
qDebug () << " startUpdates lastPosition " << lastPosition.toString();
  emit NewDestination (destPosition, destSpot->name);
}

void
Locator::stopUpdates ()
{
  timer->stop();
}

void
Locator::requestUpdate (int timeout)
{
  QTimer::singleShot (100,this, SLOT (getPosition()));
}

void
Locator::InitCircuit ()
{
  //AddSpot (43.0 + 4.777/60.0,- (79.0 + 21.044/60.0) );
  circuit.clear ();
  QString filename ("./locations");
  filename = Settings().value ("locations",filename).toString();
  Settings().setValue ("locations",filename); 
  QFile circuitFile (filename);
  bool ok = circuitFile.open (QFile::ReadOnly);
  if (ok) {
    QTextStream input (&circuitFile);
    double lat, lon;
    QString name;
    while (ok && !input.atEnd()) {
      input >> lat;
      input >> lon;
      name = input.readLine (2048);
      AddSpot (lat, lon, name.trimmed());
    }
  } else {
    AddSpot (39.760414,-104.998169, "Denver"); // Denver
    AddSpot (35.185482,-114.06189, "Kingman, AZ"); // kingman, AZ
    AddSpot (32.798141, -117.240801, "San Diego, CA"); // pacifig beach
    AddSpot (37.556173, -121.992989, "Fremont, CA"); // fremont
    AddSpot (29.426202,-98.463135, "San Antonio, TX"); // San Antonio, TX
    AddSpot (30.042469,-90.095216, "New Orleans, LA");
    AddSpot (30.270454,-81.569825, "Jacksonville, FL");
    AddSpot (41.888646,-87.612412, "Chicago, IL"); // chicago
    AddSpot (36.214142,-86.817627, "Nashville, TN");
    AddSpot (39.953543,-83.063966, "Columbus, OH");
    AddSpot (43.025934,-83.079529, "Imlay City, MI");
    AddSpot (43.081001,-79.071407, "Niagara Falls"); // niagara falls
    AddSpot (40.689828,-74.045162, "NYC"); // lady liberty
    AddSpot (38.673503,-90.223389, "St. Louis, MO");
  }
  circuitFile.close ();
#if 0
  AddSpot (76.531192,-68.710756, "Thule");  // Thule
  AddSpot (64.126967,-21.861649, "Reykjavik"); // Reykjavik
  AddSpot (57.155908,-2.10022, "Aberdeen");  // Aberdeen
  AddSpot (51.6748, -0.0417,"London, England"); // london
  AddSpot (36.14378,-5.355206, "Gibraltar"); // Gibraltar
  AddSpot (52.356816,4.870605, "Amsterdam"); //  Amsterdam
  AddSpot (54.350474,18.619995, "Gdansk"); //  Gdansk
  AddSpot (40.997728,28.981018, "Istanbul"); //  Istanbul
  AddSpot (32.060697,34.787292, "Tel Aviv");  // Tel Aviv
  AddSpot (21.435685,39.845581, "Mecca");  // Mecca
  AddSpot (29.655597,91.182861, "Lhasa, Tibet"); // Lhasa, Tibet
  AddSpot (1.321538,103.829956, "Singapore"); // Singapore
  AddSpot (43.140226,131.917534, "Vladivostok");  // Vladivostok
  AddSpot (66.158471,-169.804745, "Siberia");  // spot in NE Siberia
  AddSpot (61.282905,-149.868166, "Anchorage, AK");  // anchorage, AK
#endif
  QString destName;
  FirstSpot (lastPosition, destPosition, destName);
  moveStep = LocalMoveStep;
qDebug () << "InitCircuit lastPosition " << lastPosition.toString();
}

void
Locator::FirstSpot (QGeoCoordinate & origin, 
                  QGeoCoordinate & destination,
                  QString & destName)
{
  currentSpot = circuit.begin();
  destSpot = currentSpot;
  destSpot++;
  origin = currentSpot->coordinate;
  destination = destSpot->coordinate;
  destName = destSpot->name;
}

void
Locator::AddSpot (double lat, double lon, const QString & name)
{
  GeoSpot loc;
  loc.coordinate.setLatitude (lat);
  loc.coordinate.setLongitude (lon);
  loc.name = name;
  circuit.append (loc);
}

void
Locator::getPosition ()
{
qDebug () << " Locator getPosition ";
qDebug () << "        lastPosition " << lastPosition.toString();
qDebug () << "        destPosition " << destPosition.toString();
  double dist = lastPosition.distanceTo (destPosition);
  double bearing = lastPosition.azimuthTo (destPosition);
qDebug () << "        dist " << dist;
qDebug () << "        bearing " << bearing;
  if (dist <= moveStep) {
    QString destName;
    NextSpot (lastPosition, destPosition, destName);
    emit NewDestination (destPosition, destName);
  } else {
    double newLat, newLon;
    NewPos (newLat, newLon, 
            lastPosition.latitude(),
            lastPosition.longitude(),
            moveStep, bearing);
    lastPosition.setLatitude (newLat);
    lastPosition.setLongitude (newLon);
  }
  QDateTime now = QDateTime::currentDateTime();
  QGeoPositionInfo here;
  here.setCoordinate (lastPosition);
  here.setTimestamp (now.toUTC());
qDebug () << "reporting position " << here.coordinate().toString();
  emit positionUpdated (here);
}

void
Locator::NextSpot (QGeoCoordinate & origin,
		   QGeoCoordinate & destination,
                   QString & destName)
{
  currentSpot++;
  if (currentSpot == circuit.end()) {
    currentSpot = circuit.begin();
  }
  destSpot = currentSpot;
  destSpot++;
  if (destSpot == circuit.end()) {
    destSpot = circuit.begin();
  }
  origin = currentSpot->coordinate;
  destination = destSpot->coordinate;
  destName = destSpot->name;
}

void
Locator::NewPos (double & lat2, double & lon2,
                                double lat1, double lon1,
                                double dist, double bearingDeg)
{
  const double Rearth (6371000);
  double bearing (bearingDeg * (M_PI/180.0));
  lat1 = lat1*(M_PI/180.0);
  lon1 = lon1*(M_PI/180.0);
  lat2 = asin( sin(lat1)*cos(dist/Rearth) + 
                      cos(lat1)*sin(dist/Rearth)*cos(bearing) );
  lon2 = lon1 + atan2(sin(bearing)*sin(dist/Rearth)*cos(lat1), 
                             cos(dist/Rearth)-sin(lat1)*sin(lat2));
  lat2 *= (180.0/M_PI);
  lon2 *= (180.0/M_PI);
qDebug () << " New Pos lat2 " << lat2 << " lon2 " << lon2;
}

} // namespace

