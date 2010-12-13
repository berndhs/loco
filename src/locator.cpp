
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


namespace loco
{

int Locator::LocalMoveStep (334);

Locator::Locator (QObject *parent)
  :QGeoPositionInfoSource (parent),
   timer (0),
   moveStep (LocalMoveStep)
{
  LocalMoveStep = Settings().value ("steps/localmove",
                  LocalMoveStep).toInt();
  Settings().setValue ("steps/localmove",LocalMoveStep);
  moveStep = LocalMoveStep;
  timer = new QTimer (this);
  connect (timer, SIGNAL (timeout()), this, SLOT (getPosition()));
  InitCircuit ();
}

Locator::Locator (const QString & tourfile, QObject *parent)
  :QGeoPositionInfoSource (parent),
   timer (0),
   moveStep (LocalMoveStep)
{
  LocalMoveStep = Settings().value ("steps/localmove",
                  LocalMoveStep).toInt();
  Settings().setValue ("steps/localmove",LocalMoveStep);
  moveStep = LocalMoveStep;
  timer = new QTimer (this);
  connect (timer, SIGNAL (timeout()), this, SLOT (getPosition()));
  InitCircuit (tourfile);
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
Locator::InitCircuit (const QString & tourfile)
{
  //AddSpot (43.0 + 4.777/60.0,- (79.0 + 21.044/60.0) );
  circuit.clear ();
  QString filename (tourfile);
  if (filename.length() == 0) {
    filename = QString ("./locations");
    filename = Settings().value ("locations",filename).toString();
    Settings().setValue ("locations",filename); 
  }
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
    AddSpot (43.081001,-79.071407, "Niagara Falls"); // niagara falls
    AddSpot (40.689828,-74.045162, "NYC"); // lady liberty
  }
  circuitFile.close ();
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

