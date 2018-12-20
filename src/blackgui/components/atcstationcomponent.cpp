/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift Project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackcore/context/contextnetwork.h"
#include "blackcore/context/contextownaircraft.h"
#include "blackcore/webdataservices.h"
#include "blackgui/components/atcstationcomponent.h"
#include "blackgui/guiapplication.h"
#include "blackgui/guiutility.h"
#include "blackgui/infoarea.h"
#include "blackgui/uppercasevalidator.h"
#include "blackgui/models/atcstationlistmodel.h"
#include "blackgui/models/atcstationtreemodel.h"
#include "blackgui/views/atcstationview.h"
#include "blackgui/views/viewbase.h"
#include "blackmisc/aviation/atcstationlist.h"
#include "blackmisc/aviation/informationmessage.h"
#include "blackmisc/compare.h"
#include "blackmisc/icons.h"
#include "blackmisc/logmessage.h"
#include "blackmisc/weather/metar.h"
#include "ui_atcstationcomponent.h"

#include <QAbstractItemModel>
#include <QLineEdit>
#include <QModelIndex>
#include <QPushButton>
#include <QStandardItemModel>
#include <QStringBuilder>
#include <QTabBar>
#include <QGroupBox>
#include <QTableView>
#include <QTextEdit>
#include <QTimer>
#include <QTreeView>
#include <QCompleter>
#include <QPointer>

using namespace BlackGui::Models;
using namespace BlackGui::Views;
using namespace BlackGui::Settings;
using namespace BlackMisc;
using namespace BlackMisc::Aviation;
using namespace BlackMisc::PhysicalQuantities;
using namespace BlackMisc::Weather;
using namespace BlackCore;
using namespace BlackCore::Context;

namespace BlackGui
{
    namespace Components
    {
        CAtcStationComponent::CAtcStationComponent(QWidget *parent) :
            COverlayMessagesFrame(parent),
            CIdentifiable(this),
            ui(new Ui::CAtcStationComponent)
        {
            Q_ASSERT_X(sGui, Q_FUNC_INFO, "Need sGui");
            ui->setupUi(this);
            ui->tw_Atc->setCurrentIndex(0);
            ui->tw_Atc->tabBar()->setExpanding(false);
            ui->tw_Atc->tabBar()->setUsesScrollButtons(true);
            CUpperCaseValidator *ucv = new CUpperCaseValidator(ui->le_AtcStationsOnlineMetar);
            ui->le_AtcStationsOnlineMetar->setValidator(ucv);

            // some icons
            ui->tb_AtcStationsAtisReload->setIcon(CIcons::atis());
            ui->tb_AtcStationsAtisReload->setText("");
            ui->tb_AtcStationsLoadMetar->setIcon(CIcons::metar());
            ui->tb_AtcStationsLoadMetar->setText("");
            ui->tb_Audio->setIcon(CIcons::appAudio16());
            ui->tb_Audio->setText("");
            ui->tb_TextMessageOverlay->setIcon(CIcons::appTextMessages16());
            ui->tb_TextMessageOverlay->setText("");

            // set station mode
            ui->tvp_AtcStationsOnline->setStationMode(CAtcStationListModel::StationsOnline);
            ui->tvp_AtcStationsBooked->setStationMode(CAtcStationListModel::StationsBooked);
            ui->tvp_AtcStationsOnlineTree->setColumns(ui->tvp_AtcStationsOnline->getColumns());

            // menus
            ui->tvp_AtcStationsOnline->menuRemoveItems(CAtcStationView::MenuClear);
            ui->tvp_AtcStationsBooked->menuRemoveItems(CAtcStationView::MenuClear);

            // Signal / Slots
            connect(ui->le_AtcStationsOnlineMetar, &QLineEdit::returnPressed, this, &CAtcStationComponent::getMetarAsEntered);
            connect(ui->tb_AtcStationsLoadMetar, &QPushButton::clicked, this, &CAtcStationComponent::getMetarAsEntered);
            connect(ui->tb_Audio, &QPushButton::clicked, this, &CAtcStationComponent::requestAudioWidget);
            connect(ui->tb_TextMessageOverlay, &QPushButton::clicked, this, &CAtcStationComponent::showOverlayInlineTextMessage);
            connect(ui->tw_Atc, &QTabWidget::currentChanged, this, &CAtcStationComponent::atcStationsTabChanged); // "local" tab changed (booked, online)
            connect(ui->tvp_AtcStationsOnline, &CAtcStationView::objectClicked, this, &CAtcStationComponent::onlineAtcStationSelected);
            connect(ui->tvp_AtcStationsOnline, &CAtcStationView::objectSelected, this, &CAtcStationComponent::onlineAtcStationSelected);
            connect(ui->tvp_AtcStationsOnline, &CAtcStationView::testRequestDummyAtcOnlineStations, this, &CAtcStationComponent::testCreateDummyOnlineAtcStations);
            connect(ui->tvp_AtcStationsOnline, &CAtcStationView::requestUpdate, this, &CAtcStationComponent::requestOnlineStationsUpdate);
            connect(ui->tvp_AtcStationsOnline, &CAtcStationView::requestNewBackendData, this, &CAtcStationComponent::requestOnlineStationsUpdate);
            connect(ui->tvp_AtcStationsOnline, &CAtcStationView::modelDataChangedDigest, this, &CAtcStationComponent::onCountChanged);
            connect(ui->tvp_AtcStationsOnline, &CAtcStationView::requestComFrequency, this, &CAtcStationComponent::setComFrequency);
            connect(ui->tvp_AtcStationsOnline, &CAtcStationView::requestTextMessageWidget, this, &CAtcStationComponent::requestTextMessageWidget);
            connect(ui->tvp_AtcStationsOnlineTree, &CAtcStationTreeView::requestComFrequency, this, &CAtcStationComponent::setComFrequency);
            connect(ui->tvp_AtcStationsOnlineTree, &CAtcStationTreeView::requestTextMessageWidget, this, &CAtcStationComponent::requestTextMessageWidget);

            connect(ui->comp_AtcStationsSettings, &CSettingsAtcStationsInlineComponent::changed, this, &CAtcStationComponent::forceUpdate, Qt::QueuedConnection);

            connect(ui->tvp_AtcStationsBooked, &CAtcStationView::requestUpdate, this, &CAtcStationComponent::reloadAtcStationsBooked);
            connect(ui->tvp_AtcStationsBooked, &CAtcStationView::requestNewBackendData, this, &CAtcStationComponent::reloadAtcStationsBooked);
            connect(ui->tvp_AtcStationsBooked, &CAtcStationView::modelDataChangedDigest, this, &CAtcStationComponent::onCountChanged);

            connect(ui->tb_AtcStationsAtisReload, &QPushButton::clicked, this, &CAtcStationComponent::requestAtis);
            connect(&m_updateTimer, &QTimer::timeout, this, &CAtcStationComponent::update);

            // Group box
            connect(ui->gb_Details, &QGroupBox::toggled, this, &CAtcStationComponent::onDetailsToggled);

            // runtime based connects
            connect(sGui->getIContextNetwork(), &IContextNetwork::changedAtcStationsOnlineDigest, this, &CAtcStationComponent::changedAtcStationsOnline, Qt::QueuedConnection);
            connect(sGui->getIContextNetwork(), &IContextNetwork::changedAtcStationsBookedDigest, this, &CAtcStationComponent::changedAtcStationsBooked, Qt::QueuedConnection);
            connect(sGui->getIContextNetwork(), &IContextNetwork::changedAtcStationOnlineConnectionStatus, this, &CAtcStationComponent::changedAtcStationOnlineConnectionStatus, Qt::QueuedConnection);
            connect(sGui->getIContextNetwork(), &IContextNetwork::connectionStatusChanged, this, &CAtcStationComponent::connectionStatusChanged, Qt::QueuedConnection);

            // selection
            ui->tvp_AtcStationsOnline->acceptClickSelection(true);
            ui->tvp_AtcStationsOnline->acceptRowSelection(true);

            QVBoxLayout *layout = this->vLayout();
            m_stretch.push_back(layout->stretch(0));
            m_stretch.push_back(layout->stretch(1));

            // web readers
            if (sGui->hasWebDataServices())
            {
                connect(sGui->getWebDataServices(), &CWebDataServices::swiftDbAirportsRead, this, &CAtcStationComponent::airportsRead);
                this->airportsRead();
            }

            // init settings
            this->settingsChanged();
        }

        CAtcStationComponent::~CAtcStationComponent()
        { }

        void CAtcStationComponent::setTab(CAtcStationComponent::AtcTab tab)
        {
            const int t = static_cast<int>(tab);
            ui->tw_Atc->setCurrentIndex(t);
        }

        int CAtcStationComponent::countBookedStations() const
        {
            return ui->tvp_AtcStationsBooked->rowCount();
        }

        int CAtcStationComponent::countOnlineStations() const
        {
            return ui->tvp_AtcStationsOnline->rowCount();
        }

        bool CAtcStationComponent::setParentDockWidgetInfoArea(CDockWidgetInfoArea *parentDockableWidget)
        {
            CEnableForDockWidgetInfoArea::setParentDockWidgetInfoArea(parentDockableWidget);
            const bool c = connect(this->getParentInfoArea(), &CInfoArea::changedInfoAreaTabBarIndex, this, &CAtcStationComponent::infoAreaTabBarChanged);
            Q_ASSERT_X(c, Q_FUNC_INFO, "failed connect");
            Q_ASSERT_X(parentDockableWidget, Q_FUNC_INFO, "missing parent");
            return c && parentDockableWidget;
        }

        void CAtcStationComponent::forceUpdate()
        {
            m_timestampOnlineStationsChanged = QDateTime::currentDateTimeUtc();
            this->update();
        }

        void CAtcStationComponent::update()
        {
            if (!this->canAccessContext()) { return; }
            Q_ASSERT(ui->tvp_AtcStationsBooked);
            Q_ASSERT(ui->tvp_AtcStationsOnline);

            // check if component is visible, if we have already data then skip udpate
            const bool hasData = this->countBookedStations() > 0 || this->countOnlineStations() > 0;
            if (hasData && !this->isVisibleWidget())
            {
                // Update skipped, as not visible
                ui->tvp_AtcStationsBooked->hideLoadIndicator();
                ui->tvp_AtcStationsOnline->hideLoadIndicator();
                return;
            }

            // bookings
            if (m_timestampBookedStationsChanged > m_timestampLastReadBookedStations)
            {
                this->reloadAtcStationsBooked();
            }

            // online stations, only when connected
            if (sGui->getIContextNetwork()->isConnected())
            {
                // update
                if (m_timestampOnlineStationsChanged > m_timestampLastReadOnlineStations)
                {
                    const CAtcStationsSettings settings = ui->comp_AtcStationsSettings->getSettings();
                    CAtcStationList onlineStations = sGui->getIContextNetwork()->getAtcStationsOnline(true);
                    if (settings.showOnlyWithValidFrequency()) { onlineStations = onlineStations.stationsWithValidFrequency(); }
                    if (settings.showOnlyWithValidVoiceRoom()) { onlineStations = onlineStations.stationsWithValidVoiceRoom(); }
                    if (settings.showOnlyInRange())
                    {
                        onlineStations.removeIfOutsideRange();
                    }

                    ui->tvp_AtcStationsOnline->updateContainerMaybeAsync(onlineStations);
                    m_timestampLastReadOnlineStations = QDateTime::currentDateTimeUtc();
                    m_timestampOnlineStationsChanged  = m_timestampLastReadOnlineStations;
                    this->updateTreeView();
                }
            }
            else
            {
                ui->tvp_AtcStationsOnline->clear();
                this->updateTreeView();
            }
        }

        void CAtcStationComponent::changedAtcStationOnlineConnectionStatus(const CAtcStation &station, bool added)
        {
            // trick here is, we want to display a station ASAP
            ui->tvp_AtcStationsOnline->changedAtcStationConnectionStatus(station, added);
        }

        void CAtcStationComponent::getMetarAsEntered()
        {
            this->getMetar("");
        }

        void CAtcStationComponent::getMetar(const QString &airportIcaoCode)
        {
            if (!this->canAccessContext()) { return; }
            const CAirportIcaoCode icao(airportIcaoCode.isEmpty() ? ui->le_AtcStationsOnlineMetar->text().trimmed().toUpper() : airportIcaoCode.trimmed().toUpper());
            ui->le_AtcStationsOnlineMetar->setText(icao.asString());
            if (!icao.hasValidIcaoCode()) { return; }
            const CMetar metar(sGui->getIContextNetwork()->getMetarForAirport(icao));
            if (metar.hasMessage())
            {
                const QString metarText = metar.getMessage() % u"\n\n" % metar.getMetarText();
                ui->te_AtcStationsOnlineInfo->setText(metarText);
            }
            else
            {
                ui->te_AtcStationsOnlineInfo->clear();
            }
        }

        void CAtcStationComponent::reloadAtcStationsBooked()
        {
            Q_ASSERT(ui->tvp_AtcStationsBooked);
            if (!this->canAccessContext()) { return; }

            QObject *sender = QObject::sender();
            if (sender == ui->tvp_AtcStationsBooked)
            {
                // trigger new read, which takes some time. A signal will be received when this is done
                CLogMessage(this).info("Requested new bookings");
                sGui->getIContextNetwork()->requestAtcBookingsUpdate();
            }
            else
            {
                ui->tvp_AtcStationsBooked->updateContainerMaybeAsync(sGui->getIContextNetwork()->getAtcStationsBooked(false));
                m_timestampLastReadBookedStations = QDateTime::currentDateTimeUtc();
            }
        }

        void CAtcStationComponent::changedAtcStationsOnline()
        {
            // just update timestamp, data will be pulled by timer
            // the timestamp will tell if there are any newer data
            m_timestampOnlineStationsChanged = QDateTime::currentDateTimeUtc();
        }

        void CAtcStationComponent::changedAtcStationsBooked()
        {
            // a change can mean a complete change of the bookings, or
            // a single value is updated (e.g. online status)
            // just update timestamp, data will be pulled by timer
            // the timestamp will tell if there are any newer data
            // unlike online stations, this can happen if we are not connected to a FSD server

            m_timestampBookedStationsChanged = QDateTime::currentDateTimeUtc();
            if (m_updateTimer.isActive()) { return; } // update by timer
            this->update();
        }

        void CAtcStationComponent::connectionStatusChanged(INetwork::ConnectionStatus from, INetwork::ConnectionStatus to)
        {
            Q_UNUSED(from);
            if (INetwork::isConnectedStatus(to))
            {
                ui->tvp_AtcStationsOnline->clear();
                this->updateTreeView();
                m_updateTimer.start();
            }
            else if (INetwork::isDisconnectedStatus(to))
            {
                m_updateTimer.stop();
                this->clearOnlineViews();
            }
        }

        void CAtcStationComponent::testCreateDummyOnlineAtcStations(int number)
        {
            if (this->canAccessContext())
            {
                sGui->getIContextNetwork()->testCreateDummyOnlineAtcStations(number);
            }
        }

        void CAtcStationComponent::requestOnlineStationsUpdate()
        {
            m_timestampLastReadOnlineStations.setMSecsSinceEpoch(0); // mark as outdated
            this->update();
        }

        void CAtcStationComponent::infoAreaTabBarChanged(int index)
        {
            // ignore in those cases
            if (!this->isVisibleWidget()) { return; }
            if (this->isParentDockWidgetFloating()) { return; }

            // here I know I am the selected widget, update, but keep GUI responsive (-> timer)
            const QPointer<CAtcStationComponent> myself(this);
            QTimer::singleShot(1000, this, [ = ]
            {
                if (!sApp || sApp->isShuttingDown() || !myself) { return; }
                this->update();
            });
            Q_UNUSED(index);
        }

        void CAtcStationComponent::onCountChanged(int count, bool withFilter)
        {
            Q_UNUSED(count);
            Q_UNUSED(withFilter);
            const int io = ui->tw_Atc->indexOf(ui->tb_AtcStationsOnline);
            const int it = ui->tw_Atc->indexOf(ui->tb_AtcStationsOnlineTree);
            const int ib = ui->tw_Atc->indexOf(ui->tb_AtcStationsBooked);
            QString o = ui->tw_Atc->tabBar()->tabText(io);
            QString t = ui->tw_Atc->tabBar()->tabText(it);
            QString b = ui->tw_Atc->tabBar()->tabText(ib);
            o = CGuiUtility::replaceTabCountValue(o, this->countOnlineStations());
            t = CGuiUtility::replaceTabCountValue(t, this->countOnlineStations());
            b = CGuiUtility::replaceTabCountValue(b, this->countBookedStations());
            ui->tw_Atc->tabBar()->setTabText(io, o);
            ui->tw_Atc->tabBar()->setTabText(it, t);
            ui->tw_Atc->tabBar()->setTabText(ib, b);
        }

        void CAtcStationComponent::setComFrequency(const PhysicalQuantities::CFrequency &frequency, CComSystem::ComUnit unit)
        {
            if (unit != CComSystem::Com1 && unit != CComSystem::Com2) { return; }
            if (!CComSystem::isValidComFrequency(frequency)) { return; }
            sGui->getIContextOwnAircraft()->updateActiveComFrequency(frequency, unit, identifier());
        }

        void CAtcStationComponent::settingsChanged()
        {
            if (!this->canAccessContext()) { return; }
            const CViewUpdateSettings settings = m_settingsView.get();
            const int ms = settings.getAtcUpdateTime().toMs();
            const bool connected = sGui->getIContextNetwork()->isConnected();
            m_updateTimer.setInterval(ms);
            if (connected)
            {
                m_timestampOnlineStationsChanged = QDateTime::currentDateTimeUtc();
                m_updateTimer.start(ms); // restart
                this->update();
            }
            else
            {
                m_updateTimer.stop();
            }
        }

        void CAtcStationComponent::airportsRead()
        {
            this->initCompleters();
        }

        void CAtcStationComponent::updateTreeView()
        {
            ui->tvp_AtcStationsOnlineTree->updateContainer(ui->tvp_AtcStationsOnline->container());
            ui->tvp_AtcStationsOnlineTree->fullResizeToContents();
        }

        void CAtcStationComponent::initCompleters()
        {
            if (!sGui || !sGui->getWebDataServices()) { return; }
            const QStringList airports = sGui->getWebDataServices()->getAirports().allIcaoCodes(true);
            if (!airports.isEmpty())
            {
                QCompleter *airportCompleter = new QCompleter(airports, this);
                airportCompleter->popup()->setMinimumWidth(75);
                ui->le_AtcStationsOnlineMetar->setCompleter(airportCompleter);
            }
        }

        void CAtcStationComponent::onlineAtcStationSelected(const CVariant &object)
        {
            ui->te_AtcStationsOnlineInfo->setText(""); // reset
            if (!object.isValid() || !object.canConvert<CAtcStation>()) { return; }
            const CAtcStation stationClicked = object.valueOrDefault(CAtcStation());
            QString infoMessage;

            if (stationClicked.hasAtis())
            {
                infoMessage.append(stationClicked.getAtis().getMessage());
            }
            if (stationClicked.hasMetar())
            {
                if (!infoMessage.isEmpty()) { infoMessage.append("\n\n"); }
                infoMessage.append(stationClicked.getMetar().getMessage());
            }
            ui->te_AtcStationsOnlineInfo->setText(infoMessage);
        }

        void CAtcStationComponent::atcStationsTabChanged()
        {
            const bool booked = ui->tw_Atc->currentWidget() == ui->tb_AtcStationsBooked;
            if (booked)
            {
                if (ui->tvp_AtcStationsBooked->isEmpty())
                {
                    this->reloadAtcStationsBooked();
                }
            }
            ui->gb_Details->setVisible(!booked);
        }

        void CAtcStationComponent::requestAtis()
        {
            if (!this->canAccessContext()) return;
            sGui->getIContextNetwork()->requestAtisUpdates();
        }

        bool CAtcStationComponent::canAccessContext() const
        {
            if (!sGui || sGui->isShuttingDown() || !sGui->getIContextNetwork()) { return false; }
            return true;
        }

        void CAtcStationComponent::clearOnlineViews()
        {
            ui->tvp_AtcStationsOnline->clear();
            ui->tvp_AtcStationsOnlineTree->clear();
        }

        void CAtcStationComponent::showOverlayInlineTextMessage()
        {
            COverlayMessagesFrame::showOverlayInlineTextMessage(TextMessagesCom1);
        }

        void CAtcStationComponent::onDetailsToggled(bool checked)
        {
            QVBoxLayout *layout = this->vLayout();
            if (layout)
            {
                if (checked)
                {
                    layout->setStretchFactor(ui->tw_Atc, m_stretch.at(0));
                    layout->setStretchFactor(ui->gb_Details, m_stretch.at(1));
                }
                else
                {
                    layout->setStretchFactor(ui->tw_Atc, 0);
                    layout->setStretchFactor(ui->gb_Details, 0);
                }
            }

            ui->te_AtcStationsOnlineInfo->setVisible(checked);
            ui->comp_AtcStationsSettings->setVisible(checked);
            ui->le_AtcStationsOnlineMetar->setVisible(checked);
            ui->tb_AtcStationsAtisReload->setVisible(checked);
            ui->tb_AtcStationsLoadMetar->setVisible(checked);
            ui->tb_TextMessageOverlay->setVisible(checked);
            ui->tb_Audio->setVisible(checked);
        }

        QVBoxLayout *CAtcStationComponent::vLayout() const
        {
            QVBoxLayout *layout = qobject_cast<QVBoxLayout *>(this->layout());
            return layout;
        }
    } // namespace
} // namespace
