#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "blackgui/atcstationlistmodel.h"
#include "blackcore/dbus_server.h"
#include "blackcore/context_network.h"
#include "blackmisc/hwkeyboardkey.h"

using namespace BlackCore;
using namespace BlackMisc;
using namespace BlackGui;
using namespace BlackMisc::Network;
using namespace BlackMisc::Aviation;
using namespace BlackMisc::PhysicalQuantities;
using namespace BlackMisc::Geo;
using namespace BlackMisc::Settings;

/*
 * Reload settings
 */
void MainWindow::reloadSettings()
{
    // local copy
    CSettingsNetwork nws = this->getIContextSettings()->getNetworkSettings();

    // update servers
    this->ui->tvp_SettingsTnServers->setSelectedServer(nws.getCurrentTrafficNetworkServer());
    this->ui->tvp_SettingsTnServers->update(nws.getTrafficNetworkServers());

    // update hot keys
    this->ui->tvp_SettingsMiscHotkeys->update(this->getIContextSettings()->getHotkeys());




    // fake setting for sound notifications
    this->ui->cb_SettingsAudioPlayNotificationSounds->setChecked(true);
    this->ui->cb_SettingsAudioNotificationTextMessage->setChecked(true);
}

/*
 * Network has been selected
 */
void MainWindow::networkServerSelected(QModelIndex index)
{
    const CServer clickedServer = this->ui->tvp_SettingsTnServers->at<CServer>(index);
    this->updateGuiSelectedServerTextboxes(clickedServer);
}

/*
 * Alter server
 */
void MainWindow::alterTrafficServer()
{
    CServer server = this->selectedServerFromTextboxes();
    if (!server.isValidForLogin())
    {
        const CStatusMessage validation = CStatusMessage::getValidationError("Wrong settings for server");
        this->displayStatusMessage(validation);
        return;
    }

    const QString path = CSettingUtilities::appendPaths(IContextSettings::PathNetworkSettings(), CSettingsNetwork::ValueTrafficServers());
    QObject *sender = QObject::sender();
    CStatusMessageList msgs;
    if (sender == this->ui->pb_SettingsTnCurrentServer)
    {
        msgs = this->getIContextSettings()->value(path, CSettingsNetwork::CmdSetCurrentServer(), server.toQVariant());
    }
    else if (sender == this->ui->pb_SettingsTnRemoveServer)
    {
        msgs = this->getIContextSettings()->value(path, CSettingUtilities::CmdRemove(), server.toQVariant());
    }
    else if (sender == this->ui->pb_SettingsTnSaveServer)
    {
        msgs = this->getIContextSettings()->value(path, CSettingUtilities::CmdUpdate(), server.toQVariant());
    }

    // status messages
    this->displayStatusMessages(msgs);
}

/*
 * Settings did changed
 */
void MainWindow::changedSettings(uint typeValue)
{
    IContextSettings::SettingsType type = static_cast<IContextSettings::SettingsType>(typeValue);
    this->reloadSettings();
    if (type == IContextSettings::SettingsHotKeys) this->setHotkeys();
}

/*
 * Textboxes from server
 */
void MainWindow::updateGuiSelectedServerTextboxes(const CServer &server)
{
    this->ui->le_SettingsTnCsName->setText(server.getName());
    this->ui->le_SettingsTnCsDescription->setText(server.getDescription());
    this->ui->le_SettingsTnCsAddress->setText(server.getAddress());
    this->ui->le_SettingsTnCsPort->setText(QString::number(server.getPort()));
    this->ui->le_SettingsTnCsRealName->setText(server.getUser().getRealName());
    this->ui->le_SettingsTnCsNetworkId->setText(server.getUser().getId());
    this->ui->le_SettingsTnCsPassword->setText(server.getUser().getPassword());
}

/*
 * Server settings from textboxes
 */
CServer MainWindow::selectedServerFromTextboxes() const
{
    CServer server;
    bool portOk = false;
    server.setName(this->ui->le_SettingsTnCsName->text());
    server.setDescription(this->ui->le_SettingsTnCsDescription->text());
    server.setAddress(this->ui->le_SettingsTnCsAddress->text());
    server.setPort(this->ui->le_SettingsTnCsPort->text().toInt(&portOk));
    if (!portOk) server.setPort(-1);

    CUser user;
    user.setRealName(this->ui->le_SettingsTnCsRealName->text());
    user.setId(this->ui->le_SettingsTnCsNetworkId->text());
    user.setPassword(this->ui->le_SettingsTnCsPassword->text());
    server.setUser(user);

    return server;
}

/*
 * Save the hotkeys
 */
void MainWindow::saveHotkeys()
{
    const QString path = CSettingUtilities::appendPaths(IContextSettings::PathRoot(), IContextSettings::PathHotkeys());
    CStatusMessageList msgs = this->getIContextSettings()->value(path, CSettingUtilities::CmdUpdate(), this->ui->tvp_SettingsMiscHotkeys->derivedModel()->getContainer().toQVariant());

    // status messages
    this->displayStatusMessages(msgs);
}

/*
 * Clear particular hotkey
 */
void MainWindow::clearHotkey()
{
    QModelIndex i = this->ui->tvp_SettingsMiscHotkeys->currentIndex();
    if (i.row() < 0 || i.row() >= this->ui->tvp_SettingsMiscHotkeys->rowCount()) return;
    BlackMisc::Hardware::CKeyboardKey key = this->ui->tvp_SettingsMiscHotkeys->at<BlackMisc::Hardware::CKeyboardKey>(i);
    BlackMisc::Hardware::CKeyboardKey defaultKey;
    defaultKey.setFunction(key.getFunction());
    this->ui->tvp_SettingsMiscHotkeys->derivedModel()->update(i, defaultKey);
}
