/* Copyright (C) 2013
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKCORE_CONTEXTSIMULATOR_H
#define BLACKCORE_CONTEXTSIMULATOR_H

//! \addtogroup dbus
//! @{

//! DBus interface for context
#define BLACKCORE_CONTEXTSIMULATOR_INTERFACENAME "org.swift_project.blackcore.contextsimulator"

//! DBus object path for context
#define BLACKCORE_CONTEXTSIMULATOR_OBJECTPATH "/simulator"

//! @}

#include "blackconfig/buildconfig.h"
#include "blackcore/blackcoreexport.h"
#include "blackcore/context/context.h"
#include "blackcore/corefacade.h"
#include "blackcore/corefacadeconfig.h"
#include "blackcore/simulator.h"
#include "blackmisc/aviation/airportlist.h"
#include "blackmisc/identifier.h"
#include "blackmisc/pixmap.h"
#include "blackmisc/pq/length.h"
#include "blackmisc/pq/time.h"
#include "blackmisc/simulation/aircraftmodellist.h"
#include "blackmisc/simulation/simulatorplugininfo.h"
#include "blackmisc/simulation/simulatorplugininfolist.h"
#include "blackmisc/simulation/simulatorsetup.h"
#include "blackmisc/weather/weathergrid.h"

#include <QObject>
#include <QString>

class QDBusConnection;

namespace BlackMisc
{
    class CDBusServer;
    namespace Simulation { class CSimulatedAircraft; }
}
namespace BlackCore
{
    namespace Context
    {
        //! Network context
        class BLACKCORE_EXPORT IContextSimulator : public CContext
        {
            Q_OBJECT
            Q_CLASSINFO("D-Bus Interface", BLACKCORE_CONTEXTSIMULATOR_INTERFACENAME)

        public:
            //! Service name
            static const QString &InterfaceName();

            //! Service path
            static const QString &ObjectPath();

            //! Highlight time
            static const BlackMisc::PhysicalQuantities::CTime &HighlightTime();

            //! \copydoc CContext::getPathAndContextId()
            virtual QString getPathAndContextId() const { return this->buildPathAndContextId(ObjectPath()); }

            //! Factory method
            static IContextSimulator *create(CCoreFacade *parent, CCoreFacadeConfig::ContextMode mode, BlackMisc::CDBusServer *server, QDBusConnection &conn);

            //! Destructor
            virtual ~IContextSimulator() {}

        signals:
            //! Simulator combined status
            //! \sa ISimulator::SimulatorStatus
            void simulatorStatusChanged(int status);

            //! Simulator plugin loaded / unloaded (default info)
            void simulatorPluginChanged(BlackMisc::Simulation::CSimulatorPluginInfo info);

            //! Render restrictions have been changed
            void renderRestrictionsChanged(bool restricted, bool enabled, int maxAircraft, const BlackMisc::PhysicalQuantities::CLength &maxRenderedDistance, const BlackMisc::PhysicalQuantities::CLength &maxRenderedBoundary);

            //! Installed aircraft models ready or changed
            void installedAircraftModelsChanged();

            //! A single model has been matched for given aircraft
            void modelMatchingCompleted(const BlackMisc::Simulation::CSimulatedAircraft &aircraft);

            //! Aircraft rendering changed
            void aircraftRenderingChanged(const BlackMisc::Simulation::CSimulatedAircraft &aircraft);

            //! Emitted when own aircraft model changes
            void ownAircraftModelChanged(const BlackMisc::Simulation::CAircraftModel &model);

            //! An airspace snapshot was handled
            void airspaceSnapshotHandled();

            //! A weather grid, requested with requestWeatherGrid(), is received
            void weatherGridReceived(const BlackMisc::Weather::CWeatherGrid &weatherGrid, const BlackMisc::CIdentifier &identifier);

        public slots:
            //! Simulator info, currently loaded plugin
            virtual BlackMisc::Simulation::CSimulatorPluginInfo getSimulatorPluginInfo() const = 0;

            //! Return list of available simulator plugins
            virtual BlackMisc::Simulation::CSimulatorPluginInfoList getAvailableSimulatorPlugins() const = 0;

            //! Load and start specific simulator plugin
            virtual bool startSimulatorPlugin(const BlackMisc::Simulation::CSimulatorPluginInfo &simulatorInfo) = 0;

            //! Stop listener or unload the given plugin (if currently loaded)
            virtual void stopSimulatorPlugin(const BlackMisc::Simulation::CSimulatorPluginInfo &simulatorInfo) = 0;

            //! Simulator combined status
            virtual int getSimulatorStatus() const = 0;

            //! Get simulator status as enum
            //! \todo To be removed with Qt 5.5 when getSimualtorStatus directly provides the enum
            ISimulator::SimulatorStatus getSimulatorStatusEnum() const;

            //! Simulator setup
            virtual BlackMisc::Simulation::CSimulatorSetup getSimulatorSetup() const = 0;

            //! Airports in range
            virtual BlackMisc::Aviation::CAirportList getAirportsInRange() const = 0;

            //! Reload models from disk
            virtual void reloadInstalledModels() = 0;

            //! Installed models in simulator eco system
            virtual BlackMisc::Simulation::CAircraftModelList getInstalledModels() const = 0;

            //! Number of installed models in simulator eco system
            virtual int getInstalledModelsCount() const = 0;

            //! Model for model string
            virtual BlackMisc::Simulation::CAircraftModelList getInstalledModelsStartingWith(const QString modelString) const = 0;

            //! Set time synchronization between simulator and user's computer time
            //! \remarks not all drivers implement this, e.g. if it is an intrinsic simulator feature
            virtual bool setTimeSynchronization(bool enable, const BlackMisc::PhysicalQuantities::CTime &offset) = 0;

            //! Is time synchronization on?
            virtual bool isTimeSynchronized() const = 0;

            //! Max. number of remote aircraft rendered
            virtual int getMaxRenderedAircraft() const = 0;

            //! Max. rendered distance
            virtual BlackMisc::PhysicalQuantities::CLength getMaxRenderedDistance() const = 0;

            //! Technical range until aircraft are visible
            virtual BlackMisc::PhysicalQuantities::CLength getRenderedDistanceBoundary() const = 0;

            //! Text describing the rendering restrictions
            virtual QString getRenderRestrictionText() const = 0;

            //! Max. number of remote aircraft rendered and provide optional selection which aircraft those are
            virtual void setMaxRenderedAircraft(int number) = 0;

            //! Max. distance until we render an aircraft
            virtual void setMaxRenderedDistance(const BlackMisc::PhysicalQuantities::CLength &distance) = 0;

            //! Delete all restrictions (if any) -> unlimited number of aircraft
            virtual void deleteAllRenderingRestrictions() = 0;

            //! Is number of aircraft restricted ormax distance set?
            virtual bool isRenderingRestricted() const = 0;

            //! Rendering enabled at all
            virtual bool isRenderingEnabled() const = 0;

            //! Time synchronization offset
            virtual BlackMisc::PhysicalQuantities::CTime getTimeSynchronizationOffset() const = 0;

            //! Simulator avialable (driver available)?
            bool isSimulatorAvailable() const { return BlackConfig::CBuildConfig::isCompiledWithFlightSimulatorSupport() && !getSimulatorPluginInfo().isUnspecified(); }

            //! Is available simulator simulating? Returns false if no simulator is available
            bool isSimulatorSimulating() const;

            //! Icon representing the model
            virtual BlackMisc::CPixmap iconForModel(const QString &modelString) const = 0;

            //! Enable debugging
            virtual void enableDebugMessages(bool driver, bool interpolator) = 0;

            //! Highlight aircraft in simulator
            virtual void highlightAircraft(const BlackMisc::Simulation::CSimulatedAircraft &aircraftToHighlight, bool enableHighlight, const BlackMisc::PhysicalQuantities::CTime &displayTime) = 0;

            //! Request weather grid. Argument identifier is past in the signal to identify the requestor
            virtual void requestWeatherGrid(const BlackMisc::Weather::CWeatherGrid &weatherGrid, const BlackMisc::CIdentifier &identifier) = 0;

        protected:
            //! Constructor
            IContextSimulator(CCoreFacadeConfig::ContextMode mode, CCoreFacade *runtime) : CContext(mode, runtime) {}
        };
    } // namespace
} // namespace

#endif // guard
