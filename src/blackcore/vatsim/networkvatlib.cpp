/* Copyright (C) 2013
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \cond PRIVATE

#include "blackconfig/buildconfig.h"
#include "blackcore/application.h"
#include "blackcore/vatsim/networkvatlib.h"
#include "blackmisc/simulation/aircraftmodel.h"
#include "blackmisc/simulation/simulatedaircraft.h"
#include "blackmisc/aviation/aircraftsituation.h"
#include "blackmisc/aviation/altitude.h"
#include "blackmisc/aviation/comsystem.h"
#include "blackmisc/aviation/flightplan.h"
#include "blackmisc/aviation/heading.h"
#include "blackmisc/aviation/informationmessage.h"
#include "blackmisc/aviation/livery.h"
#include "blackmisc/aviation/transponder.h"
#include "blackmisc/geo/coordinategeodetic.h"
#include "blackmisc/geo/latitude.h"
#include "blackmisc/geo/longitude.h"
#include "blackmisc/network/network.h"
#include "blackmisc/network/textmessage.h"
#include "blackmisc/network/user.h"
#include "blackmisc/pq/angle.h"
#include "blackmisc/pq/frequency.h"
#include "blackmisc/pq/length.h"
#include "blackmisc/pq/speed.h"
#include "blackmisc/pq/time.h"
#include "blackmisc/pq/units.h"
#include "blackmisc/compare.h"
#include "blackmisc/json.h"
#include "blackmisc/logmessage.h"
#include "blackmisc/logcategory.h"
#include "blackmisc/statusmessage.h"
#include "vatlib/vatlib.h"

#include <QChar>
#include <QCommandLineOption>
#include <QCoreApplication>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonValue>
#include <QList>
#include <QRegExp>
#include <QTextCodec>
#include <QVector>
#include <Qt>
#include <QtDebug>
#include <QtGlobal>
#include <algorithm>
#include <type_traits>

static_assert(! std::is_abstract<BlackCore::Vatsim::CNetworkVatlib>::value, "Must implement all pure virtuals");
static_assert(VAT_LIBVATLIB_VERSION == 906, "Wrong vatlib header installed");

using namespace BlackConfig;
using namespace BlackMisc;
using namespace BlackMisc::Aviation;
using namespace BlackMisc::Geo;
using namespace BlackMisc::Json;
using namespace BlackMisc::Network;
using namespace BlackMisc::PhysicalQuantities;
using namespace BlackMisc::Simulation;

namespace BlackCore
{
    namespace Vatsim
    {
        const CLogCategoryList &CNetworkVatlib::getLogCategories()
        {
            static const CLogCategoryList cats({ CLogCategory::vatsimSpecific(), CLogCategory::network() });
            return cats;
        }

        CNetworkVatlib::CNetworkVatlib(IOwnAircraftProvider *ownAircraft, QObject *parent)
            : INetwork(parent), COwnAircraftAware(ownAircraft),
              m_loginMode(LoginNormal),
              m_status(vatStatusIdle),
              m_tokenBucket(10, CTime(5, CTimeUnit::s()), 1)
        {
            connect(this, &CNetworkVatlib::terminate, this, &INetwork::terminateConnection, Qt::QueuedConnection);

            Q_ASSERT_X(Vat_GetVersion() == VAT_LIBVATLIB_VERSION, "swift.network", "Wrong vatlib shared library installed");

            Vat_SetNetworkLogHandler(SeverityLevel::SeverityError, CNetworkVatlib::networkLogHandler);

            m_processingTimer.setObjectName(this->objectName().append(":m_processingTimer"));
            m_positionUpdateTimer.setObjectName(this->objectName().append(":m_positionUpdateTimer"));
            m_interimPositionUpdateTimer.setObjectName(this->objectName().append(":m_interimPositionUpdateTimer"));

            connect(&m_processingTimer, &QTimer::timeout, this, &CNetworkVatlib::process);
            connect(&m_positionUpdateTimer, &QTimer::timeout, this, &CNetworkVatlib::sendPositionUpdate);
            connect(&m_interimPositionUpdateTimer, &QTimer::timeout, this, &CNetworkVatlib::sendInterimPositions);
            connect(&m_scheduledConfigUpdate, &QTimer::timeout, this, &CNetworkVatlib::sendIncrementalAircraftConfig);
            m_scheduledConfigUpdate.setSingleShot(true);

            m_processingTimer.start(c_processingIntervalMsec);
        }

        void CNetworkVatlib::initializeSession()
        {
            Q_ASSERT_X(isDisconnected(), Q_FUNC_INFO, "attempted to reinitialize session while still connected");

            int clientCapabilities = vatCapsAircraftInfo | vatCapsFastPos | vatCapsAtcInfo | vatCapsAircraftConfig;
            if (m_loginMode == LoginStealth)
            {
                clientCapabilities |= vatCapsStealth;
            }

            int clientId = 0;
            QString clientKey;
            if (!getCmdLineClientIdAndKey(clientId, clientKey))
            {
                clientId = CBuildConfig::vatsimClientId();
                clientKey = CBuildConfig::vatsimPrivateKey();
            }

            VatServerType serverType;
            bool success = getCmdLineServerType(serverType);
            if (!success) { serverType = CBuildConfig::isVatsimVersion() ? vatServerVatsim : vatServerLegacyFsd; }

            m_net.reset(Vat_CreateNetworkSession(serverType, sApp->swiftVersionChar(),
                                                 CBuildConfig::getVersion().majorVersion(), CBuildConfig::getVersion().minorVersion(),
                                                 "None", clientId, clientKey.toLocal8Bit().constData(),
                                                 clientCapabilities));

            Vat_SetStateChangeHandler(m_net.data(), onConnectionStatusChanged, this);
            Vat_SetTextMessageHandler(m_net.data(), onTextMessageReceived, this);
            Vat_SetRadioMessageHandler(m_net.data(), onRadioMessageReceived, this);
            Vat_SetDeletePilotHandler(m_net.data(), onPilotDisconnected, this);
            Vat_SetDeleteAtcHandler(m_net.data(), onControllerDisconnected, this);
            Vat_SetPilotPositionHandler(m_net.data(), onPilotPositionUpdate, this);
            Vat_SetInterimPilotPositionHandler(m_net.data(), onInterimPilotPositionUpdate, this);
            Vat_SetAtcPositionHandler(m_net.data(), onAtcPositionUpdate, this);
            Vat_SetKillHandler(m_net.data(), onKicked, this);
            Vat_SetPongHandler(m_net.data(), onPong, this);
            Vat_SetMetarResponseHandler(m_net.data(), onMetarReceived, this);
            Vat_SetClientQueryHandler(m_net.data(), onInfoQueryRequestReceived, this);
            Vat_SetClientQueryResponseHandler(m_net.data(), onInfoQueryReplyReceived, this);
            Vat_SetInfoCAPSReplyHandler(m_net.data(), onCapabilitiesReplyReceived, this);
            Vat_SetControllerAtisHandler(m_net.data(), onAtisReplyReceived, this);
            Vat_SetFlightPlanHandler(m_net.data(), onFlightPlanReceived, this);
            Vat_SetServerErrorHandler(m_net.data(), onErrorReceived, this);
            Vat_SetAircraftInfoRequestHandler(m_net.data(), onPilotInfoRequestReceived, this);
            Vat_SetAircraftInfoHandler(m_net.data(), onPilotInfoReceived, this);
            Vat_SetCustomPilotPacketHandler(m_net.data(), onCustomPacketReceived, this);
            Vat_SetAircraftConfigHandler(m_net.data(), onAircraftConfigReceived, this);
        }

        CNetworkVatlib::~CNetworkVatlib()
        {
            Q_ASSERT_X(isDisconnected(), Q_FUNC_INFO, "CNetworkVatlib destroyed while still connected.");
        }

        void CNetworkVatlib::process()
        {
            if (!m_net) { return; }
            sendIncrementalAircraftConfig();
            Vat_ExecuteNetworkTasks(m_net.data());
        }

        void CNetworkVatlib::sendPositionUpdate()
        {
            if (!m_net) { return; }

            if (isConnected())
            {
                CSimulatedAircraft myAircraft(getOwnAircraft());
                if (this->m_loginMode == LoginAsObserver)
                {
                    // Observer
                    VatAtcPosition pos;
                    pos.facility = vatFacilityTypeUnknown;
                    pos.visibleRange = 10; // NM
                    pos.latitude  = myAircraft.latitude().value(CAngleUnit::deg());
                    pos.longitude = myAircraft.longitude().value(CAngleUnit::deg());
                    pos.elevation = 0;
                    pos.rating = vatAtcRatingObserver;
                    pos.frequency = 199998;
                    Vat_SendATCUpdate(m_net.data(), &pos);
                }
                else
                {
                    // Normal / Stealth mode
                    VatPilotPosition pos;
                    //! \fixme we need to distinguish true and pressure altitude
                    pos.altitudePressure = myAircraft.getAltitude().value(CLengthUnit::ft());
                    pos.altitudeTrue = myAircraft.getAltitude().value(CLengthUnit::ft());
                    pos.heading      = myAircraft.getHeading().value(CAngleUnit::deg());
                    pos.pitch        = myAircraft.getPitch().value(CAngleUnit::deg());
                    pos.bank         = myAircraft.getBank().value(CAngleUnit::deg());
                    pos.latitude     = myAircraft.latitude().value(CAngleUnit::deg());
                    pos.longitude    = myAircraft.longitude().value(CAngleUnit::deg());
                    pos.groundSpeed  = myAircraft.getGroundSpeed().value(CSpeedUnit::kts());
                    pos.rating = vatPilotRatingUnknown;
                    pos.transponderCode = static_cast<qint16>(myAircraft.getTransponderCode());
                    pos.transponderMode = vatTransponderModeStandby;
                    switch (myAircraft.getTransponderMode())
                    {
                    case CTransponder::ModeC: pos.transponderMode = vatTransponderModeCharlie; break;
                    case CTransponder::StateIdent: pos.transponderMode = vatTransponderModeIdent; break;
                    default: pos.transponderMode = vatTransponderModeStandby; break;
                    }
                    Vat_SendPilotUpdate(m_net.data(), &pos);
                }
            }
        }

        void CNetworkVatlib::sendInterimPositions()
        {
            if (!m_net) { return; }

            if (isConnected())
            {
                CSimulatedAircraft myAircraft(getOwnAircraft());
                if (this->m_loginMode == LoginNormal)
                {
                    VatInterimPilotPosition pos;
                    pos.altitudeTrue = myAircraft.getAltitude().value(CLengthUnit::ft());
                    pos.heading      = myAircraft.getHeading().value(CAngleUnit::deg());
                    pos.pitch        = myAircraft.getPitch().value(CAngleUnit::deg());
                    pos.bank         = myAircraft.getBank().value(CAngleUnit::deg());
                    pos.latitude     = myAircraft.latitude().value(CAngleUnit::deg());
                    pos.longitude    = myAircraft.longitude().value(CAngleUnit::deg());

                    for (const auto &receiver : as_const(m_interimPositionReceivers))
                    {
                        Vat_SendInterimPilotUpdate(m_net.data(), toFSD(receiver), &pos);
                    }
                }
            }
        }

        //! Convert vatlib status code to INetwork::ConnectionStatus
        INetwork::ConnectionStatus convertConnectionStatus(VatConnectionStatus status)
        {
            switch (status)
            {
            case vatStatusIdle:              return INetwork::Disconnected;
            case vatStatusConnecting:        return INetwork::Connecting;
            case vatStatusConnected:         return INetwork::Connected;
            case vatStatusDisconnected:      return INetwork::Disconnected;
            case vatStatusDisconnecting:     return INetwork::Disconnecting;
            case vatStatusError:             return INetwork::DisconnectedError;
            }
            qFatal("unrecognised connection status");
            return INetwork::DisconnectedError;
        }

        void CNetworkVatlib::changeConnectionStatus(VatConnectionStatus status)
        {
            if (m_status != status)
            {
                qSwap(m_status, status);
                if (m_status == vatStatusConnected)
                {
                    m_server.setConnectedSinceNow();
                }
                else
                {
                    m_server.markAsDisconnected();
                }

                if (isDisconnected())
                {
                    stopPositionTimers();
                }

                emit connectionStatusChanged(convertConnectionStatus(status), convertConnectionStatus(m_status));
            }
        }

        QByteArray CNetworkVatlib::toFSD(const QString &qstr) const
        {
            Q_ASSERT_X(m_fsdTextCodec, Q_FUNC_INFO, "Missing codec");
            return m_fsdTextCodec->fromUnicode(qstr);
        }

        QByteArray CNetworkVatlib::toFSD(const BlackMisc::Aviation::CCallsign &callsign) const
        {
            return toFSD(callsign.getStringAsSet());
        }

        std::function<const char **()> CNetworkVatlib::toFSD(const QStringList &qstrList) const
        {
            QVector<QByteArray> bytesVec;
            for (auto i = qstrList.cbegin(); i != qstrList.cend(); ++i)
            {
                bytesVec.push_back(toFSD(*i));
            }

            return [ cstrVec = QVector<const char *>(), bytesVec = std::move(bytesVec) ]() mutable
            {
                Q_ASSERT(cstrVec.isEmpty());
                for (auto i = bytesVec.cbegin(); i != bytesVec.cend(); ++i)
                {
                    cstrVec.push_back(i->constData());
                }
                return const_cast<const char **>(cstrVec.constData());
            };
        }

        QString CNetworkVatlib::fromFSD(const char *cstr) const
        {
            Q_ASSERT_X(m_fsdTextCodec, Q_FUNC_INFO, "Missing codec");
            return m_fsdTextCodec->toUnicode(cstr);
        }

        QStringList CNetworkVatlib::fromFSD(const char **cstrArray, int size) const
        {
            QStringList qstrList;
            for (int i = 0; i < size; ++i)
            {
                qstrList.push_back(fromFSD(cstrArray[i]));
            }
            return qstrList;
        }

        bool CNetworkVatlib::isInterimPositionUpdateEnabledForServer() const
        {
            const CFsdSetup::SendReceiveDetails d = m_server.getFsdSetup().getSendReceiveDetails();
            return (d & CFsdSetup::SendIterimPositions) || (d & CFsdSetup::ReceiveInterimPositions);
        }

        void CNetworkVatlib::startPositionTimers()
        {
            m_positionUpdateTimer.start(c_updatePostionIntervalMsec);
            if (isInterimPositionUpdateEnabledForServer()) { m_interimPositionUpdateTimer.start(c_updateInterimPostionIntervalMsec); }
        }

        void CNetworkVatlib::stopPositionTimers()
        {
            m_positionUpdateTimer.stop();
            m_interimPositionUpdateTimer.stop();
        }

        QString CNetworkVatlib::convertToUnicodeEscaped(const QString &str)
        {
            QString escaped;
            for (const auto &ch : str)
            {
                ushort code = ch.unicode();
                if (code < 0x80)
                {
                    escaped += ch;
                }
                else
                {
                    escaped += "\\u";
                    escaped += QString::number(code, 16).rightJustified(4, '0');
                }
            }
            return escaped;
        }

        VatSimType CNetworkVatlib::convertToSimType(CSimulatorPluginInfo &simInfo)
        {
            //! \fixme Define recognized simulators somewhere */
            if (simInfo.getSimulator() == "fs9" || simInfo.getSimulator() == "fsx")
            {
                return vatSimTypeMSCFS;
            }
            else if (simInfo.getSimulator() == "xplane")
            {
                return vatSimTypeXPLANE;
            }
            else
            {
                return vatSimTypeUnknown;
            }
        }

        /********************************** * * * * * * * * * * * * * * * * * * * ************************************/
        /**********************************             INetwork functions        ************************************/
        /********************************** * * * * * * * * * * * * * * * * * * * ************************************/

        void CNetworkVatlib::presetServer(const CServer &server)
        {
            Q_ASSERT_X(isDisconnected(), Q_FUNC_INFO, "Can't change server details while still connected");
            m_server = server;
            const QString codecName(server.getFsdSetup().getTextCodec());
            Q_ASSERT_X(!codecName.isEmpty(), Q_FUNC_INFO, "Missing code name");
            this->m_fsdTextCodec = QTextCodec::codecForName(codecName.toLocal8Bit());
            if (!this->m_fsdTextCodec) { this->m_fsdTextCodec = QTextCodec::codecForName("latin1"); }
        }

        void CNetworkVatlib::presetSimulatorInfo(const CSimulatorPluginInfo &simInfo)
        {
            Q_ASSERT_X(isDisconnected(), Q_FUNC_INFO, "Can't change server details while still connected");
            m_simulatorInfo = simInfo;
        }

        void CNetworkVatlib::presetCallsign(const BlackMisc::Aviation::CCallsign &callsign)
        {
            Q_ASSERT_X(isDisconnected(), Q_FUNC_INFO, "Can't change callsign while still connected");
            m_ownCallsign = callsign;
            updateOwnCallsign(callsign);
        }

        void CNetworkVatlib::presetIcaoCodes(const BlackMisc::Simulation::CSimulatedAircraft &ownAircraft)
        {
            Q_ASSERT_X(isDisconnected(), Q_FUNC_INFO, "Can't change ICAO codes while still connected");
            m_ownAircraftIcaoCode = ownAircraft.getAircraftIcaoCode();
            m_ownAirlineIcaoCode = ownAircraft.getAirlineIcaoCode();
            m_ownLiveryDescription = ownAircraft.getLivery().getDescription();
            updateOwnIcaoCodes(m_ownAircraftIcaoCode, m_ownAirlineIcaoCode);
        }

        void CNetworkVatlib::presetLoginMode(LoginMode mode)
        {
            Q_ASSERT_X(isDisconnected(), Q_FUNC_INFO, "Can't change login mode while still connected");
            m_loginMode = mode;
            m_net.reset(nullptr);
        }

        void CNetworkVatlib::initiateConnection()
        {
            Q_ASSERT_X(isDisconnected(), Q_FUNC_INFO, "Can't connect while still connected");
            if (!m_net) { initializeSession(); }
            QByteArray callsign = toFSD(m_loginMode == LoginAsObserver ?
                                        m_ownCallsign.getAsObserverCallsignString() :
                                        m_ownCallsign.asString());
            QByteArray name = toFSD(m_server.getUser().getRealName());

            if (this->m_loginMode == LoginAsObserver)
            {
                // Observer mode
                VatAtcConnection info;
                info.name = name.data();
                info.rating = vatAtcRatingObserver;
                info.callsign = callsign.data();
                Vat_SpecifyATCLogon(m_net.data(), toFSD(m_server.getAddress()), m_server.getPort(),
                                    toFSD(m_server.getUser().getId()),
                                    toFSD(m_server.getUser().getPassword()),
                                    &info);
            }
            else
            {
                // normal scenario, also used in STEALTH
                VatPilotConnection info;
                info.callsign = callsign.data();
                info.name = name.data();
                info.rating = vatPilotRatingStudent; // as documented, expected to be vatPilotRatingStudent only
                info.simType = convertToSimType(m_simulatorInfo);
                Vat_SpecifyPilotLogon(m_net.data(), toFSD(m_server.getAddress()), m_server.getPort(),
                                      toFSD(m_server.getUser().getId()),
                                      toFSD(m_server.getUser().getPassword()),
                                      &info);
            }

            Vat_Logon(m_net.data());

            startPositionTimers();
        }

        void CNetworkVatlib::terminateConnection()
        {
            stopPositionTimers();
            if (m_net && isConnected())
            {
                // Process all pending tasks before logging off
                process();
                Vat_Logoff(m_net.data());
            }
        }

        void CNetworkVatlib::sendTextMessages(const BlackMisc::Network::CTextMessageList &messages)
        {
            Q_ASSERT_X(isConnected(), Q_FUNC_INFO, "Can't send to server when disconnected");

            if (messages.isEmpty()) { return; }
            CTextMessageList privateMessages = messages.getPrivateMessages();
            privateMessages.markAsSent();
            for (const auto &message : as_const(privateMessages))
            {
                if (message.getRecipientCallsign().isEmpty()) { continue; }
                Vat_SendTextMessage(m_net.data(), toFSD(message.getRecipientCallsign()), toFSD(message.getMessage()));
                emit textMessageSent(message);
            }

            CTextMessageList radioMessages = messages.getRadioMessages();
            radioMessages.markAsSent();
            QVector<int> freqsVec;
            for (const auto &message : radioMessages)
            {
                // I could send the same message to n frequencies in one step
                // if this is really required, I need to group by message
                // currently I send individual messages
                freqsVec.clear();
                freqsVec.push_back(message.getFrequency().valueRounded(CFrequencyUnit::kHz(), 0));
                Vat_SendRadioMessage(m_net.data(), freqsVec.data(), freqsVec.size(), toFSD(message.getMessage()));
                emit textMessageSent(message);
            }
        }

        void CNetworkVatlib::sendCustomPacket(const BlackMisc::Aviation::CCallsign &callsign, const QString &packetId, const QStringList &data)
        {
            Q_ASSERT_X(isConnected(), Q_FUNC_INFO, "Can't send to server when disconnected");
            Vat_SendCustomPilotPacket(m_net.data(), toFSD(callsign), toFSD(packetId), toFSD(data)(), data.size());
        }

        void CNetworkVatlib::sendIpQuery()
        {
            Q_ASSERT_X(isConnected(), Q_FUNC_INFO, "Can't send to server when disconnected");
            Vat_SendClientQuery(m_net.data(), vatClientQueryIP, nullptr);
        }

        void CNetworkVatlib::sendFrequencyQuery(const BlackMisc::Aviation::CCallsign &callsign)
        {
            Q_ASSERT_X(isConnected(), Q_FUNC_INFO, "Can't send to server when disconnected");
            Vat_SendClientQuery(m_net.data(), vatClientQueryFreq, toFSD(callsign));
        }

        void CNetworkVatlib::sendUserInfoQuery(const BlackMisc::Aviation::CCallsign &callsign)
        {
            Q_ASSERT_X(isConnected(), Q_FUNC_INFO, "Can't send to server when disconnected");
            Vat_SendClientQuery(m_net.data(), vatClientQueryInfo, toFSD(callsign));
        }

        void CNetworkVatlib::setInterimPositionReceivers(const CCallsignSet &receivers)
        {
            m_interimPositionReceivers = receivers;
        }

        const CCallsignSet &CNetworkVatlib::getInterimPositionReceivers() const
        {
            return m_interimPositionReceivers;
        }

        void CNetworkVatlib::sendServerQuery(const BlackMisc::Aviation::CCallsign &callsign)
        {
            Q_ASSERT_X(isConnected(), Q_FUNC_INFO, "Can't send to server when disconnected");
            Vat_SendClientQuery(m_net.data(), vatClientQueryServer, toFSD(callsign));
        }

        void CNetworkVatlib::sendAtcQuery(const BlackMisc::Aviation::CCallsign &callsign)
        {
            Q_ASSERT_X(isConnected(), Q_FUNC_INFO, "Can't send to server when disconnected");
            Vat_SendClientQuery(m_net.data(), vatClientQueryAtc, toFSD(callsign));
        }

        void CNetworkVatlib::sendAtisQuery(const BlackMisc::Aviation::CCallsign &callsign)
        {
            Q_ASSERT_X(isConnected(), Q_FUNC_INFO, "Can't send to server when disconnected");
            Vat_SendClientQuery(m_net.data(), vatClientQueryAtis, toFSD(callsign));
        }

        void CNetworkVatlib::sendFlightPlan(const CFlightPlan &flightPlan)
        {
            Q_ASSERT_X(isConnected(), Q_FUNC_INFO, "Can't send to server when disconnected");

            VatFlightPlan vatlibFP;
            QString route = QString(flightPlan.getRoute()).replace(" ", ".");
            QString remarks = QString(flightPlan.getRemarks()).replace(":", ";").trimmed();
            QString alt = flightPlan.getCruiseAltitude().isFlightLevel() ?
                          flightPlan.getCruiseAltitude().toQString() :
                          flightPlan.getCruiseAltitude().valueRoundedWithUnit(0);
            alt = alt.remove('.').remove(','); // remove any separators

            QByteArray acTypeTemp, altAptTemp, cruiseAltTemp, depAptTemp, destAptTemp, routeTemp, remarksTemp;
            vatlibFP.aircraftType = acTypeTemp = toFSD(flightPlan.getEquipmentIcao());
            vatlibFP.alternateAirport = altAptTemp = toFSD(flightPlan.getAlternateAirportIcao().asString());
            vatlibFP.cruiseAltitude = cruiseAltTemp = toFSD(alt);
            vatlibFP.departAirport = depAptTemp = toFSD(flightPlan.getOriginAirportIcao().asString());
            vatlibFP.departTimeActual = flightPlan.getTakeoffTimeActual().toUTC().toString("hhmm").toInt();
            vatlibFP.departTime = flightPlan.getTakeoffTimePlanned().toUTC().toString("hhmm").toInt();
            vatlibFP.destAirport = destAptTemp = toFSD(flightPlan.getDestinationAirportIcao().asString());

            QList<int> timeParts = flightPlan.getEnrouteTime().getHrsMinSecParts();
            vatlibFP.enrouteHrs = timeParts[CTime::Hours];
            vatlibFP.enrouteMins = timeParts[CTime::Minutes];

            timeParts = flightPlan.getFuelTime().getHrsMinSecParts();
            vatlibFP.fuelHrs = timeParts[CTime::Hours];
            vatlibFP.fuelMins = timeParts[CTime::Minutes];
            vatlibFP.remarks = remarksTemp = toFSD(remarks);
            vatlibFP.route = routeTemp = toFSD(route);
            vatlibFP.trueCruisingSpeed = flightPlan.getCruiseTrueAirspeed().valueRounded(CSpeedUnit::kts());
            switch (flightPlan.getFlightRules())
            {
            default:
            case CFlightPlan::IFR:  vatlibFP.flightType = vatFlightTypeIFR; break;
            case CFlightPlan::VFR:  vatlibFP.flightType = vatFlightTypeVFR; break;
            case CFlightPlan::SVFR: vatlibFP.flightType = vatFlightTypeSVFR; break;
            case CFlightPlan::DVFR: vatlibFP.flightType = vatFlightTypeDVFR; break;
            }
            Vat_SendFlightPlan(m_net.data(), &vatlibFP);
        }

        void CNetworkVatlib::sendFlightPlanQuery(const BlackMisc::Aviation::CCallsign &callsign)
        {
            Q_ASSERT_X(isConnected(), Q_FUNC_INFO, "Can't send to server when disconnected");
            Vat_SendClientQuery(m_net.data(), vatClientQueryFP, toFSD(callsign));
        }

        void CNetworkVatlib::sendRealNameQuery(const BlackMisc::Aviation::CCallsign &callsign)
        {
            Q_ASSERT_X(isConnected(), Q_FUNC_INFO, "Can't send to server when disconnected");
            Vat_SendClientQuery(m_net.data(), vatClientQueryName, toFSD(callsign));
        }

        void CNetworkVatlib::sendCapabilitiesQuery(const BlackMisc::Aviation::CCallsign &callsign)
        {
            Q_ASSERT_X(isConnected(), Q_FUNC_INFO, "Can't send to server when disconnected");
            Vat_SendClientQuery(m_net.data(), vatClientQueryCaps, toFSD(callsign));
        }

        void CNetworkVatlib::replyToFrequencyQuery(const BlackMisc::Aviation::CCallsign &callsign) // private
        {
            QStringList response { QString::number(getOwnAircraft().getCom1System().getFrequencyActive().value(CFrequencyUnit::MHz()), 'f', 3)};
            Vat_SendClientQueryResponse(m_net.data(), vatClientQueryFreq, toFSD(callsign), toFSD(response)(), response.size());
        }

        void CNetworkVatlib::replyToNameQuery(const BlackMisc::Aviation::CCallsign &callsign) // private
        {
            QStringList response { m_server.getUser().getRealName(), "" };
            Vat_SendClientQueryResponse(m_net.data(), vatClientQueryName, toFSD(callsign), toFSD(response)(), response.size());
        }

        void CNetworkVatlib::replyToConfigQuery(const CCallsign &callsign)
        {
            QJsonObject config = getOwnAircraftParts().toJson();
            config.insert("is_full_data", true);
            QString data = QJsonDocument(QJsonObject { { "config", config } }).toJson(QJsonDocument::Compact);
            data = convertToUnicodeEscaped(data);
            Vat_SendAircraftConfig(m_net.data(), toFSD(callsign), toFSD(data));
        }

        void CNetworkVatlib::sendIcaoCodesQuery(const BlackMisc::Aviation::CCallsign &callsign)
        {
            Q_ASSERT_X(isConnected(), Q_FUNC_INFO, "Can't send to server when disconnected");
            Vat_RequestAircraftInfo(m_net.data(), toFSD(callsign));
        }

        void CNetworkVatlib::sendAircraftInfo(const BlackMisc::Aviation::CCallsign &callsign) // private
        {
            const QByteArray acTypeICAObytes = toFSD(m_ownAircraftIcaoCode.getDesignator());
            const QByteArray airlineICAObytes = toFSD(m_ownAirlineIcaoCode.getDesignator());
            const QByteArray liverybytes = toFSD(m_ownLiveryDescription);

            VatAircraftInfo aircraftInfo {acTypeICAObytes, airlineICAObytes, liverybytes};
            Vat_SendAircraftInfo(m_net.data(), toFSD(callsign), &aircraftInfo);
        }

        void CNetworkVatlib::sendIncrementalAircraftConfig()
        {
            if (!isConnected()) { return; }
            const CAircraftParts currentParts(getOwnAircraftParts());

            // If it hasn't changed, return
            if (m_sentAircraftConfig == currentParts) { return; }

            if (!m_tokenBucket.tryConsume())
            {
                // If timer is not yet active, start it
                if (!m_scheduledConfigUpdate.isActive()) m_scheduledConfigUpdate.start(1000);
                return;
            }

            // Method could have been triggered by another change in aircraft config
            // so a previous update might still be scheduled. Stop it.
            if (m_scheduledConfigUpdate.isActive()) m_scheduledConfigUpdate.stop();
            const QJsonObject previousConfig = m_sentAircraftConfig.toJson();
            const QJsonObject currentConfig = currentParts.toJson();
            const QJsonObject incrementalConfig = getIncrementalObject(previousConfig, currentConfig);
            broadcastAircraftConfig(incrementalConfig);
            m_sentAircraftConfig = currentParts;
        }

        void CNetworkVatlib::sendPing(const BlackMisc::Aviation::CCallsign &callsign)
        {
            Q_ASSERT_X(isConnected(), Q_FUNC_INFO, "Can't send to server when disconnected");
            Vat_SendPing(m_net.data(), toFSD(callsign));
        }

        void CNetworkVatlib::sendMetarQuery(const BlackMisc::Aviation::CAirportIcaoCode &airportIcao)
        {
            Q_ASSERT_X(isConnected(), Q_FUNC_INFO, "Can't send to server when disconnected");
            Vat_RequestMetar(m_net.data(), toFSD(airportIcao.asString()));
        }

        const QList<QCommandLineOption> &CNetworkVatlib::getCmdLineOptions()
        {
            static const QList<QCommandLineOption> e;
            static const QList<QCommandLineOption> opts
            {
                QCommandLineOption({ "idAndKey", "clientIdAndKey" },
                QCoreApplication::translate("networkvatlib", "Client id and key pair separated by ':', e.g. <id>:<key>."), "clientIdAndKey"),
                QCommandLineOption({ "s", "serverType" },
                QCoreApplication::translate("networkvatlib", "FSD server type. Possible values: vatsim, fsd"), "serverType")
            };

            // only in not officially shipped versions
            return (CBuildConfig::isStableBranch() && !CBuildConfig::isDevBranch()) ? e : opts;
        }

        bool CNetworkVatlib::getCmdLineClientIdAndKey(int &id, QString &key) const
        {
            QString clientIdAndKey = sApp->getParserValue("clientIdAndKey").toLower();
            if (clientIdAndKey.isEmpty() || !clientIdAndKey.contains(':')) { return false; }
            const auto stringList = clientIdAndKey.split(':');
            QString clientIdAsString = stringList[0];
            bool ok = true;
            id = clientIdAsString.toInt(&ok, 0);
            if (!ok) { return false; }
            key = stringList[1];
            return true;
        }

        bool CNetworkVatlib::getCmdLineServerType(VatServerType &serverType) const
        {
            QString serverTypeAsString = sApp->getParserValue("serverType").toLower();
            if (QString::compare(serverTypeAsString, "vatsim", Qt::CaseInsensitive) == 0)
            {
                serverType = vatServerVatsim;
                return true;
            }

            if (QString::compare(serverTypeAsString, "fsd", Qt::CaseInsensitive) == 0)
            {
                serverType = vatServerLegacyFsd;
                return true;
            }
            return false;
        }

        void CNetworkVatlib::sendCustomFsinnQuery(const BlackMisc::Aviation::CCallsign &callsign)
        {
            Q_ASSERT_X(isConnected(), Q_FUNC_INFO, "Can't send to server when disconnected");
            CSimulatedAircraft myAircraft(getOwnAircraft());
            QString modelString = myAircraft.getModel().getModelString();
            if (modelString.isEmpty()) { modelString = defaultModelString(); }

            const QStringList data { { "0" },
                myAircraft.getAirlineIcaoCodeDesignator(),
                myAircraft.getAircraftIcaoCodeDesignator(),
                { "" }, { "" }, { "" }, { "" },
                myAircraft.getAircraftIcaoCombinedType(),
                modelString
            };
            sendCustomPacket(callsign, "FSIPIR", data);
        }

        void CNetworkVatlib::sendCustomFsinnReponse(const BlackMisc::Aviation::CCallsign &callsign)
        {
            Q_ASSERT_X(isConnected(), Q_FUNC_INFO, "Can't send to server when disconnected");
            CSimulatedAircraft myAircraft(getOwnAircraft());
            QString modelString = myAircraft.getModel().getModelString();
            if (modelString.isEmpty()) { modelString = defaultModelString(); }

            const QStringList data { { "0" },
                myAircraft.getAirlineIcaoCodeDesignator(),
                myAircraft.getAircraftIcaoCodeDesignator(),
                { "" }, { "" }, { "" }, { "" },
                myAircraft.getAircraftIcaoCombinedType(),
                modelString
            };
            sendCustomPacket(callsign, "FSIPI", data);
        }

        void CNetworkVatlib::broadcastAircraftConfig(const QJsonObject &config)
        {
            QString data = QJsonDocument(QJsonObject { { "config", config } }).toJson(QJsonDocument::Compact);
            data = convertToUnicodeEscaped(data);
            Vat_SendAircraftConfigBroadcast(m_net.data(), toFSD(data));
        }

        void CNetworkVatlib::sendAircraftConfigQuery(const CCallsign &callsign)
        {
            const QJsonDocument doc(JsonPackets::aircraftConfigRequest());
            const QString data { doc.toJson(QJsonDocument::Compact) };
            Vat_SendAircraftConfig(m_net.data(), toFSD(callsign), toFSD(data));
        }

        /********************************** * * * * * * * * * * * * * * * * * * * ************************************/
        /**********************************           shimlib callbacks           ************************************/
        /********************************** * * * * * * * * * * * * * * * * * * * ************************************/

        // Cast void* to a pointer of CNetworkVatlib
        CNetworkVatlib *cbvar_cast(void *cbvar)
        {
            return static_cast<CNetworkVatlib *>(cbvar);
        }

        void CNetworkVatlib::onConnectionStatusChanged(VatSessionID, VatConnectionStatus, VatConnectionStatus newStatus, void *cbvar)
        {
            if (newStatus == vatStatusError)
            {
                cbvar_cast(cbvar)->changeConnectionStatus(newStatus);
            }
            else
            {
                cbvar_cast(cbvar)->changeConnectionStatus(newStatus);
            }
        }

        void CNetworkVatlib::onTextMessageReceived(VatSessionID, const char *from, const char *to, const char *msg, void *cbvar)
        {
            CTextMessage tm(cbvar_cast(cbvar)->fromFSD(msg), CCallsign(cbvar_cast(cbvar)->fromFSD(from)), CCallsign(cbvar_cast(cbvar)->fromFSD(to)));
            tm.setCurrentUtcTime();
            CTextMessageList messages(tm);
            emit cbvar_cast(cbvar)->textMessagesReceived(messages);
        }

        void CNetworkVatlib::onRadioMessageReceived(VatSessionID, const char *from, int numFreq, int *freqList, const char *msg, void *cbvar)
        {
            int com1 = cbvar_cast(cbvar)->getOwnAircraft().getCom1System().getFrequencyActive().valueInteger(CFrequencyUnit::kHz());
            int com2 = cbvar_cast(cbvar)->getOwnAircraft().getCom2System().getFrequencyActive().valueInteger(CFrequencyUnit::kHz());
            QList<CFrequency> frequencies;
            for (int i = 0; i < numFreq; ++i)
            {
                if (freqList[i] == com1 || freqList[i] == com2)
                {
                    frequencies.push_back(CFrequency(freqList[i], CFrequencyUnit::kHz()));
                }
            }
            if (frequencies.isEmpty()) { return; }
            BlackMisc::Network::CTextMessageList messages(cbvar_cast(cbvar)->fromFSD(msg), frequencies, CCallsign(cbvar_cast(cbvar)->fromFSD(from)));
            messages.setCurrentUtcTime();
            emit cbvar_cast(cbvar)->textMessagesReceived(messages);
        }

        void CNetworkVatlib::onPilotDisconnected(VatSessionID, const char *callsign, void *cbvar)
        {
            emit cbvar_cast(cbvar)->pilotDisconnected(CCallsign(cbvar_cast(cbvar)->fromFSD(callsign), CCallsign::Aircraft));
        }

        void CNetworkVatlib::onControllerDisconnected(VatSessionID, const char *callsign, void *cbvar)
        {
            emit cbvar_cast(cbvar)->atcDisconnected(CCallsign(cbvar_cast(cbvar)->fromFSD(callsign), CCallsign::Atc));
        }

        void CNetworkVatlib::onPilotPositionUpdate(VatSessionID, const char *callsignChar , const VatPilotPosition *position, void *cbvar)
        {
            const CCallsign callsign(callsignChar, CCallsign::Aircraft);
            CAircraftSituation situation(
                callsign,
                CCoordinateGeodetic(position->latitude, position->longitude, position->altitudeTrue),
                CHeading(position->heading, CHeading::True, CAngleUnit::deg()),
                CAngle(position->pitch, CAngleUnit::deg()),
                CAngle(position->bank, CAngleUnit::deg()),
                CSpeed(position->groundSpeed, CSpeedUnit::kts()),
                CAltitude({ 0, nullptr }, CAltitude::MeanSeaLevel)
            );
            situation.setCurrentUtcTime();
            situation.setTimeOffsetMs(c_positionTimeOffsetMsec);

            QString transponderName("transponder ");
            transponderName.append(callsign.asString());
            CTransponder::TransponderMode mode = CTransponder::StateStandby;
            switch (position->transponderMode)
            {
            case vatTransponderModeCharlie:
                mode = CTransponder::ModeC;
                break;
            case vatTransponderModeStandby:
                mode = CTransponder::StateStandby;
                break;
            case vatTransponderModeIdent:
                mode = CTransponder::StateIdent;
                break;
            default:
                mode = CTransponder::StateStandby;
                break;
            }

            // I did have a situation where I got wrong transponder codes (KB)
            // So I now check for a valid code in order to detect such codes
            CTransponder transponder;
            if (CTransponder::isValidTransponderCode(position->transponderCode))
            {
                transponder = CTransponder(position->transponderCode, mode);
            }
            else
            {
                CLogMessage(static_cast<CNetworkVatlib *>(nullptr)).warning("Wrong transponder code %1 for %2") << position->transponderCode << callsign;

                // default
                transponder = CTransponder(7000, mode);
            }
            emit cbvar_cast(cbvar)->aircraftPositionUpdate(situation, transponder);
        }

        void CNetworkVatlib::onAircraftConfigReceived(VatSessionID, const char *callsign, const char *aircraftConfig, void *cbvar)
        {
            QByteArray json = cbvar_cast(cbvar)->fromFSD(aircraftConfig).toUtf8();
            QJsonParseError parserError;
            QJsonDocument doc = QJsonDocument::fromJson(json, &parserError);
            CCallsign cs(callsign, CCallsign::Aircraft);

            if (parserError.error != QJsonParseError::NoError)
            {
                CLogMessage(static_cast<CNetworkVatlib *>(nullptr)).warning("Failed to parse aircraft config packet: %1") << parserError.errorString();
            }

            QJsonObject packet = doc.object();

            if (packet == JsonPackets::aircraftConfigRequest())
            {
                cbvar_cast(cbvar)->replyToConfigQuery(cbvar_cast(cbvar)->fromFSD(callsign));
                return;
            }

            QJsonObject config = doc.object().value("config").toObject();
            if (config.empty()) return;

            bool isFull = config.take("is_full_data").toBool(false);
            emit cbvar_cast(cbvar)->aircraftConfigPacketReceived(cs, config, isFull);
        }

        void CNetworkVatlib::onInterimPilotPositionUpdate(VatSessionID, const char *sender, const VatInterimPilotPosition *position, void *cbvar)
        {
            CAircraftSituation situation(
                CCallsign(sender),
                CCoordinateGeodetic(position->latitude, position->longitude, position->altitudeTrue),
                CHeading(position->heading, CHeading::True, CAngleUnit::deg()),
                CAngle(position->pitch, CAngleUnit::deg()),
                CAngle(position->bank, CAngleUnit::deg()),
                CSpeed(0.0, nullptr), // There is no speed information in a interim packet
                CAltitude({ 0, nullptr }, CAltitude::MeanSeaLevel)
            );
            situation.setCurrentUtcTime();
            situation.setTimeOffsetMs(c_interimPositionTimeOffsetMsec);
            situation.setInterimFlag(true);

            emit cbvar_cast(cbvar)->aircraftInterimPositionUpdate(situation);
        }

        void CNetworkVatlib::onAtcPositionUpdate(VatSessionID, const char *callsign, const VatAtcPosition *pos, void *cbvar)
        {
            int frequencyKHz = pos->frequency;
            CFrequency freq(frequencyKHz, CFrequencyUnit::kHz());
            freq.switchUnit(CFrequencyUnit::MHz()); // we would not need to bother, but this makes it easier to identify
            emit cbvar_cast(cbvar)->atcPositionUpdate(
                CCallsign(cbvar_cast(cbvar)->fromFSD(callsign), CCallsign::Atc), freq,
                CCoordinateGeodetic(pos->latitude, pos->longitude, 0), CLength(pos->visibleRange, CLengthUnit::NM()));
        }

        void CNetworkVatlib::onKicked(VatSessionID, const char *reason, void *cbvar)
        {
            emit cbvar_cast(cbvar)->kicked(cbvar_cast(cbvar)->fromFSD(reason));
        }

        void CNetworkVatlib::onPong(VatSessionID, const char *sender, double elapsedTime, void *cbvar)
        {
            emit cbvar_cast(cbvar)->pongReceived(cbvar_cast(cbvar)->fromFSD(sender), CTime(elapsedTime, CTimeUnit::ms()));
        }

        void CNetworkVatlib::onCustomPacketReceived(VatSessionID, const char *callsign, const char *packetId, const char **data, int dataSize, void *cbvar)
        {
            cbvar_cast(cbvar)->customPacketDispatcher(cbvar_cast(cbvar)->fromFSD(callsign), cbvar_cast(cbvar)->fromFSD(packetId), cbvar_cast(cbvar)->fromFSD(data, dataSize));
        }

        void CNetworkVatlib::customPacketDispatcher(const BlackMisc::Aviation::CCallsign &callsign, const QString &packetId, const QStringList &data)
        {
            if (packetId.compare("FSIPI", Qt::CaseInsensitive) == 0)
            {
                if (data.size() < 9)
                {
                    CLogMessage(this).warning("Malformed FSIPI packet");
                }
                else
                {
                    // It doesn't matter whether it was a query or response. The information
                    // is the same for both.
                    emit customFSInnPacketReceived(callsign, data[1], data[2], data[7], data[8]);
                }
            }
            else if (packetId.compare("FSIPIR", Qt::CaseInsensitive) == 0)
            {
                if (data.size() < 9)
                {
                    CLogMessage(this).warning("Malformed FSIPIR packet");
                }
                else
                {
                    sendCustomFsinnReponse(callsign);
                    // It doesn't matter whether it was a query or response. The information
                    // is the same for both.
                    emit customFSInnPacketReceived(callsign, data[1], data[2], data[7], data[8]);
                }
            }
            else if (packetId.compare("FSIP2PR", Qt::CaseInsensitive) == 0)
            {
                // FSInn peer2peer protocol - ignore, not supported
            }
            else
            {
                CLogMessage(this).warning("Unknown custom packet from %1 - id: %2") << callsign.toQString() << packetId;
            }
        }

        void CNetworkVatlib::onMetarReceived(VatSessionID, const char *data, void *cbvar)
        {
            emit cbvar_cast(cbvar)->metarReplyReceived(cbvar_cast(cbvar)->fromFSD(data));
        }

        void CNetworkVatlib::onInfoQueryRequestReceived(VatSessionID, const char *callsignString, VatClientQueryType type, const char *, void *cbvar)
        {
            auto timer = new QTimer(cbvar_cast(cbvar));
            timer->setSingleShot(true);
            timer->start(0);

            BlackMisc::Aviation::CCallsign callsign(callsignString);
            switch (type)
            {
            case vatClientQueryFreq:
                connect(timer, &QTimer::timeout, [ = ]() { cbvar_cast(cbvar)->replyToFrequencyQuery(callsign); });
                break;
            case vatClientQueryName:
                connect(timer, &QTimer::timeout, [ = ]() { cbvar_cast(cbvar)->replyToNameQuery(callsign); });
                break;
            default:
                break;
            }
        }

        void CNetworkVatlib::onInfoQueryReplyReceived(VatSessionID, const char *callsign, VatClientQueryType type, const char *data, const char *data2, void *cbvar)
        {
            switch (type)
            {
            case vatClientQueryFreq:   emit cbvar_cast(cbvar)->frequencyReplyReceived(cbvar_cast(cbvar)->fromFSD(callsign), CFrequency(cbvar_cast(cbvar)->fromFSD(data).toFloat(), CFrequencyUnit::MHz())); break;
            case vatClientQueryServer: emit cbvar_cast(cbvar)->serverReplyReceived(cbvar_cast(cbvar)->fromFSD(callsign), cbvar_cast(cbvar)->fromFSD(data)); break;
            case vatClientQueryAtc:    emit cbvar_cast(cbvar)->atcReplyReceived(CCallsign(cbvar_cast(cbvar)->fromFSD(data2), CCallsign::Atc), *data == 'Y'); break;
            case vatClientQueryName:   emit cbvar_cast(cbvar)->realNameReplyReceived(cbvar_cast(cbvar)->fromFSD(callsign), cbvar_cast(cbvar)->fromFSD(data)); break;
            case vatClientQueryIP:     emit cbvar_cast(cbvar)->ipReplyReceived(cbvar_cast(cbvar)->fromFSD(data)); break;
            default: break;
            }
        }

        void CNetworkVatlib::onCapabilitiesReplyReceived(VatSessionID, const char *callsign, int capabilityFlags, void *cbvar)
        {
            int flags = 0;
            if (capabilityFlags & vatCapsAtcInfo) { flags |= AcceptsAtisResponses; }
            if (capabilityFlags & vatCapsFastPos) { flags |= SupportsInterimPosUpdates; }
            if (capabilityFlags & vatCapsAircraftInfo) { flags |= SupportsIcaoCodes; }
            if (capabilityFlags & vatCapsAircraftConfig) { flags |= SupportsAircraftConfigs; }
            emit cbvar_cast(cbvar)->capabilitiesReplyReceived(cbvar_cast(cbvar)->fromFSD(callsign), flags);
        }

        void CNetworkVatlib::onAtisReplyReceived(VatSessionID, const char *callsign, const VatControllerAtis *atis, void *cbvar)
        {
            emit cbvar_cast(cbvar)->atisVoiceRoomReplyReceived(cbvar_cast(cbvar)->fromFSD(callsign), cbvar_cast(cbvar)->fromFSD(atis->voiceRoom));
            emit cbvar_cast(cbvar)->atisLogoffTimeReplyReceived(cbvar_cast(cbvar)->fromFSD(callsign), cbvar_cast(cbvar)->fromFSD(atis->zuluLogoff));

            CInformationMessage atisMessage;
            atisMessage.setType(CInformationMessage::ATIS);
            for (int i = 0; i < atis->textLineCount; ++i)
            {
                const QString fixed = cbvar_cast(cbvar)->fromFSD(atis->textLines[i]).trimmed();
                if (! fixed.isEmpty())
                {
                    //  detect the stupid z1, z2, z3 placeholders
                    //! \fixme: Anything better as this stupid code here?
                    const QString test = fixed.toLower().remove(QRegExp("[\\n\\t\\r]"));
                    if (test == "z") return;
                    if (test.startsWith("z") && test.length() == 2) return; // z1, z2, ..
                    if (test.length() == 1) return; // sometimes just z

                    // append
                    if (!atisMessage.isEmpty()) atisMessage.appendMessage("\n");
                    atisMessage.appendMessage(fixed);
                }
            }

            emit cbvar_cast(cbvar)->atisReplyReceived(CCallsign(cbvar_cast(cbvar)->fromFSD(callsign), CCallsign::Atc), atisMessage);
        }

        void CNetworkVatlib::onFlightPlanReceived(VatSessionID, const char *callsign, const VatFlightPlan *fp, void *cbvar)
        {
            BlackMisc::Aviation::CFlightPlan::FlightRules rules = BlackMisc::Aviation::CFlightPlan::VFR;
            switch (fp->flightType)
            {
            default:
            case vatFlightTypeVFR:   rules = BlackMisc::Aviation::CFlightPlan::VFR;  break;
            case vatFlightTypeIFR:   rules = BlackMisc::Aviation::CFlightPlan::IFR;  break;
                // case Cvatlib_Network::fpRuleType_SVFR:  rules = BlackMisc::Aviation::CFlightPlan::SVFR; break;
            }

            auto cruiseAltString = cbvar_cast(cbvar)->fromFSD(fp->cruiseAltitude);
            static const QRegExp withUnit("\\D+");
            if (!cruiseAltString.isEmpty() && withUnit.indexIn(cruiseAltString) < 0)
            {
                cruiseAltString += "ft";
            }
            BlackMisc::Aviation::CAltitude cruiseAlt;
            cruiseAlt.parseFromString(cruiseAltString);

            QString depTimePlanned = QString("0000").append(QString::number(fp->departTime)).right(4);
            QString depTimeActual = QString("0000").append(QString::number(fp->departTimeActual)).right(4);

            BlackMisc::Aviation::CFlightPlan flightPlan(
                cbvar_cast(cbvar)->fromFSD(fp->aircraftType),
                cbvar_cast(cbvar)->fromFSD(fp->departAirport),
                cbvar_cast(cbvar)->fromFSD(fp->destAirport),
                cbvar_cast(cbvar)->fromFSD(fp->alternateAirport),
                QDateTime::fromString(depTimePlanned, "hhmm"),
                QDateTime::fromString(depTimeActual, "hhmm"),
                BlackMisc::PhysicalQuantities::CTime(fp->enrouteHrs * 60 + fp->enrouteMins, BlackMisc::PhysicalQuantities::CTimeUnit::min()),
                BlackMisc::PhysicalQuantities::CTime(fp->fuelHrs * 60 + fp->fuelMins, BlackMisc::PhysicalQuantities::CTimeUnit::min()),
                cruiseAlt,
                BlackMisc::PhysicalQuantities::CSpeed(fp->trueCruisingSpeed, BlackMisc::PhysicalQuantities::CSpeedUnit::kts()),
                rules,
                cbvar_cast(cbvar)->fromFSD(fp->route),
                cbvar_cast(cbvar)->fromFSD(fp->remarks)
            );

            emit cbvar_cast(cbvar)->flightPlanReplyReceived(CCallsign(callsign, CCallsign::Atc), flightPlan);
        }

        void CNetworkVatlib::onErrorReceived(VatSessionID, VatServerError error, const char *msg, const char *data, void *cbvar)
        {
            auto *self = cbvar_cast(cbvar);
            switch (error)
            {
            case vatServerErrorCsInUs:          CLogMessage(self).error("The requested callsign is already taken"); emit self->terminate(); break;
            case vatServerErrorCallsignInvalid: CLogMessage(self).error("The requested callsign is not valid"); emit self->terminate(); break;
            case vatServerErrorCidInvalid:      CLogMessage(self).error("Wrong user ID or password"); emit self->terminate(); break;
            case vatServerErrorRevision:        CLogMessage(self).error("This server does not support our protocol version"); emit self->terminate(); break;
            case vatServerErrorLevel:           CLogMessage(self).error("You are not authorized to use the requested pilot rating"); emit self->terminate(); break;
            case vatServerErrorServFull:        CLogMessage(self).error("The server is full"); emit self->terminate(); break;
            case vatServerErrorCsSuspended:     CLogMessage(self).error("Your user account is suspended"); emit self->terminate(); break;
            case vatServerErrorInvPos:          CLogMessage(self).error("You are not authorized to use the requested rating"); emit self->terminate(); break;
            case vatServerErrorUnAuth:          CLogMessage(self).error("This software is not authorized for use on this network"); emit self->terminate(); break;

            case vatServerErrorNone:            CLogMessage(self).info("OK"); break;
            case vatServerErrorSyntax:          CLogMessage(self).info("Malformed packet: Syntax error: %1") << self->fromFSD(data); break;
            case vatServerErrorSrcInvalid:      CLogMessage(self).info("Server: source invalid %1") << self->fromFSD(data); break;
            case vatServerErrorNoSuchCs:        CLogMessage(self).info("Shim lib: %1 (%2)") << self->fromFSD(msg) << self->fromFSD(data); break;
            case vatServerErrorNoFP:            CLogMessage(self).info("Server: no flight plan"); break;
            case vatServerErrorNoWeather:       CLogMessage(self).info("Server: requested weather profile does not exist"); break;

            // we have no idea what these mean
            case vatServerErrorRegistered:
            case vatServerErrorInvalidCtrl:     CLogMessage(self).info("Server: ") << self->fromFSD(msg); break;

            default:                            qFatal("vatlib: %s (error %d)", msg, error); emit self->terminate();
            }
        }

        void CNetworkVatlib::onPilotInfoRequestReceived(VatSessionID, const char *callsignString, void *cbvar)
        {
            auto timer = new QTimer(cbvar_cast(cbvar));
            timer->setSingleShot(true);
            timer->start(0);

            BlackMisc::Aviation::CCallsign callsign(callsignString);
            connect(timer, &QTimer::timeout, [ = ]() { cbvar_cast(cbvar)->sendAircraftInfo(callsign); });
        }

        void CNetworkVatlib::onPilotInfoReceived(VatSessionID, const char *callsign, const VatAircraftInfo *aircraftInfo, void *cbvar)
        {
            emit cbvar_cast(cbvar)->icaoCodesReplyReceived(
                cbvar_cast(cbvar)->fromFSD(callsign),
                cbvar_cast(cbvar)->fromFSD(aircraftInfo->aircraftType).trimmed().toUpper(),
                cbvar_cast(cbvar)->fromFSD(aircraftInfo->airline).trimmed().toUpper(),
                cbvar_cast(cbvar)->fromFSD(aircraftInfo->livery).trimmed().toUpper()
            );
        }

        void CNetworkVatlib::networkLogHandler(SeverityLevel /** severity **/, const char *message)
        {
            CLogMessage(static_cast<CNetworkVatlib *>(nullptr)).error(message);
        }

        QJsonObject CNetworkVatlib::JsonPackets::aircraftConfigRequest()
        {
            return { { "request", "full" } };
        }
    } // namespace
} // namespace
//! \endcond
