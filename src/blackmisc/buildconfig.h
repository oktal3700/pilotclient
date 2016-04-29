/* Copyright (C) 2016
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKMISC_BUILDCONFIG_H
#define BLACKMISC_BUILDCONFIG_H

#include "blackmisc/blackmiscexport.h"

#include <QList>
#include <QString>

class QDateTime;
class QStringList;

namespace BlackMisc
{
    //! Build configuration
    class BLACKMISC_EXPORT CBuildConfig
    {
    public:
        //! with BlackCore?
        static bool isCompiledWithBlackCore();

        //! with BlackSound?
        static bool isCompiledWithBlackSound();

        //! with BlackInput?
        static bool isCompiledWithBlackInput();

        //! with FS9 support?
        static bool isCompiledWithFs9Support();

        //! with FSX support?
        static bool isCompiledWithFsxSupport();

        //! with P3D support
        static bool isCompiledWithP3DSupport();

        //! Compiled with any MS Flight Simulator support (P3D, FSX, FS9)
        static bool isCompiledWithMsFlightSimulatorSupport();

        //! with XPlane support?
        static bool isCompiledWithXPlaneSupport();

        //! with any simulator libraries
        static bool isCompiledWithFlightSimulatorSupport();

        //! with GUI?
        static bool isCompiledWithGui();

        //! Debug build?
        static bool isDebugBuild();

        //! Release build?
        static bool isReleaseBuild();

        //! Beta test?
        static bool isBetaTest();

        //! Can run in dev. environment
        static bool canRunInDeveloperEnvironment();

        //! Shipped version?
        static bool isShippedVersion();

        //! Running on Windows NT platform?
        static bool isRunningOnWindowsNtPlatform();

        //! Running on Mac OS X platform?
        static bool isRunningOnMacOSXPlatform();

        //! Running on Linux platform?
        static bool isRunningOnLinuxPlatform();

        //! Running on Unix (Linux or Mac OS X) platform
        static bool isRunningOnUnixPlatform();

        //! Application directory where current application is located
        //! \todo Move to to CApplication
        static const QString &getApplicationDir();

        //! Where resource files (static DB files, ...) etc are located
        static const QString &getSwiftResourceDir();

        //! Bootstrap resource directory
        static const QString &getBootstrapResourceFile();

        //! Where static DB files are located
        static const QString &getSwiftStaticDbFilesDir();

        //! Where sound files are located
        static const QString &getSoundFilesDir();

        //! Where qss files are located
        static const QString &getStylesheetsDir();

        //! Where images are located
        static const QString &getImagesDir();

        //! Directory where data can be stored
        static const QString &getDocumentationDirectory();

        //! Info string about compilation
        static const QString &compiledWithInfo(bool shortVersion = true);

        //! Executable name for swift GUI, no(!) appendix
        static const QString &swiftGuiExecutableName();

        //! Executable name for swift core, no(!) appendix
        static const QString &swiftCoreExecutableName();

        //! Executable name for swift data, no(!) appendix
        static const QString &swiftDataExecutableName();

        //! swift team default servers for DB, bootstrap etc.
        static const QStringList &swiftTeamDefaultServers();

        //! End of lifetime
        static const QDateTime &getEol();

        //! Lifetime ended?
        static bool isLifetimeExpired();

        //! Vatsim client id
        static int vatsimClientId();

        //! Vatsim client key
        static const QString &vatsimPrivateKey();
    };

    //! Version
    class BLACKMISC_EXPORT CVersion
    {
    public:
        //! Version info
        static const QString &version();

        //! Version major
        static int versionMajor();

        //! Version minor
        static int versionMinor();

        //! Version patch
        static int versionPatch();

        //! Is the given string representing a newer version?
        static bool isNewerVersion(const QString &versionString);

    private:
        //! Parts of version string 1.0.2
        static QList<int> getVersionParts(const QString &versionString);
    };
}

#endif // guard
