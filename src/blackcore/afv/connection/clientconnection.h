/* Copyright (C) 2019
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution. No part of swift project, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 */

//! \file

#ifndef BLACKCORE_AFV_CONNECTION_CLIENTCONNECTION_H
#define BLACKCORE_AFV_CONNECTION_CLIENTCONNECTION_H

#include "blackcore/afv/crypto/cryptodtoserializer.h"
#include "blackcore/afv/connection/clientconnectiondata.h"
#include "blackcore/afv/connection/apiserverconnection.h"
#include "blackcore/afv/dto.h"

#include <QObject>
#include <QString>
#include <QTimer>
#include <QUdpSocket>

namespace BlackCore
{
    namespace Afv
    {
        namespace Connection
        {
            //! Client connection
            class CClientConnection : public QObject
            {
                Q_OBJECT

            public:
                //! Connection status
                enum ConnectionStatus
                {
                    Disconnected,   //!< Not connected
                    Connected,      //!< Connection established
                };
                Q_ENUM(ConnectionStatus)

                //! Ctor
                CClientConnection(const QString &apiServer, QObject *parent = nullptr);

                //! Connect/disconnect @{
                void connectTo(const QString &userName, const QString &password, const QString &callsign);
                void disconnectFrom(const QString &reason = {});
                bool isConnected() const { return m_connection.isConnected(); }
                //! @}

                //! Receiving audio? @{
                void setReceiveAudio(bool value) { m_connection.setReceiveAudio(value); }
                bool receiveAudio()    const { return m_connection.isReceivingAudio(); }
                bool receiveAudioDto() const { return m_receiveAudioDto; }
                void setReceiveAudioDto(bool receiveAudioDto)
                {
                    m_receiveAudioDto = receiveAudioDto;
                }
                //! @}

                //! Send voiceDTO to server
                template<typename T>
                void sendToVoiceServer(const T &dto)
                {
                    const QUrl voiceServerUrl("udp://" + m_connection.getTokens().VoiceServer.addressIpV4);
                    const QByteArray dataBytes = Crypto::CryptoDtoSerializer::serialize(*m_connection.m_voiceCryptoChannel, Crypto::CryptoDtoMode::AEAD_ChaCha20Poly1305, dto);
                    m_udpSocket->writeDatagram(dataBytes, QHostAddress(voiceServerUrl.host()), static_cast<quint16>(voiceServerUrl.port()));
                }

                //! Update transceivers
                void updateTransceivers(const QString &callsign, const QVector<TransceiverDto> &transceivers);

                //! All aliased stations
                QVector<StationDto> getAllAliasedStations();

            signals:
                //! Audio has been received
                void audioReceived(const AudioRxOnTransceiversDto &dto);

            private:
                void connectToVoiceServer();
                void disconnectFromVoiceServer();

                void readPendingDatagrams();
                void processMessage(const QByteArray &messageDdata, bool loopback = false);
                void handleSocketError(QAbstractSocket::SocketError error);

                void voiceServerHeartbeat();

                const QUuid m_networkVersion = QUuid("3a5ddc6d-cf5d-4319-bd0e-d184f772db80");

                //Data
                CClientConnectionData m_connection;

                // Voice server
                QUdpSocket *m_udpSocket        = nullptr;
                QTimer     *m_voiceServerTimer = nullptr;

                // API server
                CApiServerConnection *m_apiServerConnection = nullptr;

                // Properties
                bool m_receiveAudioDto = true;
            };
        } // ns
    } // ns
} // ns

#endif // guard
