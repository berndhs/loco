
#include "slip-cache.h"

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

#include <QDir>
#include <QFileInfo>
#include <QByteArray>
#include <QBuffer>
#include <QDebug>

#include "deliberate.h"

namespace loco
{

SlipCache * SlipCache::theOnlyOne (0);

SlipCache *
SlipCache::Ptr ()
{
  return theOnlyOne;
}

void
SlipCache::Allocate (const QString & path, QObject *parent)
{
  if (theOnlyOne) {
    return;
  }
  theOnlyOne = new SlipCache (path, parent);
  theOnlyOne->SetupTimers ();
}

void
SlipCache::SetupTimers ()
{
  writeTimer = new QTimer (this);
  connect (writeTimer, SIGNAL (timeout()),
           this, SLOT (DoWrites()));
  writeTimer->start (250);
  readTimer = new QTimer (this);
  connect (readTimer, SIGNAL (timeout()),
           this, SLOT (DoReads ()));
  readTimer->start (100);
  sendTimer = new QTimer (this);
  connect (sendTimer, SIGNAL (timeout()),
           this, SLOT (DoSends()));
  sendTimer->start (50);
  cleanTimer = new QTimer (this);
  connect (cleanTimer, SIGNAL (timeout()),
           this, SLOT (DoClean ()));
  cleanTimer->start (2*60*1000);
}

SlipCache::SlipCache (const QString & path, QObject *parent)
  :QObject (parent),
   readTimer (0),
   writeTimer (0),
   sendTimer (0),
   cleanTimer (0),
   pathname (path)
{
  pathname.append (QDir::separator());
  QDir pathdir (path);
  if (!pathdir.exists()) {
    pathdir.mkpath (path);
  }
}

bool
SlipCache::GetTile (QPoint pt, QString path)
{
  if (ramCache.contains (path)) {
    ramCache[path].unused = false;
    QueueSend (ramCache[path]);
  } else {
    QString baseName (QString ("%1%2").arg (pathname).arg(path));
    QFileInfo  info (QString ("%1.img")
                      .arg (baseName));
    if (info.exists()) {
      QueueRead (baseName);
    } else {
      return false;
    }
  }
  return true;
}

void
SlipCache::Flush ()
{
  ramCache.clear ();
}

void
SlipCache::QueueSend (const TileRecord & rec)
{
  sendQ << rec;
}

void
SlipCache::QueueRead (const QFileInfo & info)
{
  readQ << info.filePath();
}

void
SlipCache::QueueWrite (const TileRecord & rec)
{
  writeQ << rec;
}

void
SlipCache::DoReads ()
{
  TileRecord rec;
  bool doEmit (false);
  while (!readQ.isEmpty()) {
    QString name = readQ.takeFirst();  
    QString descName (name);
    descName.append (".dsc");
    QString imgName (name);
    imgName.append (".img");
    QFile descFile (descName);
    bool ok = descFile.open (QFile::ReadOnly);
    if (ok) {
      QByteArray bytes;
      bytes = descFile.readLine (2048);
      rec.name = QString (bytes);
      bytes = descFile.readLine (256);
      if (bytes.endsWith('\n')) {
        bytes.chop(1);
      }
      int x = bytes.toInt();
      bytes = descFile.readLine (256);
      if (bytes.endsWith('\n')) {
        bytes.chop(1);
      }
      int y = bytes.toInt ();
      rec.point = QPoint (x,y);
      QFile imgFile (imgName);
      ok = imgFile.open (QFile::ReadOnly);
      if (ok) {
        bytes = imgFile.readAll ();
        rec.img.loadFromData (bytes);
        ramCache [rec.name] = rec;
        doEmit = true;
      }
      imgFile.close ();
    }
    descFile.close ();
  }
  if (doEmit) {
    emit HaveTile (rec.point, rec.img);
  }
}

void
SlipCache::DoClean ()
{
  RamCacheType::iterator chase;
  RamCacheType::iterator trail;
  bool                   removeit (false);
  chase = ramCache.begin();
  while (chase != ramCache.end()) {
    if (chase->unused) {
      trail = chase;
      removeit = true;
    } else {
      chase->unused = true;
    }
    chase++;
    if (removeit) {
      ramCache.erase (trail);
      removeit = false;
    }
  }
}

void
SlipCache::DoSends ()
{
  while (!sendQ.isEmpty()) {
    TileRecord rec = sendQ.takeFirst();
    emit HaveTile (rec.point, rec.img);
  }
}

void
SlipCache::DoWrites ()
{
  int i;
  for (i=0; i<100; i++) {
    if (writeQ.isEmpty()) {
      return;
    }
    TileRecord rec = writeQ.takeFirst();
    Write (rec);
  }
}

void
SlipCache::Write (const TileRecord & rec)
{
  QString name (pathname);
  name.append (rec.name);

  QString descName (name);
  descName.append (".dsc");
  QString imgName (name);
  imgName.append (".img");
  QFileInfo info (descName);
  if (!info.exists()) {
    QDir dir (info.path());
    if (!dir.exists()) {
      dir.mkpath (info.path());
    }
  }
  QFile descFile (descName);
  bool ok = descFile.open (QFile::WriteOnly);
  if (ok) {
    descFile.write (rec.name.toLatin1());
    QByteArray eol ("\n");
    descFile.write (eol);
    QByteArray buf;
    buf.setNum (rec.point.x());
    buf.append (eol);
    descFile.write (buf);
    buf.setNum (rec.point.y());
    buf.append (eol);
    descFile.write (buf);
    QFile imgFile (imgName);
    ok = imgFile.open (QFile::WriteOnly);
    if (ok) {
      buf.clear ();
      QBuffer imgBuf (&buf);
      imgBuf.open (QBuffer::WriteOnly);
      rec.img.save (&imgBuf, "PNG");
      imgBuf.close();
      imgFile.write (buf);
    }  
    imgFile.close ();
  }
  descFile.close ();
}

void
SlipCache::Save (const QString & name,
                 const QPoint & pt,
                 const QImage & img)
{
  qDebug () << "save tile " << name;
  TileRecord rec;
  rec.name = name;
  rec.point = pt;
  rec.img = img;
  rec.unused = false;
  ramCache[name] = rec;
  QueueWrite (rec);
}

} // namespace
