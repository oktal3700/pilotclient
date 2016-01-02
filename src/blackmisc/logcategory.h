/* Copyright (C) 2014
 * swift Project Community / Contributors
 *
 * This file is part of swift Project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#ifndef BLACKMISC_LOGCATEGORY_H
#define BLACKMISC_LOGCATEGORY_H

//! \file

#include "blackmiscexport.h"
#include "valueobject.h"
#include "sequence.h"

namespace BlackMisc
{
    /*!
     * A log category is an arbitrary string tag which can be attached to log messages.
     *
     * A log handler can filter messages based on their categories.
     */
    class BLACKMISC_EXPORT CLogCategory : public CValueObject<CLogCategory>
    {
    public:
        //! \name Predefined special categories (public static methods)
        //! @{

        //! Uncategorized
        static const CLogCategory &uncategorized()
        {
            static const CLogCategory cat { "swift.uncategorized" };
            return cat;
        }

        //! Verification
        static const CLogCategory &verification()
        {
            static const CLogCategory cat { "swift.verification" };
            return cat;
        }

        //! Validation
        static const CLogCategory &validation()
        {
            static const CLogCategory cat { "swift.validation" };
            return cat;
        }

        //! Contexts
        static const CLogCategory &context()
        {
            static const CLogCategory cat { "swift.context" };
            return cat;
        }

        //! Context slots
        static const CLogCategory &contextSlot()
        {
            static const CLogCategory cat { "swift.context.slot" };
            return cat;
        }

        //! GUI components
        static const CLogCategory &guiComponent()
        {
            static const CLogCategory cat { "swift.gui.component" };
            return cat;
        }

        //! Generic downloads
        static const CLogCategory &download()
        {
            static const CLogCategory cat { "swift.download" };
            return cat;
        }

        //! Webservice
        static const CLogCategory &webservice()
        {
            static const CLogCategory cat { "swift.webservice" };
            return cat;
        }

        //! Webservice with swift DB
        static const CLogCategory &swiftDbWebservice()
        {
            static const CLogCategory cat { "swift.db.webservice" };
            return cat;
        }

        //! All predefined special categories
        //! \note Human readable patterns are defined in CLogPattern::allHumanReadablePatterns
        static const QList<CLogCategory> &allSpecialCategories()
        {
            static const QList<CLogCategory> cats
            {
                uncategorized(),
                validation(),
                context(),
                contextSlot(),
                guiComponent(),
                download(),
                webservice(),
                swiftDbWebservice(),
            };
            return cats;
        }

        //! @}

        //! Constructor.
        CLogCategory() = default;

        //! Constructor.
        CLogCategory(const QString &categoryString) : m_string(categoryString) {}

        //! Constructor.
        CLogCategory(const char *categoryString) : m_string(categoryString) {}

        //! Returns true if the category string starts with the given prefix.
        bool startsWith(const QString &prefix) const { return m_string.startsWith(prefix); }

        //! Returns true if the category string ends with the given suffix.
        bool endsWith(const QString &suffix) const { return m_string.endsWith(suffix); }

        //! Returns true if the category string contains the given substring.
        bool contains(const QString &substring) const { return m_string.contains(substring); }

        //! \copydoc CValueObject::convertToQString()
        QString convertToQString(bool i18n = false) const;

    private:
        BLACK_ENABLE_TUPLE_CONVERSION(CLogCategory)
        QString m_string;
    };
}

Q_DECLARE_METATYPE(BlackMisc::CLogCategory)
BLACK_DECLARE_TUPLE_CONVERSION(BlackMisc::CLogCategory, (o.m_string))

#endif
