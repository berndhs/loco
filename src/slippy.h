#ifndef SLIPPY_MAP_H
#define SLIPPY_MAP_H

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

#include <math.h>
#include "loco-global.h"

#include <QtCore>
#include <QtGui>
#include <QtNetwork>

#include <QGeoCoordinate>
#include <QGeoPositionInfoSource>
#include <QGeoPositionInfo>

//#include <QtLocation/QGeoPositionInfoSource>
//#include <QtLocation/QGeoSatelliteInfoSource>
//#include <QtLocation/QNmeaPositionInfoSource>
//#include <QtLocation/QGeoPositionInfo>
#include <QtNetwork/QNetworkConfigurationManager>
#include <QtNetwork/QNetworkSession>
//#include <QDesktopServices>
#include <QStandardPaths>
#include <QPainter>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#warning Do not have M_PI
#endif


//using namespace QtMobility;

namespace loco
{

class SlipCache;

class SlippyMap: public QObject
{
  Q_OBJECT

public:
  int width;
  int height;
  int zoom;
  qreal latitude;
  qreal longitude;

  SlippyMap(QNetworkSession *session, QGeoPositionInfoSource *location, QObject *parent = 0);

  ~SlippyMap() ;

  void ConnectCache ();

  void invalidate();

  void render(QPainter *p, const QRect &rect) ;

  void pan(const QPoint &delta) ;

  qlonglong  CacheHits () { return cacheHits; }
  qlonglong  CacheMisses () { return cacheMisses; }

public slots:

  void positionUpdated(const QGeoPositionInfo &gpsPos) ;

private slots:

  void handleNetworkData(QNetworkReply *reply);
  void handleCacheData (QPoint tp, const QImage & img);

  void download() ;

signals:
  void updated(const QRect &rect);

protected:
  QRect tileRect(const QPoint &tp);

private:
  QPoint m_offset;
  QRect m_tilesRect;
  QPixmap m_emptyTile;
  QHash<QPoint, QPixmap> m_tilePixmaps;
  QNetworkAccessManager *m_manager;
  QUrl m_url;

  QGeoPositionInfoSource* m_location;
  QNetworkSession* m_session;
  SlipCache      *sCache;
  QList<QNetworkReply*> m_pendingReplies;

  static int countMaps;

  qlonglong   cacheHits;
  qlonglong   cacheMisses;

};

} // namespace

#endif
