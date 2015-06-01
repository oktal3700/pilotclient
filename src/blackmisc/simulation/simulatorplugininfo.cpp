/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "simulatorplugininfo.h"

using namespace BlackMisc;

namespace BlackMisc
{
    namespace Simulation
    {
        CSimulatorPluginInfo::CSimulatorPluginInfo(const QString &identifier, const QString &name, const QString &simulator, const QString &description, bool valid) :
            m_identifier(identifier), m_name(name), m_simulator(simulator), m_description(description), m_valid(valid)
        { }

        void CSimulatorPluginInfo::convertFromJson(const QJsonObject &json)
        {
            if (json.contains("IID"))   // comes from the plugin
            {
                if (json["IID"].toString() != QStringLiteral("org.swift-project.blackcore.simulatorinterface"))
                {
                    return;
                }

                if (!json["MetaData"].isObject())
                {
                    return;
                }

                QJsonObject data = json["MetaData"].toObject();
                if (data.value("identifier").isUndefined() || data.value("simulator").isUndefined())
                {
                    return;
                }

                CValueObject::convertFromJson(data);
                m_valid = true;
            }
            else
            {
                CValueObject::convertFromJson(json);
            }
        }

        bool CSimulatorPluginInfo::isUnspecified() const
        {
            return m_identifier.isEmpty();
        }

        bool CSimulatorPluginInfo::isAuto() const
        {
            return (*this) == autoPlugin();
        }

        QString CSimulatorPluginInfo::convertToQString(bool i18n) const
        {
            Q_UNUSED(i18n);
            return QString("%1 (%2)").arg(m_name, m_identifier);
        }

        const CSimulatorPluginInfo &CSimulatorPluginInfo::autoPlugin()
        {
            static const CSimulatorPluginInfo p("auto", "auto", "auto", "automatic plugin selection", false);
            return p;
        }
    } // ns
} // ns
