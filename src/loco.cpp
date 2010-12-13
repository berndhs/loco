#include "loco.h"

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

#include <QApplication>
#include "deliberate.h"
#include "version.h"
#include "helpview.h"
#include <QSize>
#include <QDebug>
#include <QMessageBox>
#include <QTimer>
#include <QCursor>
#include <QDir>
#include <QDesktopServices>
#include <QtLocation/QGeoCoordinate>
#include "locator.h"
#include "slip-cache.h"

using namespace deliberate;
using namespace QtMobility;

static int LocalUpdateDelay (200);

namespace loco
{

Loco::Loco (const QString & tour, QWidget *parent)
  :QMainWindow (parent),
   initDone (false),
   app (0),
   configEdit (this),
   helpView (0),
   runAgain (false),
   locator (0)
{
  mainUi.setupUi (this);
  LocalUpdateDelay = 
       Settings().value ("timers/updateperiod", LocalUpdateDelay).toInt();
  Settings().setValue ("timers/updateperiod",LocalUpdateDelay);
  mainUi.displayMap->setUpdateDelay (LocalUpdateDelay);
  mainUi.actionRestart->setEnabled (false);
  helpView = new HelpView (this);
  locator = new Locator (tour, this);
  Connect ();
}

void
Loco::Init (QApplication &ap)
{
  app = &ap;
  connect (app, SIGNAL (lastWindowClosed()), this, SLOT (Exiting()));
  Settings().sync();
  initDone = true;
  QString cachePath = QDesktopServices::storageLocation 
                        (QDesktopServices::DataLocation);
  cachePath.append( QDir::separator() );
  cachePath.append ("tiles");
  cachePath.append (QDir::separator());
  SlipCache::Allocate (cachePath, this);
  mainUi.displayMap->ConnectCache ();
}

bool
Loco::Again ()
{
  bool again = runAgain;
  runAgain = false;
  return again;
}

bool
Loco::Run ()
{
  runAgain = false;
  if (!initDone) {
    Quit ();
    return false;
  }
  qDebug () << " Start Loco";
  QSize defaultSize = size();
  QSize newsize = Settings().value ("sizes/main", defaultSize).toSize();
  resize (newsize);
  Settings().setValue ("sizes/main",newsize);
  show ();
  mainUi.displayMap->startPositioning ();
  locator->setUpdateInterval (LocalUpdateDelay);
  locator->requestUpdate();
  return true;
}

void
Loco::RunSlippy ()
{
  mainUi.displayMap->SetLocator (locator);
  // Set Internet Access Point
  QNetworkConfigurationManager manager;
  const bool canStartIAP = (manager.capabilities()
                            & QNetworkConfigurationManager::CanStartAndStopInterfaces);

  // Is there default access point, use it
  QNetworkConfiguration cfg1 = manager.defaultConfiguration();
  if (!cfg1.isValid() || (!canStartIAP && cfg1.state() != QNetworkConfiguration::Active)) {
    networkSetupError = 
        QString(tr("This example requires networking, "
                   "and no avaliable networks or access points "
                    "could be found."));
    QTimer::singleShot(0, this, SLOT(networkSetupError()));
    return;
  }

  netSession = new QNetworkSession(cfg1, this);
  conHelp = new ConnectivityHelper (netSession, this);
  connect (netSession, SIGNAL (opened()),
           this, SLOT (networkSessionOpened ()));
  connect (conHelp, SIGNAL (networkingCancelled()),
           this, SLOT (Quit()));
}

void
Loco::Connect ()
{
  connect (mainUi.actionQuit, SIGNAL (triggered()),
           this, SLOT (Quit()));
  connect (mainUi.actionSettings, SIGNAL (triggered()),
           this, SLOT (EditSettings()));
  connect (mainUi.actionAbout, SIGNAL (triggered()),
           this, SLOT (About ()));
  connect (mainUi.actionLicense, SIGNAL (triggered()),
           this, SLOT (License ()));
  connect (mainUi.actionRestart, SIGNAL (triggered()),
           this, SLOT (Restart ()));
  connect (locator, SIGNAL (positionUpdated (const QGeoPositionInfo &)),
           this, SLOT (NewPosition (const QGeoPositionInfo &)));
  connect (locator, SIGNAL (NewDestination (const QGeoCoordinate &, 
                                            const QString &)),
           this, SLOT (NewDestination (const QGeoCoordinate &,
                                       const QString &)));
  connect (mainUi.startButton, SIGNAL (clicked()),
           locator, SLOT (startUpdates()));
  connect (mainUi.stopButton, SIGNAL (clicked()),
           locator, SLOT (stopUpdates()));
}

void
Loco::Restart ()
{
  qDebug () << " restart called ";
  runAgain = true;
  Quit ();
}


void
Loco::Quit ()
{
  CloseCleanup ();
  if (app) {
    app->quit();
  }
}

void
Loco::closeEvent (QCloseEvent *event)
{
  Q_UNUSED (event)
  CloseCleanup ();
}

void
Loco::CloseCleanup ()
{
  QSize currentSize = size();
  Settings().setValue ("sizes/main",currentSize);
  Settings().sync();
}

void
Loco::EditSettings ()
{
  configEdit.Exec ();
  SetSettings ();
}

void
Loco::SetSettings ()
{
  Settings().sync ();
}

void
Loco::About ()
{
  QString version (deliberate::ProgramVersion::Version());
  QStringList messages;
  messages.append (version);
  messages.append (configMessages);

  QMessageBox  box;
  box.setText (version);
  box.setDetailedText (messages.join ("\n"));
  QTimer::singleShot (30000, &box, SLOT (accept()));
  box.exec ();
}

void
Loco::Exiting ()
{
  QSize currentSize = size();
  Settings().setValue ("sizes/main",currentSize);
  Settings().sync();
}

void
Loco::License ()
{
  if (helpView) {
    helpView->Show ("qrc:/help/LICENSE.txt");
  }
}

void
Loco::NewPosition (const QGeoPositionInfo & here)
{
  static QGeoCoordinate old;
  QGeoCoordinate coord = here.coordinate();
  QString newPlace = coord.toString();
  qreal dist = old.distanceTo (coord);
  qreal azi = old.azimuthTo (coord);
  QString directions = QString (" %1 km head %2 (%3)")
                       .arg (dist/1000.0)
                       .arg (CompassDir(azi))
                       .arg (azi);
  old = coord;
  lastLocation = coord;
  QDateTime now = QDateTime::currentDateTime();
#if 0
  mainUi.msgLog->append (QString ("%3 go %1 to %2")
                         .arg (directions)
                         .arg (newPlace)
                         .arg (now.toString ("hh:mm:ss")));
#endif
  dist = coord.distanceTo (destination);
  mainUi.destination->setText (tr("going %4 (%5) to %1 at %2 with %3 km left")
                               .arg (destName)
                               .arg (destination.toString())
                               .arg (qRound(dist/1000.0))
                               .arg (CompassDir (azi))
                               .arg (azi));
  QString stats ("Cache: Hits %1 Misses %2");
  mainUi.statLabel->setText (stats
                     .arg (mainUi.displayMap->CacheHits())
                     .arg (mainUi.displayMap->CacheMisses()));
}

void
Loco::NewDestination (const QGeoCoordinate & whereTo,
                      const QString & name)
{
  destination = whereTo;
  destName = name;
  QDateTime now = QDateTime::currentDateTime();
  mainUi.destination->setText (tr("going to %1 at %2")
                                 .arg (name)
                                 .arg (whereTo.toString()));
  double dist = lastLocation.distanceTo (whereTo);
  mainUi.msgLog->append (tr("At %1 destination %2 (%3)  %4 nm %5 mi %6 km")
                         .arg (now.toString("hh:mm:ss"))
                         .arg (name)
                         .arg (whereTo.toString())
                         .arg (dist/1852.0)
                         .arg (dist/1609.344)
                         .arg (dist/1000.0));
}

void
Loco::UpdateMap (QRect rect)
{
  update (rect);
}

QString
Loco::CompassDir (double degrees)
{
  static QString name[16] =
      { "N", "NNE", "NE", "ENE", 
        "E", "ESE", "SE", "SSE",
        "S", "SSW", "SW", "WSW",
        "W", "WNW", "NW", "NNW"
      };
  int point = qRound (degrees/(360.0/16.0));
  return name[point];
}

void
Loco::download ()
{
  qDebug () << "Loco::download ";
}

void
Loco::handleNetworkData (QNetworkReply *reply)
{
  qDebug () << " Network Data arrived " << reply;
  qDebug () << " from " << reply->url();
}


} // namespace

