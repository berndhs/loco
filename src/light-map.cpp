#include "light-map.h"

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


#include <QDebug>
#include <QMessageBox>

namespace loco
{
LightMap::LightMap(QWidget *parent)
  :QWidget(parent),
   m_normalMap(0),
   m_largeMap(0),
   firstLat(0.0),
   firstLong(0.0),
   pressed(false),
   snapped(false),
   zoomed(false),
   invert(false),
   m_usingLogFile(false),
   m_location(0),
   waitingForFix(false),
   updateDelay (10000)
{

  // Set Internet Access Point
  QNetworkConfigurationManager manager;
  const bool canStartIAP = (manager.capabilities()
                            & QNetworkConfigurationManager::CanStartAndStopInterfaces);

  // Is there default access point, use it
  QNetworkConfiguration cfg1 = manager.defaultConfiguration();
  if (!cfg1.isValid() || (!canStartIAP && cfg1.state() != 
                           QNetworkConfiguration::Active)) {
    m_networkSetupError = QString(tr("This example requires networking, and no avaliable networks or access points could be found."));
    QTimer::singleShot(0, this, SLOT(networkSetupError()));
    return;
  }

  m_session = new QNetworkSession(cfg1, this);
  m_connectivityHelper = new ConnectivityHelper(m_session, this);
  connect(m_session, SIGNAL(opened()), this, SLOT(networkSessionOpened()));
  connect(m_connectivityHelper, SIGNAL(networkingCancelled()), qApp, SLOT(quit()));

  m_session->open();
}

LightMap::~LightMap()
{
  m_session->close();
  if (m_location)
    m_location->stopUpdates();
}

qlonglong
LightMap::CacheHits ()
{
  qlonglong count (0);
  if (m_normalMap) {
    count += m_normalMap->CacheHits ();
  }
  if (m_largeMap) {
    count += m_largeMap->CacheHits ();
  }
  return count;
}

qlonglong
LightMap::CacheMisses ()
{
  qlonglong count (0);
  if (m_normalMap) {
    count += m_normalMap->CacheMisses ();
  }
  if (m_largeMap) {
    count += m_largeMap->CacheMisses ();
  }
  return count;
}

void
LightMap::SetCourseAngle (qreal angleDegrees)
{
  courseAngle = angleDegrees;
}

inline void
LightMap::stopPositioning()
{
  if (m_location)
    m_location->stopUpdates();
  running = false;
}

inline bool
LightMap::isRunning()
{
  return running;
}

void
LightMap::startPositioning()
{
qDebug () << Q_FUNC_INFO << m_location;
  if (m_location)
    m_location->startUpdates();
  running = true;
}

void
LightMap::setCenter(qreal lat, qreal lng)
{
  qDebug() << Q_FUNC_INFO << lat << lng;
  if (!m_normalMap || !m_largeMap) {
    firstLat = lat;
    firstLong = lng;
    return;
  }
  m_normalMap->latitude = lat;
  m_normalMap->longitude = lng;
  m_normalMap->invalidate();
  m_largeMap->latitude = lat;
  m_largeMap->longitude = lng;
  m_largeMap->invalidate();
}

void
LightMap::ConnectCache ()
{
  if (m_normalMap) {
    m_normalMap->ConnectCache ();
  }
  if (m_largeMap) {
    m_largeMap->ConnectCache ();
  }
}

void
LightMap::toggleNightMode()
{
  invert = !invert;
  update();
}

void
LightMap::networkSetupError()
{
  QMessageBox::critical(this, tr("LightMap"),
                        m_networkSetupError);
  QTimer::singleShot(0, qApp, SLOT(quit()));
}

void
LightMap::SetLocator (QGeoPositionInfoSource * locSrc)
{
  m_location = locSrc;

  m_location->setUpdateInterval(updateDelay);

  connect(m_location,
          SIGNAL(positionUpdated(QGeoPositionInfo)),
          this,
          SLOT(positionUpdated(QGeoPositionInfo)));

}

void
LightMap::networkSessionOpened()
{
qDebug () << " LightMap networkSessionOpened " ;
  m_location = QGeoPositionInfoSource::createDefaultSource(this);
qDebug () << "          m_location " << m_location;
  if (!m_location) {
    QNmeaPositionInfoSource *nmeaLocation = 
              new QNmeaPositionInfoSource
                 (QNmeaPositionInfoSource::SimulationMode, this);
    QFile *logFile = new QFile(QCoreApplication::applicationDirPath()
                               + QDir::separator() + "nmealog.txt", this);
    nmeaLocation->setDevice(logFile);
    m_location = nmeaLocation;
    m_usingLogFile = true;
  }

  m_location->setUpdateInterval(updateDelay);

  connect(m_location,
          SIGNAL(positionUpdated(QGeoPositionInfo)),
          this,
          SLOT(positionUpdated(QGeoPositionInfo)));

  if (m_usingLogFile) {
    QMessageBox::information(this, tr("LightMap"),
                             tr("No GPS support detected, using GPS data from a sample log file instead."));
  } else {

    m_location->stopUpdates();
  }

  m_normalMap = new SlippyMap(m_session, m_location, this);
  m_largeMap = new SlippyMap(m_session, m_location, this);

  connect(m_normalMap, SIGNAL(updated(QRect)), SLOT(updateMap(QRect)));
  connect(m_largeMap, SIGNAL(updated(QRect)), SLOT(update()));

  setCenter(firstLat, firstLong);

  m_normalMap->width = width();
  m_normalMap->height = height();
  m_largeMap->width = m_normalMap->width * 2;
  m_largeMap->height = m_normalMap->height * 2;

  connect(m_location, SIGNAL(updateTimeout()), this, SLOT(waitForFix()));

  startPositioning();
qDebug () << "          final m_location " << m_location;
}

void 
LightMap::positionUpdated(const QGeoPositionInfo &pos) 
{
qDebug () << Q_FUNC_INFO << pos.coordinate().toString();
  setCenter(pos.coordinate().latitude(), pos.coordinate().longitude());
}

void
LightMap::activateZoom()
{
  stopPositioning();
  zoomed = true;
  tapTimer.stop();
  m_largeMap->zoom = m_normalMap->zoom + 1;
  m_largeMap->width = m_normalMap->width * 2;
  m_largeMap->height = m_normalMap->height * 2;
  m_largeMap->latitude = m_normalMap->latitude;
  m_largeMap->longitude = m_normalMap->longitude;
  m_largeMap->invalidate();
  update();
}

void
LightMap::resizeEvent(QResizeEvent *)
{
  if (!m_normalMap || !m_largeMap)
    return;

  m_normalMap->width = width();
  m_normalMap->height = height();
  m_normalMap->invalidate();
  m_largeMap->width = m_normalMap->width * 2;
  m_largeMap->height = m_normalMap->height * 2;
  m_largeMap->invalidate();
}

void
LightMap::paintEvent(QPaintEvent *event)
{
qDebug () << Q_FUNC_INFO << event;
  if (!m_normalMap || !m_largeMap)
    return;

  QPainter p;
  p.begin(this);
  m_normalMap->render(&p, event->rect());
  p.setPen(Qt::black);
  p.drawText(rect(), Qt::AlignBottom | Qt::TextWordWrap,
             "Map data CCBYSA 2016 OpenStreetMap.org contributors");
  QPoint midpoint (rect().center());
  QPen pen (Qt::DashLine);
  pen.setColor (Qt::darkBlue);
  p.setPen (pen);
  p.drawEllipse (midpoint, 70,70);
  pen.setColor (Qt::red);
  p.setPen (pen);
  p.drawEllipse (midpoint, 30,30);
  p.setPen (Qt::black);
  p.translate (midpoint);
  p.rotate (courseAngle);
  p.drawLine (0,0,0,100);
  p.end();

  
  if (invert) {
    QPainter p(this);
    p.setCompositionMode(QPainter::CompositionMode_Difference);
    p.fillRect(event->rect(), Qt::white);
    p.end();
  }
}

void
LightMap::mousePressEvent(QMouseEvent *event)
{
  if (!m_normalMap || !m_largeMap)
    return;

  if (event->buttons() != Qt::LeftButton)
    return;
  pressed = snapped = true;
  pressPos = dragPos = event->pos();
  tapTimer.stop();
  tapTimer.start(3000, this);
}

void
LightMap::mouseMoveEvent(QMouseEvent *event)
{
  if (!m_normalMap || !m_largeMap)
    return;

  if (!event->buttons())
    return;

  stopPositioning();

  if (!zoomed) {
    if (!pressed || !snapped) {
      QPoint delta = event->pos() - pressPos;
      pressPos = event->pos();
      m_normalMap->pan(delta);
      return;
    } else {
      const int threshold = 10;
      QPoint delta = event->pos() - pressPos;
      if (snapped) {
        snapped &= delta.x() < threshold;
        snapped &= delta.y() < threshold;
        snapped &= delta.x() > -threshold;
        snapped &= delta.y() > -threshold;
      }
      if (!snapped)
        tapTimer.stop();
    }
  } else {
    dragPos = event->pos();
    update();
  }
}

void
LightMap::mouseReleaseEvent(QMouseEvent *)
{
  if (!m_normalMap || !m_largeMap)
    return;

  zoomed = false;
  update();
}

void
LightMap::keyPressEvent(QKeyEvent *event)
{
  if (!m_normalMap || !m_largeMap)
    return;

  if (!zoomed) {
    if (event->key() == Qt::Key_Left)
      m_normalMap->pan(QPoint(20, 0));
    if (event->key() == Qt::Key_Right)
      m_normalMap->pan(QPoint(-20, 0));
    if (event->key() == Qt::Key_Up)
      m_normalMap->pan(QPoint(0, 20));
    if (event->key() == Qt::Key_Down)
      m_normalMap->pan(QPoint(0, -20));
    if (event->key() == Qt::Key_Z || event->key() == Qt::Key_Select) {
      dragPos = QPoint(width() / 2, height() / 2);
      activateZoom();
    }
  } else {
    if (event->key() == Qt::Key_Z || event->key() == Qt::Key_Select) {
      zoomed = false;
      update();
    }
    QPoint delta(0, 0);
    if (event->key() == Qt::Key_Left)
      delta = QPoint(-15, 0);
    if (event->key() == Qt::Key_Right)
      delta = QPoint(15, 0);
    if (event->key() == Qt::Key_Up)
      delta = QPoint(0, -15);
    if (event->key() == Qt::Key_Down)
      delta = QPoint(0, 15);
    if (delta != QPoint(0, 0)) {
      dragPos += delta;
      update();
    }
  }
}


} // namespace
