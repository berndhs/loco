#ifndef DENADA_H
#define DENADA_H

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

#include "loco-global.h"
#include <QMainWindow>
#include "ui_loco.h"
#include "config-edit.h"
#include "helpview.h"
#include <QtLocation/QGeoPositionInfo>
#include <QRect>
#include "slippy.h"
#include "light-map.h"
#include "connectivityhelper.h"

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define HOLD_TIME 701

// maximum size of the magnifier
// Hint: see above to find why I picked this one :)
#define MAX_MAGNIFIER 229

class QApplication;

using namespace QtMobility;


using namespace deliberate;

namespace loco
{

class Locator;

class Loco : public QMainWindow
{
  Q_OBJECT

public:

  Loco (const QString & tour, QWidget *parent=0);

  void  Init (QApplication &ap);
  void  SetArgs (const QStringList & arglist);
  bool  Run ();
  void  RunSlippy ();
  bool  Again ();

  void  AddConfigMessages (const QStringList & cm) {
    configMessages.append (cm);
  }

  void closeEvent ( QCloseEvent *event);


private slots:

  void Quit ();
  void Restart ();
  void EditSettings ();
  void SetSettings ();
  void About ();
  void License ();
  void Exiting ();
  void UpdateMap (QRect rect);

  void NewPosition (const QGeoPositionInfo & here);
  void NewDestination (const QGeoCoordinate & whereTo,
                       const QString & name);
  void handleNetworkData(QNetworkReply *reply);
  void download ();

signals:

  void updated ();

private:

  void Connect ();
  void CloseCleanup ();
  QString  CompassDir (double degrees);

  bool             initDone;
  QApplication    *app;
  Ui_LocoMain    mainUi;

  ConfigEdit       configEdit;
  QStringList      configMessages;

  deliberate::HelpView        *helpView;
  bool             runAgain;

  Locator    *locator;
  QNetworkSession *netSession;
  ConnectivityHelper * conHelp;
  QString          networkSetupError;
  QGeoCoordinate   destination;
  QString          destName;
  QGeoCoordinate   lastLocation;

  int              normalStep;
  int              shortStep;
  bool             steppingNormal;

};

} // namespace

#endif
