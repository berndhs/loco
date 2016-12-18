
#include "locator.h"

/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (C) 2016, Bernd Stramm
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
#include <QGeoCoordinate>
//#include <QtLocation/QGeoCoordinate>
#include "deliberate.h"
#include <math.h>
#include "loco-global.h"

//using namespace QtMobility;
using namespace deliberate;


namespace loco
{

int Locator::LocalMoveStep (334);

Locator::Locator (QObject *parent)
  :QGeoPositionInfoSource (parent),
   timer (0),
   moveStep (LocalMoveStep)
{
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
  moveStep = LocalMoveStep;
  timer = new QTimer (this);
  connect (timer, SIGNAL (timeout()), this, SLOT (getPosition()));
  InitCircuit (tourfile);
}

int
Locator::Interval ()
{
  return timer->interval();
}

QGeoPositionInfoSource::Error Locator::error() const
{
  qDebug() << Q_FUNC_INFO ;
  return QGeoPositionInfoSource::NoError;
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

int
Locator::MoveStep ()
{
  return moveStep;
}

void
Locator::SetMoveStep (int newStep)
{
  moveStep = newStep;
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
  lastPosition = loco::lastPlace;
qDebug () << Q_FUNC_INFO << lastPosition.toString();
  emit NewDestination (destPosition, destSpot->name);
}

void
Locator::stopUpdates ()
{
  qDebug() << Q_FUNC_INFO << lastPosition;
  emit iAmHere(lastPosition);
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
  loco::lastPlace = origin;
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

