/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKMISC_ICON_H
#define BLACKMISC_ICON_H

#include "icons.h"
#include "pqangle.h"

namespace BlackMisc
{
    /*!
     * Value object for icons.
     */
    class CIcon : public CValueObjectStdTuple<CIcon>
    {
    public:
        //! Default constructor.
        CIcon() = default;

        //! Constructor.
        CIcon(CIcons::IconIndex index, const QString &descriptiveText) :
            m_index(static_cast<int>(index)), m_descriptiveText(descriptiveText) {}

        //! Get descriptive text
        const QString &getDescriptiveText() const { return this->m_descriptiveText; }

        //! Index
        CIcons::IconIndex getIndex() const { return static_cast< CIcons::IconIndex>(this->m_index);}

        //! Corresponding pixmap
        QPixmap toPixmap() const;

        //! Icon set?
        bool isSet() const { return (this->m_index != static_cast<int>(CIcons::NotSet));}

        //! Rotate by n degrees
        void setRotation(int degrees) { this->m_rotateDegrees = degrees; }

        //! Rotate by given degrees
        void setRotation(const BlackMisc::PhysicalQuantities::CAngle &rotate);

        //! Set descriptive text
        void setDescriptiveText(const QString &text) { this->m_descriptiveText = text; }

        //! Implicit conversion
        operator QPixmap() const { return this->toPixmap(); }

    protected:
        //! \copydoc CValueObject::convertToQString
        virtual QString convertToQString(bool i18n = false) const override;

    private:
        BLACK_ENABLE_TUPLE_CONVERSION(CIcon)
        int m_index = static_cast<int>(CIcons::NotSet);
        int m_rotateDegrees = 0;
        QString m_descriptiveText;
    };
} // namespace

BLACK_DECLARE_TUPLE_CONVERSION(BlackMisc::CIcon, (o.m_index, o.m_descriptiveText))
Q_DECLARE_METATYPE(BlackMisc::CIcon)

#endif // guard
