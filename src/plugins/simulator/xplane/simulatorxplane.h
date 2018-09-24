/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKSIMPLUGIN_SIMULATOR_XPLANE_H
#define BLACKSIMPLUGIN_SIMULATOR_XPLANE_H

#include "xplanempaircraft.h"
#include "plugins/simulator/xplaneconfig/simulatorxplaneconfig.h"
#include "plugins/simulator/plugincommon/simulatorplugincommon.h"
#include "blackmisc/simulation/aircraftmodellist.h"
#include "blackmisc/simulation/data/modelcaches.h"
#include "blackmisc/simulation/settings/simulatorsettings.h"
#include "blackmisc/simulation/settings/xswiftbussettings.h"
#include "blackmisc/simulation/simulatedaircraftlist.h"
#include "blackmisc/weather/weathergrid.h"
#include "blackmisc/aviation/airportlist.h"
#include "blackmisc/aviation/callsignset.h"
#include "blackmisc/geo/coordinategeodetic.h"
#include "blackmisc/pq/time.h"
#include "blackmisc/pq/units.h"
#include "blackmisc/settingscache.h"
#include "blackmisc/statusmessage.h"
#include "blackmisc/identifier.h"
#include "blackmisc/pixmap.h"
#include "blackmisc/sequence.h"

#include <QDBusConnection>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QHash>
#include <QPair>
#include <QTimer>

class QDBusServiceWatcher;

namespace BlackMisc
{
    namespace Aviation
    {
        class CAircraftParts;
        class CAircraftSituation;
        class CCallsign;
    }
    namespace Network { class CTextMessage; }
    namespace Simulation
    {
        class CSimulatedAircraft;
        class CSimulatorPluginInfo;
        class IOwnAircraftProvider;
        class IRemoteAircraftProvider;
    }
    namespace Weather { class IWeatherGridProvider; }
}

namespace BlackSimPlugin
{
    namespace XPlane
    {
        class CXSwiftBusServiceProxy;
        class CXSwiftBusTrafficProxy;
        class CXSwiftBusWeatherProxy;

        //! X-Plane data
        //! \todo Add units to members? pitchDeg?, altitudeFt?
        struct XPlaneData
        {
            QString aircraftModelPath;          //!< Aircraft model path
            QString aircraftIcaoCode;           //!< Aircraft ICAO code
            double latitude;                    //!< Longitude [deg]
            double longitude;                   //!< Latitude [deg]
            double altitude;                    //!< Altitude [m]
            double groundspeed;                 //!< Ground speed [m/s]
            double pitch;                       //!< Pitch [deg]
            double roll;                        //!< Roll [deg]
            double trueHeading;                 //!< True heading [deg]
            bool onGroundAll;                   //!< All wheels on ground?
            int com1Active;                     //!< COM1 active [kHz]
            int com1Standby;                    //!< COM1 standby [kHz]
            int com2Active;                     //!< COM2 active [kHz]
            int com2Standby;                    //!< COM2 standby [kHz]
            int xpdrCode;                       //!< Transpondder code
            int xpdrMode;                       //!< Transponder mode (off=0,stdby=1,on=2,test=3)
            bool xpdrIdent;                     //!< Is transponder in ident?
            bool beaconLightsOn;                //!< Beacon lights on?
            bool landingLightsOn;               //!< Landing lights on?
            bool navLightsOn;                   //!< NAV lights on?
            bool strobeLightsOn;                //!< Strobe lights on?
            bool taxiLightsOn;                  //!< Taxi lights on?
            double flapsReployRatio;            //!< Flaps deployment ratio [%]
            double gearReployRatio;             //!< Gear deployment ratio [%]
            QList<double> enginesN1Percentage;  //!< N1 per engine [%]
            double speedBrakeRatio;             //!< Speed break ratio [%]
            double seaLeveLPressure;            //!< Sea level pressure [inhg]
        };

        //! X-Plane ISimulator implementation
        class CSimulatorXPlane : public Common::CSimulatorPluginCommon
        {
            Q_OBJECT

        public:
            //! Constructor
            CSimulatorXPlane(const BlackMisc::Simulation::CSimulatorPluginInfo &info,
                             BlackMisc::Simulation::IOwnAircraftProvider *ownAircraftProvider,
                             BlackMisc::Simulation::IRemoteAircraftProvider *remoteAircraftProvider,
                             BlackMisc::Weather::IWeatherGridProvider *weatherGridProvider,
                             BlackMisc::Network::IClientProvider *clientProvider,
                             QObject *parent = nullptr);

            //! Dtor
            virtual ~CSimulatorXPlane() override;

            //! \name ISimulator implementations
            //! @{
            virtual bool isTimeSynchronized() const override { return false; } // TODO: Can we query the XP intrinisc feature?
            virtual bool connectTo() override;
            virtual bool disconnectFrom() override;
            virtual bool updateOwnSimulatorCockpit(const BlackMisc::Simulation::CSimulatedAircraft &aircraft, const BlackMisc::CIdentifier &originator) override;
            virtual bool updateOwnSimulatorSelcal(const BlackMisc::Aviation::CSelcal &selcal, const BlackMisc::CIdentifier &originator) override;
            virtual void displayStatusMessage(const BlackMisc::CStatusMessage &message) const override;
            virtual void displayTextMessage(const BlackMisc::Network::CTextMessage &message) const override;
            virtual BlackMisc::Aviation::CAirportList getAirportsInRange(bool recalculateDistance) const override;
            virtual bool setTimeSynchronization(bool enable, const BlackMisc::PhysicalQuantities::CTime &offset) override;
            virtual BlackMisc::PhysicalQuantities::CTime getTimeSynchronizationOffset() const override { return BlackMisc::PhysicalQuantities::CTime(0, BlackMisc::PhysicalQuantities::CTimeUnit::hrmin()); }
            virtual bool isPhysicallyRenderedAircraft(const BlackMisc::Aviation::CCallsign &callsign) const override;
            virtual BlackMisc::Aviation::CCallsignSet physicallyRenderedAircraft() const override;
            virtual bool followAircraft(const BlackMisc::Aviation::CCallsign &callsign) override;
            virtual void unload() override;
            virtual QString getStatisticsSimulatorSpecific() const override;
            virtual void resetAircraftStatistics() override;
            //! @}

            //! \copydoc BlackMisc::Simulation::ISimulationEnvironmentProvider::requestElevation
            virtual bool requestElevation(const BlackMisc::Geo::ICoordinateGeodetic &reference, const BlackMisc::Aviation::CCallsign &callsign) override;

            //! Creates an appropriate dbus connection from the string describing it
            static QDBusConnection connectionFromString(const QString &str);

        protected:
            //! \name ISimulator implementations
            //! @{
            virtual bool isConnected() const override;
            virtual bool physicallyAddRemoteAircraft(const BlackMisc::Simulation::CSimulatedAircraft &newRemoteAircraft) override;
            virtual bool physicallyRemoveRemoteAircraft(const BlackMisc::Aviation::CCallsign &callsign) override;
            virtual int physicallyRemoveAllRemoteAircraft() override;
            virtual void clearAllRemoteAircraftData() override;
            virtual void injectWeatherGrid(const BlackMisc::Weather::CWeatherGrid &weatherGrid) override;
            virtual bool isPaused() const override
            {
                //! \todo XP: provide correct pause state
                return false;
            }
            //! @}

        private slots:
            void serviceUnregistered();

        private:
            enum DBusMode
            {
                Session,
                P2P
            };

            using QDoubleList = QList<double>;

            void setAirportsInRange(const QStringList &icaoCodes, const QStringList &names, const BlackMisc::CSequence<double> &lats, const BlackMisc::CSequence<double> &lons, const BlackMisc::CSequence<double> &alts);
            void emitOwnAircraftModelChanged(const QString &path, const QString &filename, const QString &livery, const QString &icao,
                                             const QString &modelString, const QString &name, const QString &description);
            void fastTimerTimeout();
            void slowTimerTimeout();

            void loadCslPackages();
            QString findCslPackage(const QString &modelFileName);

            //! Update remote aircraft
            //! \remark this is where the interpolated data are set
            void updateRemoteAircraft();

            //! Update airports
            void updateAirportsInRange();

            //! Request elevation and CG from XPlane @{
            void requestRemoteAircraftDataFromXPlane();
            void requestRemoteAircraftDataFromXPlane(const BlackMisc::Aviation::CCallsignSet &callsigns);
            void triggerRequestRemoteAircraftDataFromXPlane(const BlackMisc::Aviation::CCallsignSet &callsigns);
            //! @}

            //! Adding new aircraft @{
            void addNextPendingAircraft();
            void triggerAddNextPendingAircraft();
            //! @}

            //! Detect timeouts on adding
            int detectTimeoutAdding();

            //! Trigger a removal of an aircraft
            void triggerRemoveAircraft(const BlackMisc::Aviation::CCallsign &callsign, qint64 deferMs);

            //! Timestamps of aircraft currently adding
            QPair<qint64, qint64> minMaxTimestampsAddInProgress() const;

            //! Can the next aircraft be added?
            bool canAddAircraft() const;

            //! Callbacks from simulator @{
            void onRemoteAircraftAdded(const QString &callsign);
            void onRemoteAircraftAddingFailed(const QString &callsign);
            void updateRemoteAircraftFromSimulator(const QStringList &callsigns, const QDoubleList &latitudesDeg, const QDoubleList &longitudesDeg,
                                                   const QDoubleList &elevationsMeters, const QDoubleList &verticalOffsetsMeters);
            //! @}

            //! Dsiconnect from DBus
            void disconnectFromDBus();

            DBusMode m_dbusMode;
            BlackMisc::CSettingReadOnly<BlackMisc::Simulation::Settings::TXSwiftBusServer> m_xswiftbusServerSetting { this };
            static constexpr qint64 TimeoutAdding = 10000;
            QDBusConnection m_dBusConnection { "default" };
            QDBusServiceWatcher *m_watcher { nullptr };
            CXSwiftBusServiceProxy *m_serviceProxy { nullptr };
            CXSwiftBusTrafficProxy *m_trafficProxy { nullptr };
            CXSwiftBusWeatherProxy *m_weatherProxy { nullptr };
            QTimer m_fastTimer;
            QTimer m_slowTimer;
            QTimer m_airportUpdater;
            QTimer m_pendingAddedTimer;
            BlackMisc::Aviation::CAirportList m_airportsInRange; //!< aiports in range of own aircraft
            BlackMisc::CData<BlackMisc::Simulation::Data::TModelSetCacheXP> m_modelSet { this }; //!< XPlane model set
            BlackMisc::Simulation::CSimulatedAircraftList m_pendingToBeAddedAircraft; //!< aircraft to be added
            QHash<BlackMisc::Aviation::CCallsign, qint64> m_addingInProgressAircraft; //!< aircraft just adding
            BlackMisc::Simulation::CSimulatedAircraftList m_aircraftAddedFailed; //! aircraft for which adding failed
            CXPlaneMPAircraftObjects m_xplaneAircraftObjects; //!< XPlane multiplayer aircraft
            XPlaneData m_xplaneData; //!< XPlane data

            // statistics
            qint64 m_statsAddMaxTimeMs = -1;
            qint64 m_statsAddCurrentTimeMs = -1;

            //! Reset the XPlane data
            void resetXPlaneData()
            {
                m_xplaneData = { "", "", 0, 0, 0, 0, 0, 0, 0, false, 122800, 122800, 122800, 122800, 2000, 0, false, false, false, false,
                                 false, false, 0, 0, {}, 0.0, 0.0
                               };

            }
        };

        //! Listener waits for xswiftbus service to show up
        class CSimulatorXPlaneListener : public BlackCore::ISimulatorListener
        {
            Q_OBJECT

        public:
            //! Constructor
            CSimulatorXPlaneListener(const BlackMisc::Simulation::CSimulatorPluginInfo &info);

        protected:
            //! \copydoc BlackCore::ISimulatorListener::startImpl
            virtual void startImpl() override;

            //! \copydoc BlackCore::ISimulatorListener::stopImpl
            virtual void stopImpl() override;

            //! \copydoc BlackCore::ISimulatorListener::checkImpl
            virtual void checkImpl() override;

        private:
            //! Check if XSwiftBus service is already registered on the bus
            void checkConnectionViaBus(const QString &address);

            //! Check if XSwiftBus service is available via P2P address
            void checkConnectionViaPeer();

            void serviceRegistered(const QString &serviceName);
            void xSwiftBusServerSettingChanged();

            QTimer m_timer { this };
            QDBusConnection m_conn { "default" };
            QDBusServiceWatcher *m_watcher { nullptr };
            BlackMisc::CSettingReadOnly<BlackMisc::Simulation::Settings::TXSwiftBusServer> m_xswiftbusServerSetting { this, &CSimulatorXPlaneListener::xSwiftBusServerSettingChanged };
        };

        //! Factory for creating CSimulatorXPlane instance
        class CSimulatorXPlaneFactory : public QObject, public BlackCore::ISimulatorFactory
        {
            Q_OBJECT
            Q_PLUGIN_METADATA(IID "org.swift-project.blackcore.simulatorinterface" FILE "simulatorxplane.json")
            Q_INTERFACES(BlackCore::ISimulatorFactory)

        public:
            //! \copydoc BlackCore::ISimulatorFactory::create
            virtual BlackCore::ISimulator *create(const BlackMisc::Simulation::CSimulatorPluginInfo &info,
                                                  BlackMisc::Simulation::IOwnAircraftProvider    *ownAircraftProvider,
                                                  BlackMisc::Simulation::IRemoteAircraftProvider *remoteAircraftProvider,
                                                  BlackMisc::Weather::IWeatherGridProvider *weatherGridProvider,
                                                  BlackMisc::Network::IClientProvider *clientProvider) override;

            //! \copydoc BlackCore::ISimulatorFactory::createListener
            virtual BlackCore::ISimulatorListener *createListener(const BlackMisc::Simulation::CSimulatorPluginInfo &info) override { return new CSimulatorXPlaneListener(info); }
        };
    } // ns
} // ns

#endif // guard
