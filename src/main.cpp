
#include "main.h"

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
#include <QSystemInfo>
#include "deliberate.h"
#include "version.h"
#include "cmdoptions.h"
#include "loco.h"


int
main (int argc, char *argv[])
{
  QCoreApplication::setOrganizationName ("BerndStramm");
  QCoreApplication::setOrganizationDomain ("bernd-stramm.com");
  QCoreApplication::setApplicationName ("loco");
  deliberate::ProgramVersion pv ("Loco");
  QCoreApplication::setApplicationVersion (pv.Version());


  QApplication  app (argc, argv);
  QSystemInfo sysInfo;

  QSettings  settings;
  deliberate::InitSettings ();
  deliberate::SetSettings (settings);
  settings.setValue ("program",pv.MyName());


  QStringList  configMessages;

  deliberate::CmdOptions  opts ("loco");
  opts.AddSoloOption ("debug","D",QObject::tr("show Debug log window"));
  opts.AddStringOption ("logdebug","L",QObject::tr("write Debug log to file"));

  deliberate::UseMyOwnMessageHandler ();

  bool optsOk = opts.Parse (argc, argv);
  if (!optsOk) {
    opts.Usage ();
    exit(1);
  }
  if (opts.WantHelp ()) {
    opts.Usage ();
    exit (0);
  }
  pv.CLIVersion ();
  configMessages.append (QString ("Portions %1")
         .arg ("Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)"));
  configMessages.append (QString("Built on %1 %2")
                         .arg (__DATE__).arg(__TIME__));
  configMessages.append (QObject::tr("Build with Qt %1").arg(QT_VERSION_STR));
  configMessages.append (QObject::tr("Running with Qt %1").arg(qVersion()));

  configMessages.append (QObject::tr("Current Country %1")
                         .arg(sysInfo.currentCountryCode()));
  configMessages.append (QObject::tr("Current Language %1")
                          .arg(sysInfo.currentLanguage()));
  configMessages.append (QObject::tr("OS %1")
                          .arg (sysInfo.version (QSystemInfo::Os)));
  configMessages.append (QObject::tr("Qt Core %1")
                          .arg (sysInfo.version (QSystemInfo::QtCore)));
  configMessages.append (QObject::tr("Firmware %1")
                          .arg (sysInfo.version (QSystemInfo::Firmware)));
  for (int cm=0; cm<configMessages.size(); cm++) {
    deliberate::StdOut () << configMessages[cm] << endl;
  }
  if (opts.WantVersion ()) {
    exit (0);
  }
  bool showDebug = opts.SeenOpt ("debug");
  int result;

#if DELIBERATE_DEBUG
  deliberate::StartDebugLog (showDebug);
  bool logDebug = opts.SeenOpt ("logdebug");
  if (logDebug) {
    QString logfile ("/dev/null");
    opts.SetStringOpt ("logdebug",logfile);
    deliberate::StartFileLog (logfile);
  }
#endif
  
  loco::Loco   loco;

  app.setWindowIcon (loco.windowIcon());
  loco.Init (app);
  loco.AddConfigMessages (configMessages);

  loco.Run ();
  loco.RunSlippy ();
  result = app.exec ();
  qDebug () << " QApplication exec finished " << result;
  return result;
}
