/* Copyright (C) 2013
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKMISC_TEST_TESTUTILS_H
#define BLACKMISC_TEST_TESTUTILS_H

#include "blackmisc/blackmiscexport.h"
#include <QDBusArgument>
#include <QTextStream>

namespace BlackMisc
{
    namespace Test
    {
        //! Utils for UNIT tests / samples
        class BLACKMISC_EXPORT CTestUtils
        {
        public:
            //! Get QDBusArgument signature (formatted)
            static QString getQDBusArgumentSignature(const QDBusArgument &arg, int level = 0);

            //! Signature for BlackMisc::CValueObject
            template<typename ValueObj>
            static QString dBusSignature(const ValueObj &obj)
            {
                QDBusArgument arg;
                obj.marshallToDbus(arg);
                return arg.currentSignature();
            }

            //! Type as string
            static QString dbusTypeAsString(QDBusArgument::ElementType type);

            //! Show some (important) DBus signatures
            static void showDBusSignatures(QTextStream &out);
        };
    } // ns
} // ns

#endif // guard
