#include "introwindow.h"
#include "ui_introwindow.h"
#include "blackcore/dbus_server.h"
#include "blackmisc/networkutils.h"
#include "blackmisc/settingutilities.h"
#include <QDesktopServices>
#include <QUrl>
#include <QDir>

/*
 * Constructor
 */
CIntroWindow::CIntroWindow(QWidget *parent) :
    QDialog(parent, (Qt::Tool | Qt::WindowStaysOnTopHint)),
    ui(new Ui::CIntroWindow)
{
    ui->setupUi(this);
    this->layout()->setSizeConstraint(QLayout::SetFixedSize);
    this->ui->cb_DBusServer->addItem(BlackCore::CDBusServer::sessionDBusServer());
    this->ui->cb_DBusServer->addItem(BlackCore::CDBusServer::systemDBusServer());
    this->ui->cb_DBusServer->addItems(BlackMisc::CNetworkUtils::getKnownIpAddresses());
    this->ui->cb_DBusServer->setCurrentIndex(0);
}

/*
 * Destructor
 */
CIntroWindow::~CIntroWindow() { }

/*
 * Window mode
 */
GuiModes::WindowMode CIntroWindow::getWindowMode() const
{
    if (this->ui->rb_WindowFrameless->isChecked())
        return GuiModes::WindowFrameless;
    else
        return GuiModes::WindowNormal;
}

/*
 * Core mode
 */
GuiModes::CoreMode CIntroWindow::getCoreMode() const
{
    if (this->ui->rb_CoreExternalVoiceLocal->isChecked())
        return GuiModes::CoreExternalAudioLocal;
    else if (this->ui->rb_CoreInGuiProcess->isChecked())
        return GuiModes::CoreInGuiProcess;
    else
        return GuiModes::CoreExternal;
}
/*
 * DBus server address
 */
QString CIntroWindow::getDBusAddress() const
{
    return this->ui->cb_DBusServer->currentText();
}

/*
 * Button clicked
 */
void CIntroWindow::buttonClicked() const
{
    QObject *sender = QObject::sender();
    if (sender == this->ui->pb_ModelDb)
    {
        QDesktopServices::openUrl(QUrl("http://vatrep.vatsim-germany.org/page/index.php", QUrl::TolerantMode));
    }
    else if (sender == this->ui->pb_WebSite)
    {
        QDesktopServices::openUrl(QUrl("https://dev.vatsim-germany.org/", QUrl::TolerantMode));
    }
    else if (sender == this->ui->pb_SettingsDir)
    {
        QString path = QDir::toNativeSeparators(BlackMisc::Settings::CSettingUtilities::getSettingsDirectory());
        QDesktopServices::openUrl(QUrl("file:///" + path));
    }
}
