/* Copyright (C) 2017
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "simulatoremulatedfactory.h"
#include "simulatoremulated.h"
#include "blackmisc/simulation/simulatorplugininfo.h"
#include <QTimer>

using namespace BlackMisc;
using namespace BlackMisc::Simulation;

namespace BlackSimPlugin
{
    namespace Emulated
    {
        BlackCore::ISimulator *CSimulatorEmulatedFactory::create(const CSimulatorPluginInfo &info,
                IOwnAircraftProvider *ownAircraftProvider,
                IRemoteAircraftProvider *remoteAircraftProvider,
                BlackMisc::Weather::IWeatherGridProvider *weatherGridProvider)
        {
            Q_ASSERT(ownAircraftProvider);
            return new CSimulatorEmulated(info, ownAircraftProvider, remoteAircraftProvider, weatherGridProvider, this);
        }

        BlackCore::ISimulatorListener *CSimulatorEmulatedFactory::createListener(const CSimulatorPluginInfo &info)
        {
            return new CSimulatorEmulatedListener(info);
        }
    } // namespace
} // namespace
