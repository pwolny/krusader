/***************************************************************************
                                krsearchmod.cpp
                            -------------------
   copyright            : (C) 2001 by Shie Erlich & Rafi Yanai
   email                : krusader@users.sourceforge.net
   web site             : http://krusader.sourceforge.net
---------------------------------------------------------------------------
 Description
***************************************************************************

 A

    db   dD d8888b. db    db .d8888.  .d8b.  d8888b. d88888b d8888b.
    88 ,8P' 88  `8D 88    88 88'  YP d8' `8b 88  `8D 88'     88  `8D
    88,8P   88oobY' 88    88 `8bo.   88ooo88 88   88 88ooooo 88oobY'
    88`8b   88`8b   88    88   `Y8b. 88~~~88 88   88 88~~~~~ 88`8b
    88 `88. 88 `88. 88b  d88 db   8D 88   88 88  .8D 88.     88 `88.
    YP   YD 88   YD ~Y8888P' `8888Y' YP   YP Y8888D' Y88888P 88   YD

                                                    S o u r c e    F i l e

***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "krsearchmod.h"

// QtCore
#include <QDir>
#include <QMimeDatabase>
#include <QMimeType>
#include <QRegExp>
// QtWidgets
#include <QApplication>
#include <qplatformdefs.h>

#include <KIO/Global>

#include "../FileSystem/krquery.h"
#include "../FileSystem/vfile.h"
#include "../FileSystem/krpermhandler.h"
#include "../Archive/krarchandler.h"

#define  EVENT_PROCESS_DELAY     250

extern KRarcHandler arcHandler;

KRSearchMod::KRSearchMod(const KRQuery* q)
{
    stopSearch = false; /// ===> added
    query = new KRQuery(*q);
    connect(query, SIGNAL(status(const QString &)),
            this,  SIGNAL(searching(const QString&)));
    connect(query, SIGNAL(processEvents(bool &)),
            this,  SLOT(slotProcessEvents(bool &)));

    remote_vfs = 0;
    virtual_vfs = 0;
}

KRSearchMod::~KRSearchMod()
{
    delete query;
    if (remote_vfs)
        delete remote_vfs;
    if (virtual_vfs)
        delete virtual_vfs;
}

void KRSearchMod::start()
{
    unScannedUrls.clear();
    scannedUrls.clear();
    timer.start();

    QList<QUrl> whereToSearch = query->searchInDirs();

    // search every dir that needs to be searched
    for (int i = 0; i < whereToSearch.count(); ++i)
        scanURL(whereToSearch [ i ]);

    emit finished();
}

void KRSearchMod::stop()
{
    stopSearch = true;
}

void KRSearchMod::scanURL(QUrl url)
{
    if (stopSearch) return;

    unScannedUrls.push(url);
    while (!unScannedUrls.isEmpty()) {
        QUrl urlToCheck = unScannedUrls.pop();

        if (stopSearch) return;

        if (query->isExcluded(urlToCheck)) {
            if (!query->searchInDirs().contains(urlToCheck))
                continue;
        }

        if (scannedUrls.contains(urlToCheck))
            continue;
        scannedUrls.push(urlToCheck);

        emit searching(urlToCheck.toDisplayString(QUrl::PreferLocalFile));

        if (urlToCheck.isLocalFile())
            scanLocalDir(urlToCheck);
        else
            scanRemoteDir(urlToCheck);
    }
}

void KRSearchMod::scanLocalDir(const QUrl &url)
{
    const QString dir = vfs::ensureTrailingSlash(url).path();

    QT_DIR *qdir = QT_OPENDIR(dir.toLocal8Bit());
    if (!qdir)
        return;

    QT_DIRENT *dirEnt;

    while ((dirEnt = QT_READDIR(qdir)) != NULL) {
        const QString name = QString::fromLocal8Bit(dirEnt->d_name);

        // we don't scan the ".",".." enteries
        if (name == "." || name == "..")
            continue;

        const QString fullName = dir + name;
        const QUrl fileUrl = QUrl::fromLocalFile(fullName);

        QT_STATBUF stat_p;
        QT_LSTAT(fullName.toLocal8Bit(), &stat_p);

        if (query->isRecursive()) {
            if (S_ISLNK(stat_p.st_mode) && query->followLinks())
                unScannedUrls.push(QUrl::fromLocalFile(QDir(fullName).canonicalPath()));
            else if (S_ISDIR(stat_p.st_mode))
                unScannedUrls.push(fileUrl);
        }

        // creating a vfile object for matching with krquery
        vfile *vf = vfs::createLocalVFile(name, dir);

        if (query->searchInArchives()) {
            const QString mime = vf->vfile_getMime();
            if (KRarcHandler::arcSupported(mime)) {
                QUrl archiveURL = fileUrl;
                bool encrypted;
                const QString realType = arcHandler.getType(encrypted, fileUrl.path(), mime);

                if (!encrypted) {
                    if (realType == "tbz" || realType == "tgz" || realType == "tarz" ||
                        realType == "tar" || realType == "tlz")
                        archiveURL.setScheme("tar");
                    else
                        archiveURL.setScheme("krarc");

                    unScannedUrls.push(archiveURL);
                }
            }
        }

        if (query->match(vf)) {
            // if we got here - we got a winner
            results.append(fullName);

            emit found(*vf, query->foundText()); // emitting copy of vfile
        }
        delete vf;

        if (timer.elapsed() >= EVENT_PROCESS_DELAY) {
            qApp->processEvents();
            timer.start();
            if (stopSearch)
                return;
        }
    }
    // clean up
    QT_CLOSEDIR(qdir);
}

void KRSearchMod::scanRemoteDir(QUrl url)
{
    vfs * vfs_;

    if (url.scheme() == QStringLiteral("virt")) {
        if (virtual_vfs == 0)
            virtual_vfs = new virt_vfs();
        vfs_ = virtual_vfs;
    } else {
        if (remote_vfs == 0)
            remote_vfs = new DefaultFileSystem();
        vfs_ = remote_vfs;
    }

    if (!vfs_->refresh(url)) return ;

    for (vfile *vf : vfs_->vfiles()) {
        QUrl fileURL = vf->vfile_getUrl();

        if (query->isRecursive() && ((vf->vfile_isSymLink() && query->followLinks()) || vf->vfile_isDir()))
            unScannedUrls.push(fileURL);

        if (query->match(vf)) {
            // if we got here - we got a winner
            results.append(fileURL.toDisplayString(QUrl::PreferLocalFile));

            emit found(*vf, query->foundText()); // emitting copy of vfile
        }

        if (timer.elapsed() >= EVENT_PROCESS_DELAY) {
            qApp->processEvents();
            timer.start();
            if (stopSearch) return;
        }
    }
}

void KRSearchMod::slotProcessEvents(bool & stopped)
{
    qApp->processEvents();
    stopped = stopSearch;
}

