/* Copyright (C) 2019
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution. No part of swift project, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 */

#ifndef BLACKMISC_XSWIFTBUS_CSETTINGS_H
#define BLACKMISC_XSWIFTBUS_CSETTINGS_H

#include "blackmisc/simulation/settings/xswiftbussettingsqtfree.h"
#include <string>
#include <mutex>

namespace XSwiftBus
{
    /*!
     * XSwiftBus/swift side settings class, JSON capable, shared among all services
     */
    class CSettings : public BlackMisc::Simulation::Settings::CXSwiftBusSettingsQtFree
    {
    public:
        //! Constructor.
        CSettings();

        //! JSON constructor
        CSettings(const std::string &json);

        //! Destructor;
        virtual ~CSettings() {}
    };

    //! Something owning the settings
    class ISettingsProvider
    {
    public:
        //! By value
        //! \threadsafe
        CSettings getSettings() const;

        //! Set settings
        //! \threadsafe
        void setSettings(const CSettings &settings);

    private:
        mutable std::mutex m_settingsMutex;
        CSettings          m_pluginSettings; //!< owner of the settings
    };
} // ns

#endif
