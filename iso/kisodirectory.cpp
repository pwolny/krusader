/*
    SPDX-FileCopyrightText: 2002 Szombathelyi György <gyurco@users.sourceforge.net>
    SPDX-FileCopyrightText: 2004-2020 Krusader Krew [https://krusader.org]

    This file is part of Krusader [https://krusader.org].

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kisodirectory.h"

KIsoDirectory::KIsoDirectory(KArchive* archive, const QString& name, int access,
			     int date, int adate, int cdate, const QString& user, const QString& group,
			     const QString& symlink) :
	KArchiveDirectory(archive, name, access, QDateTime::fromTime_t(date), user, group, symlink)
{


    m_adate = adate;
    m_cdate = cdate;
}

KIsoDirectory::~KIsoDirectory()
= default;
