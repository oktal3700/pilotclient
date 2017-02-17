/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef SWIFTLAUNCHER_H
#define SWIFTLAUNCHER_H

#include "blackgui/overlaymessagesframe.h"
#include "blackgui/enableforframelesswindow.h"
#include "blackgui/mainwindowaccess.h"
#include "blackcore/data/globalsetup.h"
#include "blackcore/data/updateinfo.h"
#include "blackcore/data/launchersetup.h"
#include "blackcore/coremodeenums.h"
#include "blackmisc/identifiable.h"
#include <QDialog>
#include <QTimer>
#include <QScopedPointer>
#include <QNetworkReply>

namespace Ui { class CSwiftLauncher; }

/*!
 * swift launcher tool
 * \note Besides the fact the launcher makes it easy to start our applications it also pre-fetches some
 *       cache files, hence reducing load times in the subsequent applications. Therefor starting via the launcher
 *       is preferable, but not mandatory.
 */
class CSwiftLauncher :
    public QDialog,
    public BlackGui::CEnableForFramelessWindow,
    public BlackGui::IMainWindowAccess,
    public BlackMisc::CIdentifiable
{
    Q_OBJECT
    Q_INTERFACES(BlackGui::IMainWindowAccess)

public:
    //! Pages
    enum Pages
    {
        PageNews = 0,
        PageWindowType,
        PageCoreMode,
        PageUpdates
    };

    //! Constructor
    explicit CSwiftLauncher(QWidget *parent = nullptr);

    //! Destructor
    virtual ~CSwiftLauncher();

    //! Executable (to be started)
    const QString &getExecutable() const { return m_executable; }

    //! Arguments
    const QStringList &getExecutableArgs() const { return m_executableArgs; }

    //! Current command line
    QString getCmdLine() const;

protected:
    //! \copydoc QDialog::mousePressEvent
    virtual void mousePressEvent(QMouseEvent *event) override;

    //! \copydoc QDialog::mouseMoveEvent
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    QScopedPointer<Ui::CSwiftLauncher> ui;
    BlackMisc::CData<BlackCore::Data::TUpdateInfo> m_updateInfo { this, &CSwiftLauncher::ps_changedUpdateInfoCache }; //!< version cache
    BlackMisc::CData<BlackCore::Data::TLauncherSetup> m_setup { this }; //! setup, ie last user selection
    QString     m_executable;
    QStringList m_executableArgs;
    QTimer      m_checkTimer { this };
    int         m_startCoreWaitCycles = 0;
    int         m_startMappingToolWaitCycles = 0;
    int         m_startGuiWaitCycles = 0;
    bool        m_updateInfoLoaded = false;

    //! Get core mode
    BlackCore::CoreModes::CoreMode getCoreMode() const;

    //! select DBus address/mode
    QString getDBusAddress() const;

    //! Selected window mode
    BlackGui::CEnableForFramelessWindow::WindowMode getWindowMode() const;

    //! Init
    void init();

    //! style sheets
    void initStyleSheet();

    //! combobox for DBus
    void initDBusGui();

    //! Version string
    void initVersion();

    //! Log display
    void initLogDisplay();

    //! Latest news
    //! \sa CSwiftLauncher::ps_displayLatestNews
    void loadLatestNews();

    //! Load credits and legal info
    void loadAbout();

    //! Start the core
    void startSwiftCore();

    //! Set executable for swift data
    void setSwiftDataExecutable();

    //! Set executable for swift GUI
    bool setSwiftGuiExecutable();

    //! Can DBus server be connected
    bool canConnectSwiftOnDBusServer(const QString &dbusAddress, QString &msg) const;

    //! Standalone GUI selected
    bool isStandaloneGuiSelected() const;

    //! Set default
    void setDefaults();

    //! Save state
    void saveSetup();

    //! Command line
    static QString toCmdLine(const QString &exe, const QStringList &exeArgs);

private slots:
    //! Load latest version
    void ps_loadSetup();

    //! Loaded latest version
    void ps_loadedUpdateInfo(bool success);

    //! Display latest news
    void ps_displayLatestNews(QNetworkReply *reply);

    //! Cache values have been changed
    void ps_changedUpdateInfoCache();

    //! Start button pressed
    void ps_startButtonPressed();

    //! Changed selection
    void ps_dbusServerAddressSelectionChanged(const QString &currentText);

    //! DBus server mode selected
    void ps_dbusServerModeSelected(bool selected);

    //! Display status message as overlay
    void ps_showStatusMessage(const BlackMisc::CStatusMessage &msg);

    //! Append status message
    void ps_appendLogMessage(const BlackMisc::CStatusMessage &message);

    //! Append status messages
    void ps_appendLogMessages(const BlackMisc::CStatusMessageList &messages);

    //! Show set main page
    void ps_showMainPage();

    //! Tab changed
    void ps_tabChanged(int current);

    //! Show the log page
    void ps_showLogPage();

    //! Check if applicationas are already running
    void ps_checkRunningApplicationsAndCore();
};

#endif // guard
