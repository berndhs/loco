#ifndef LOCATOR_H
#define LOCATOR_H

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
#include <QtLocation/QGeoPositionInfoSource>
#include <QtLocation/QGeoPositionInfo>
#include <QObject>
#include <QTimer>
#include <QFile>

using namespace QtMobility;

namespace loco
{
class Locator : public QGeoPositionInfoSource
{
  Q_OBJECT
public:
  Locator (QObject *parent = 0);
  Locator (const QString & tourfile, QObject *parent = 0);

  QGeoPositionInfo lastKnownPosition(bool fromSatellitePositioningMethodsOnly = false) const;

  PositioningMethods supportedPositioningMethods() const;
  int minimumUpdateInterval() const;

public slots:
  virtual void startUpdates();
  virtual void stopUpdates();

  virtual void requestUpdate(int timeout = 5000);

private slots:
  void getPosition ();

signals:

  void NewDestination (const QGeoCoordinate & whereTo,
                       const QString & name);

private:

  void InitCircuit (const QString & tourname = QString ());
  void AddSpot (double lat, double lon, const QString & name);
  void NextSpot (QGeoCoordinate & origin, 
                  QGeoCoordinate & destination,
                  QString & destName);
  void FirstSpot (QGeoCoordinate & origin, 
                  QGeoCoordinate & destination,
                  QString & destName);
  void NewPos (double & lat2, double & lon2,
                                double lat1, double lon1,
                                double dist, double bearingDeg);

  class GeoSpot 
  {
    public:
    QGeoCoordinate    coordinate;
    QString           name;
  };
  typedef  QList <GeoSpot>  GeoList;

  QTimer           *timer;
  QGeoCoordinate    lastPosition;
  QGeoCoordinate    destPosition;
  double            moveStep;
  GeoList           circuit;
  GeoList::iterator currentSpot;
  GeoList::iterator destSpot;

  static int LocalMoveStep;
};
} // namespace

#endif