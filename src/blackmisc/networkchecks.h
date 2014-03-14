/* Copyright (C) 2013 VATSIM Community / authors
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BLACKMISC_NETWORKCHECKS_H
#define BLACKMISC_NETWORKCHECKS_H

#include "nwserver.h"
#include <QString>

namespace BlackMisc
{

    /*!
     * \brief Utilities checking whether a network connection can be established
     */
    class CNetworkChecks
    {
    private:
        /*!
         * \brief Constructor
         */
        CNetworkChecks() {}

    public:
        /*!
         * \brief Is a connected interface available?
         * \param withDebugOutput enables some debugging output
         * \return
         */
        static bool hasConnectedInterface(bool withDebugOutput = false);

        /*!
         * \brief Can connect?
         * \param hostAddress   130.4.20.3, or myserver.com
         * \param port          80, 1234
         * \param timeoutMs
         * \param message       human readable message
         * \return
         */
        static bool canConnect(const QString &hostAddress, quint16 port, QString &message, int timeoutMs = 1500);

        /*!
         * \brief Can connect to server?
         * \param server
         * \param message       human readable message
         * \param timeoutMs
         * \return
         */
        static bool canConnect(const BlackMisc::Network::CServer &server, QString &message, int timeoutMs = 1500);

        //! \brief Valid IPv4 address
        static bool isValidIPv4Address(const QString &candidate);

        //! \brief Valid IPv6 address
        static bool isValidIPv6Address(const QString &candidate);

        //! \brief Valid port
        static bool isValidPort(const QString &port);

    };
} // namespace

#endif // guard

