/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackmisc/network/serverlist.h"

namespace BlackMisc
{
    namespace Network
    {

        CServerList::CServerList() { }

        CServerList::CServerList(const CSequence<CServer> &other) :
            CSequence<CServer>(other)
        { }

        bool CServerList::containsName(const QString &name) const
        {
            for (const CServer &s : *this)
            {
                if (s.matchesName(name)) { return true; }
            }
            return false;
        }

    } // namespace
} // namespace
