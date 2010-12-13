
#include "slippy.h"


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

#include "slip-cache.h"

namespace loco
{

int SlippyMap::countMaps (1);

SlippyMap::SlippyMap(QNetworkSession *session,
                     QGeoPositionInfoSource *location,
                     QObject *parent)
  :
  QObject(parent),
  width(400),
  height(300),
  zoom(12),
  latitude(59.9138204),
  longitude(10.7387413),
  m_location(location),
  m_session(session),
  sCache (0)
{
  setObjectName (QString ("SlippyMap-%1").arg(countMaps++));
  m_emptyTile = QPixmap(T_DIM, T_DIM);
  m_emptyTile.fill(Qt::lightGray);

  m_manager = new QNetworkAccessManager(this);

  QNetworkDiskCache *cache = new QNetworkDiskCache;
  cache->setCacheDirectory(QDesktopServices::storageLocation(QDesktopServices::CacheLocation));
  m_manager->setCache(cache);
  connect(m_manager, SIGNAL(finished(QNetworkReply*)),
          this, SLOT(handleNetworkData(QNetworkReply*)));

  // Listen gps position changes
  connect(m_location, SIGNAL(positionUpdated(QGeoPositionInfo)),
          this, SLOT(positionUpdated(QGeoPositionInfo)));
  
  // see if there is a cache, and if yes, connect to it
  sCache = SlipCache::Ptr ();
  if (sCache) {
    connect (sCache, SIGNAL (HaveTile (QPoint, const QImage&)),
             this, SLOT (handleCacheData (QPoint, const QImage&)));
  }
}

SlippyMap::~SlippyMap()
{
  for (int i = 0; i < m_pendingReplies.size(); ++i) {
    delete m_pendingReplies.at(i);
  }
}

void
SlippyMap::ConnectCache ()
{
  sCache = SlipCache::Ptr ();
  if (sCache) {
    connect (sCache, SIGNAL (HaveTile (QPoint, const QImage&)),
             this, SLOT (handleCacheData (QPoint, const QImage&)));
  }
}

void
SlippyMap::invalidate()
{
  if (width <= 0 || height <= 0)
    return;

  QPointF ct = tileForCoordinate(latitude, longitude, zoom);
  qreal tx = ct.x();
  qreal ty = ct.y();

  // top-left corner of the center tile
  int xp = width / 2 - (tx - floor(tx)) * T_DIM;
  int yp = height / 2 - (ty - floor(ty)) * T_DIM;

  // first tile vertical and horizontal
  int xa = (xp + T_DIM - 1) / T_DIM;
  int ya = (yp + T_DIM - 1) / T_DIM;
  int xs = static_cast<int>(tx) - xa;
  int ys = static_cast<int>(ty) - ya;

  // offset for top-left tile
  m_offset = QPoint(xp - xa * T_DIM, yp - ya * T_DIM);

  // last tile vertical and horizontal
  int xe = static_cast<int>(tx) + (width - xp - 1) / T_DIM;
  int ye = static_cast<int>(ty) + (height - yp - 1) / T_DIM;

  // build a rect
  m_tilesRect = QRect(xs, ys, xe - xs + 1, ye - ys + 1);

  if (m_url.isEmpty())
    download();

  emit updated(QRect(0, 0, width, height));
}

void
SlippyMap::render (QPainter *p, const QRect &rect)
{
  for (int x = 0; x <= m_tilesRect.width(); ++x)
    for (int y = 0; y <= m_tilesRect.height(); ++y) {
      QPoint tp(x + m_tilesRect.left(), y + m_tilesRect.top());
      QRect box = tileRect(tp);
      if (rect.intersects(box)) {
        if (m_tilePixmaps.contains(tp))
          p->drawPixmap(box, m_tilePixmaps.value(tp));
        else
          p->drawPixmap(box, m_emptyTile);
      }
    }
}
void
SlippyMap::pan(const QPoint &delta)
{
  QPointF dx = QPointF(delta) / qreal(T_DIM);
  QPointF center = tileForCoordinate(latitude, longitude, zoom) - dx;
  latitude = latitudeFromTile(center.y(), zoom);
  longitude = longitudeFromTile(center.x(), zoom);
  invalidate();
}

void
SlippyMap::positionUpdated(const QGeoPositionInfo &gpsPos)
{
qDebug () << " Slippy recieve pos update ";
  latitude = gpsPos.coordinate().latitude();
  longitude = gpsPos.coordinate().longitude();
qDebug () << "    lat " << latitude << "   lon " << longitude;
  invalidate();
}

void
SlippyMap::handleNetworkData(QNetworkReply *reply)
{
qDebug () << "SlippyMap get network data " << reply;
qDebug () << "    from " << reply->url();
  QImage img;
  QPoint tp = reply->request().attribute(QNetworkRequest::User).toPoint();
qDebug () << "    reply for point " 
         << reply->request().attribute(QNetworkRequest::User);
  QUrl url = reply->url();
  if (!reply->error()) {
    if (!img.load(reply, 0)) {
      img = QImage();
    }
  }

  for (int i = 0; i < m_pendingReplies.size(); ++i) {
    if (m_pendingReplies.at(i) == reply) {
      m_pendingReplies.removeAt(i);
      break;
    }
  }

  reply->deleteLater();
  m_tilePixmaps[tp] = QPixmap::fromImage(img);
  if (img.isNull()) {
    m_tilePixmaps[tp] = m_emptyTile;
  }
  emit updated(tileRect(tp));
  if (sCache) {
qDebug () << "SlippyMap " << objectName() << " save in cache " << tp;
    sCache->Save (url.path(), tp, img);
  }

  // purge unused spaces
  QRect bound = m_tilesRect.adjusted(-2, -2, 2, 2);
  foreach(QPoint tp, m_tilePixmaps.keys()) {
    if (!bound.contains(tp)) {
      m_tilePixmaps.remove(tp);
    } 
  }

  download();
}

void
SlippyMap::handleCacheData (QPoint tp, const QImage &img)
{
qDebug () << " SlippyMap "
          << objectName() << " get cache point " << tp << " img Null " << img.isNull();
  if (img.isNull()) {
    m_tilePixmaps[tp] = m_emptyTile;
  } else {
    m_tilePixmaps[tp] = QPixmap::fromImage (img);
QFile tmpfile (QString ("/tmp/tile-%1-%2.png")
                       .arg (tp.x())
                       .arg (tp.y()));
  }
  emit updated (tileRect (tp));
  // purge unused spaces
  QRect bound = m_tilesRect.adjusted(-2, -2, 2, 2);
  foreach(QPoint tp, m_tilePixmaps.keys()) {
    if (!bound.contains(tp)) {
      m_tilePixmaps.remove(tp);
    } 
  }
  QTimer::singleShot (100, this, SLOT (download()));

}

void
SlippyMap::download()
{
qDebug () << "SlippyMap download";
  QPoint grab(0, 0);
  for (int x = 0; x <= m_tilesRect.width(); ++x)
    for (int y = 0; y <= m_tilesRect.height(); ++y) {
      QPoint tp = m_tilesRect.topLeft() + QPoint(x, y);
      if (!m_tilePixmaps.contains(tp)) {
        grab = tp;
        break;
      }
    }
  if (grab == QPoint(0, 0)) {
    m_url = QUrl();
qDebug () << " SlippyMap donwload grab is zero";
    return;
  }

  QString path = "http://tile.openstreetmap.org/%1/%2/%3.png";
  m_url = QUrl(path.arg(zoom).arg(grab.x()).arg(grab.y()));
  if (sCache) {
    if (sCache->GetTile (grab,m_url.path())) {
qDebug () << " GetTile says yes for " << grab;
      return;
    }
  }
  QNetworkRequest request;
  request.setUrl(m_url);
  request.setRawHeader("User-Agent", "Nokia (Qt) Graphics Dojo 1.0");
  request.setAttribute(QNetworkRequest::User, QVariant(grab));
  m_pendingReplies << m_manager->get(request);
}

QRect
SlippyMap::tileRect(const QPoint &tp)
{
  QPoint t = tp - m_tilesRect.topLeft();
  int x = t.x() * T_DIM + m_offset.x();
  int y = t.y() * T_DIM + m_offset.y();
  return QRect(x, y, T_DIM, T_DIM);
}



} // namespace
