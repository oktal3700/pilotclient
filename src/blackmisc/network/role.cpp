/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackmisc/network/role.h"
#include <QJsonValue>
#include <QtGlobal>
#include <QStringBuilder>

namespace BlackMisc
{
    namespace Network
    {
        CRole::CRole(const QString &name, const QString &description)
            : m_name(name), m_description(description)
        {  }

        QString CRole::convertToQString(bool i18n) const
        {
            Q_UNUSED(i18n);
            return u"Role: " % m_name %
                   u" description: " % m_description %
                   this->getDbKeyAsStringInParentheses(" ");
        }

        CVariant CRole::propertyByIndex(const CPropertyIndex &index) const
        {
            if (index.isMyself()) { return CVariant::from(*this); }
            if (IDatastoreObjectWithIntegerKey::canHandleIndex(index)) { return IDatastoreObjectWithIntegerKey::propertyByIndex(index); }
            const ColumnIndex i = index.frontCasted<ColumnIndex>();
            switch (i)
            {
            case IndexName: return CVariant::fromValue(m_name);
            case IndexDescription: return CVariant::fromValue(m_description);
            default: break;
            }
            return CValueObject::propertyByIndex(index);
        }

        void CRole::setPropertyByIndex(const CPropertyIndex &index, const CVariant &variant)
        {
            if (index.isMyself()) { (*this) = variant.to<CRole>(); return; }
            if (IDatastoreObjectWithIntegerKey::canHandleIndex(index)) { IDatastoreObjectWithIntegerKey::setPropertyByIndex(index, variant); return; }
            const ColumnIndex i = index.frontCasted<ColumnIndex>();
            switch (i)
            {
            case IndexName: this->setName(variant.value<QString>()); break;
            case IndexDescription: this->setDescription(variant.value<QString>()); break;
            default: break;
            }
            CValueObject::setPropertyByIndex(index, variant);
        }

        CRole CRole::fromDatabaseJson(const QJsonObject &json)
        {
            CRole role;
            role.setName(json.value("name").toString());
            role.setDescription(json.value("description").toString());
            role.setDbKey(json.value("idrole").toInt(-1));
            return role;
        }
    } // ns
} // ns
