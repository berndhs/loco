#
# nothing application
#

#/****************************************************************
# * This file is distributed under the following license:
# *
# * Copyright (C) 2016, Bernd Stramm
# *
# *  This program is free software; you can redistribute it and/or
# *  modify it under the terms of the GNU General Public License
# *  as published by the Free Software Foundation; either version 2
# *  of the License, or (at your option) any later version.
# *
# *  This program is distributed in the hope that it will be useful,
# *  but WITHOUT ANY WARRANTY; without even the implied warranty of
# *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# *  GNU General Public License for more details.
# *
# *  You should have received a copy of the GNU General Public License
# *  along with this program; if not, write to the Free Software
# *  Foundation, Inc., 51 Franklin Street, Fifth Floor, 
# *  Boston, MA  02110-1301, USA.
# ****************************************************************/

MYNAME = loco

TEMPLATE = app

ARCH = $$system(uname -m)


QT += core gui
QT += widgets
QT += sql network xml
QT += positioning

CONFIG += debug_and_release
CONFIG += bearer

MAKEFILE = Makefile

CONFIG(debug, debug|release) {
  DEFINES +=  QT_MESSAGELOGCONTEXT
  DEFINES += DELIBERATE_DEBUG=1
  TARGET = bin/$${MYNAME}_d
  OBJECTS_DIR = tmp/debug/obj
  message ("DEBUG cxx-flags used $${QMAKE_CXXFLAGS_DEBUG}")
  message ("DEBUG c-flags used $${QMAKE_CFLAGS_DEBUG}")
} else {
  DEFINES +=  QT_MESSAGELOGCONTEXT
  DEFINES += DELIBERATE_DEBUG=0
  TARGET = bin/$${MYNAME}
  OBJECTS_DIR = tmp/release/obj
  QMAKE_CXXFLAGS_RELEASE -= -g
  QMAKE_CFLAGS_RELEASE -= -g
  message ("RELEASE cxx-flags used $${QMAKE_CXXFLAGS_RELEASE}")
  message ("RELEASE c-flags used $${QMAKE_CFLAGS_RELEASE}")
}



UI_DIR = tmp/ui
MOC_DIR = tmp/moc
RCC_DIR = tmp/rcc
RESOURCES = $${MYNAME}.qrc

FORMS = \
        ui/$${MYNAME}.ui \
        ui/DebugLog.ui \
        ui/config-edit.ui \
#        ui/helpwin.ui \
        

HEADERS = \
          src/$${MYNAME}.h \
          src/main.h \
          src/gpl2.h \
          src/cmdoptions.h \
          src/config-edit.h \
          src/delib-debug.h \
          src/deliberate.h \
          src/version.h \
#          src/helpview.h \
          src/locator.h \
          src/slippy.h \
          src/loco-global.h \
          src/connectivityhelper.h \
          src/light-map.h \
          src/slip-cache.h \


SOURCES = \
          src/$${MYNAME}.cpp \
          src/main.cpp \
          src/cmdoptions.cpp \
          src/config-edit.cpp \
          src/delib-debug.cpp \
          src/deliberate.cpp \
          src/version.cpp \
#          src/helpview.cpp \
          src/locator.cpp \
          src/slippy.cpp \
          src/loco-global.cpp \
          src/connectivityhelper.cpp \
          src/light-map.cpp \
          src/slip-cache.cpp \

DISTFILES += \
    tour-default


#DISTFILES += \
#    android/AndroidManifest.xml \
#    android/gradle/wrapper/gradle-wrapper.jar \
#    android/gradlew \
#    android/res/values/libs.xml \
#    android/build.gradle \
#    android/gradle/wrapper/gradle-wrapper.properties \
#    android/gradlew.bat

#ANDROID_PACKAGE_SOURCE_DIR = ../build-loco-Android_for_armeabi_v7a_GCC_4_9_Qt_5_7_1-Debug/
#message ("look in "$$ANDROID_PACKAGE_SOURCE_DIR);



