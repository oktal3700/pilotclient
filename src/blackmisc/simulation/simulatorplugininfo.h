/* Copyright (C) 2013
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKMISC_SIMULATION_SIMULATORPLUGININFO_H
#define BLACKMISC_SIMULATION_SIMULATORPLUGININFO_H

#include "blackmisc/blackmiscexport.h"
#include "blackmisc/valueobject.h"

namespace BlackMisc
{
    namespace Simulation
    {
        //! Describing a simulator plugin
        class BLACKMISC_EXPORT CSimulatorPluginInfo : public BlackMisc::CValueObject<CSimulatorPluginInfo>
        {
            //! The _identifier_ property identifies the plugin itself and must be uniqe.
            Q_PROPERTY(QString identifier READ getIdentifier)

            //! The _name_ property is a human-readable plugin name.
            Q_PROPERTY(QString same READ getName)

            //! The _simulator_ property specifies which simulator the plugin handles.
            //! There cannot be two plugins loaded for the same simulator.
            //! swift enables some features for particular simulators. Currently recognized are:
            //!   fsx, fs9, xplane
            Q_PROPERTY(QString simulator READ getSimulator)

            //! The _description_ property provides a short, human-readable description of the plugin.
            Q_PROPERTY(QString description READ getDescription)

        public:
            //! Default constructor
            CSimulatorPluginInfo() = default;

            //! Constructor (used with unit tests)
            CSimulatorPluginInfo(const QString &identifier, const QString &name,
                                 const QString &simulator, const QString &description, bool valid);

            //! \copydoc BlackMisc::CValueObject::convertFromJson
            void convertFromJson(const QJsonObject &json);

            //! Unspecified simulator
            bool isUnspecified() const;

            //! Check if the provided plugin metadata is valid.
            //! Simulator plugin (driver) has to meet the following requirements:
            //!  * implements org.swift-project.blackcore.simulatorinterface;
            //!  * provides plugin name;
            //!  * specifies simulator it handles.
            //! Unspecified sim is considered as invalid.
            bool isValid() const { return m_valid; }

            //! Identifier
            const QString &getIdentifier() const { return m_identifier; }

            //! Name
            const QString &getName() const { return m_name; }

            //! Simulator
            const QString &getSimulator() const { return m_simulator; }

            //! Description
            const QString &getDescription() const { return m_description; }

            //! Special info of type auto?
            bool isAuto() const;

            //! \copydoc CValueObject::convertToQString
            QString convertToQString(bool i18n = false) const;

            //! Info representing a entry representing automatic plugin selection
            static const CSimulatorPluginInfo &autoPlugin();

        private:
            BLACK_ENABLE_TUPLE_CONVERSION(CSimulatorPluginInfo)
            QString m_identifier;
            QString m_name;
            QString m_simulator;
            QString m_description;
            bool m_valid { false };
        };
    } // ns
} // ns

BLACK_DECLARE_TUPLE_CONVERSION(BlackMisc::Simulation::CSimulatorPluginInfo, (
                                   attr(o.m_identifier, flags <CaseInsensitiveComparison> ()),
                                   attr(o.m_name, flags < DisabledForComparison | DisabledForHashing > ()),
                                   attr(o.m_simulator, flags < DisabledForComparison | DisabledForHashing > ()),
                                   attr(o.m_description, flags < DisabledForComparison | DisabledForHashing > ()),
                                   attr(o.m_valid, flags < DisabledForComparison | DisabledForHashing > ())
                               ))
Q_DECLARE_METATYPE(BlackMisc::Simulation::CSimulatorPluginInfo)

#endif // guard
