#ifndef SLIP_CACHE_H
#define SLIP_CACHE_H

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

#include <QObject>
#include <QPoint>
#include <QImage>
#include <QString>
#include <QMap>
#include <QTimer>
#include <QFileInfo>

namespace loco
{
class SlipCache : public QObject
{
Q_OBJECT

public:

  static void Allocate (const QString & path, QObject *parent=0);

  static SlipCache * Ptr();

  QString Path () { return pathname; }
  void    Flush ();
  void    Save (const QString & name,
                const QPoint  & pt,
                const QImage  & img);

  bool GetTile (QPoint pt, QString path);

private slots:
 
  void DoWrites ();
  void DoSends ();
  void DoReads ();
  void DoClean ();

signals:
 
  void HaveTile (QPoint pt, const QImage & img);

private:

  class TileRecord {
    public:
      QString   name;
      QPoint    point;
      QImage    img;
      bool      unused;
  };

  typedef QMap <QString, TileRecord>  RamCacheType;

  SlipCache (const QString & path, QObject *parent = 0);

  void SetupTimers ();
  
  void QueueSend (const TileRecord & rec);
  void QueueRead (const QFileInfo & info);
  void QueueWrite (const TileRecord & rec);

  void Write (const TileRecord & rec);


  QTimer  *readTimer;
  QTimer  *writeTimer;
  QTimer  *sendTimer;
  QTimer  *cleanTimer;

  RamCacheType   ramCache;
 
  QList <TileRecord>  sendQ;
  QList <TileRecord>  writeQ;
  QList <QString>     readQ;

  QString        pathname;

  static SlipCache * theOnlyOne;

};

} // namespace

#endif