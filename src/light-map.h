#ifndef LIGHT_MAP_H
#define LIGHT_MAP_H

/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** 
** modifications:
** Copyright (C) 2010 Bernd Stramm
**
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QCoreApplication>
#include <QGeoCoordinate>
#include <QNmeaPositionInfoSource>

//#include <QtLocation/QGeoPositionInfoSource>
//#include <QtLocation/QGeoSatelliteInfoSource>
//#include <QtLocation/QNmeaPositionInfoSource>
//#include <QtLocation/QGeoPositionInfo>
#include <QtNetwork/QNetworkConfigurationManager>
#include <QtNetwork/QNetworkSession>
#include <QWidget>
#include <QtCore>
#include <QString>
#include <QPixmap>
#include <QWidget>
#include <QBasicTimer>
#include "slippy.h"
#include "connectivityhelper.h"

//using namespace QtMobility;

namespace loco
{

class LightMap: public QWidget
{
  Q_OBJECT

public:
  LightMap(QWidget *parent = 0);
  ~LightMap();

  void setUpdateDelay (int delay) { updateDelay = delay; }
  int  UpdateDelay () { return updateDelay; }
  void stopPositioning();

  void startPositioning();

  void setCenter(qreal lat, qreal lng);
  void SetLocator (QGeoPositionInfoSource * locSrc);
  void ConnectCache () ;

  qlonglong CacheHits ();
  qlonglong CacheMisses ();

  void SetCourseAngle (qreal angleDegrees);

public slots:

  void toggleNightMode();

private slots:

  void networkSetupError();

  void networkSessionOpened() ;

  // Brings up a satellite strength dialog box until a position fix is received.
  // This will also start the position updates if they are not already started.

  void positionUpdated(const QGeoPositionInfo &pos);

  void updateMap(const QRect &r) {
    update(r);
  }

protected:

  void activateZoom();
  void resizeEvent(QResizeEvent *);
  void paintEvent(QPaintEvent *event) ;
  void timerEvent(QTimerEvent *) {
    if (!zoomed)
      activateZoom();
    update();
  }

  void mousePressEvent(QMouseEvent *event) ;

  void mouseMoveEvent(QMouseEvent *event) ;
  void mouseReleaseEvent(QMouseEvent *) ;

  void keyPressEvent(QKeyEvent *event) ;

private:
  QString m_networkSetupError;
  SlippyMap *m_normalMap;
  SlippyMap *m_largeMap;
  qreal firstLat;
  qreal firstLong;
  bool pressed;
  bool snapped;
  QPoint pressPos;
  QPoint dragPos;
  QBasicTimer tapTimer;
  bool zoomed;
  QPixmap zoomPixmap;
  QPixmap maskPixmap;
  bool invert;
  bool m_usingLogFile;
  QGeoPositionInfoSource *m_location;
  bool waitingForFix;
  QNetworkSession *m_session;
  ConnectivityHelper *m_connectivityHelper;
  int   updateDelay;
  qreal courseAngle;
};

} // namespace
#endif

