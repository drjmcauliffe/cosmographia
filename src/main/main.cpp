// This file is part of Cosmographia.
//
// Copyright (C) 2010-2011 Chris Laurel <claurel@gmail.com>
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <QApplication>
#include <QDir>
#include <QMessageBox>
#include <QDebug>
#include <QDesktopServices>

#if defined(Q_WS_MAC) || defined(Q_OS_MAC)
#include <CoreFoundation/CFBundle.h>
#endif

#include "Cosmographia.h"
#include "FileOpenEventFilter.h"

#define MAS_DEPLOY 0


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    FileOpenEventFilter* appEventFilter = new FileOpenEventFilter();
    app.installEventFilter(appEventFilter);

#if MAS_DEPLOY
#else
    QCoreApplication::setOrganizationName("Periapsis Visual Software");
    QCoreApplication::setOrganizationDomain("periapsisvisual.com");
    QCoreApplication::setApplicationName("Cosmographia");
#endif

    // Useful when we need to know where the data files are:
    //   qDebug() << QDesktopServices::storageLocation(QDesktopServices::CacheLocation);
    //   qDebug() << QDesktopServices::storageLocation(QDesktopServices::DataLocation);

    // Set current directory so that we find the needed data files. On the Mac, we
    // just look in the app bundle. On other platforms we make some guesses, since we
    // don't know exactly where the executable will be run from.
    QString dataPath;
    bool foundData = true;
#if defined(Q_WS_MAC) || defined(Q_OS_MAC)
    // On the Mac, load resources from the app bundle. We first check for a directory
    // in a location relative to the executable. If that fails, we'll use the Core
    // Foundation bundle functions as recommended in the Qt docs. The first technique
    // *should* always work.
    QDir::setCurrent(QCoreApplication::applicationDirPath());
    if (QDir("../Resources/data").exists())
    {
        dataPath = "../Resources/data";
    }
    else
    {
        // Note that there is a bug in one of the bundle functions that prevents this from
        // working properly on a Japanese Mac OS X install.
        CFURLRef appUrlRef = CFBundleCopyBundleURL(CFBundleGetMainBundle());
        CFStringRef macPath = CFURLCopyFileSystemPath(appUrlRef, kCFURLPOSIXPathStyle);
        const char *pathPtr = CFStringGetCStringPtr(macPath, CFStringGetSystemEncoding());
        dataPath = QString(pathPtr) + "/Contents/Resources/data";
        CFRelease(appUrlRef);
        CFRelease(macPath);
    }
#else
    QDir::setCurrent(QCoreApplication::applicationDirPath());
    if (QDir("../data").exists())
    {
        dataPath = "../data";
    }
    else if (QDir("../../data").exists())
    {
        // QtCreator builds in the debug/ or release/ directory
        dataPath = "../../data";
    }
    else if (QDir("../../cosmographia/data").exists())
    {
        dataPath = "../../cosmographia/data";
    }
    else if (QDir("../../trunk/data").exists())
    {
        dataPath = "../../trunk/data";
    }
    else if (QDir("./data").exists())
    {
        dataPath = "./data";
    }
    else
    {
        foundData = false;
    }
#endif
    if (!foundData || !QDir::setCurrent(dataPath))
    {
        QMessageBox::warning(NULL, "Missing data", "Data files not found!");
        exit(0);
    }

    Cosmographia mainWindow;
    mainWindow.initialize();
    mainWindow.show();

    // Special handling for file open events that arrive before the main window is
    // ready.
    if (!appEventFilter->lastUrl().isEmpty())
    {
        mainWindow.activateCosmoUrl(appEventFilter->lastUrl());
    }

    QObject::connect(appEventFilter, SIGNAL(urlOpened(QString)), &mainWindow, SLOT(activateCosmoUrl(QString)));

    return app.exec();
}
