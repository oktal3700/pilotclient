/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackmisc/simulation/aircraftmodelloader.h"
#include "blackmisc/simulation/fscommon/aircraftcfgparser.h"
#include "blackmisc/simulation/xplane/aircraftmodelloaderxplane.h"
#include "blackmisc/simulation/xplane/xplaneutil.h"
#include "blackmisc/directoryutils.h"
#include "blackmisc/compare.h"
#include "blackmisc/logmessage.h"

#include <QDir>
#include <Qt>
#include <QtGlobal>
#include <QMap>

using namespace BlackMisc;
using namespace BlackMisc::Simulation::Data;
using namespace BlackMisc::Simulation::Settings;
using namespace BlackMisc::Simulation::FsCommon;
using namespace BlackMisc::Simulation::XPlane;

namespace BlackMisc
{
    namespace Simulation
    {
        IAircraftModelLoader::IAircraftModelLoader(const CSimulatorInfo &simulator)
        {
            Q_ASSERT_X(simulator.isSingleSimulator(), Q_FUNC_INFO, "Only one simulator per loader");
            m_caches.setCurrentSimulator(simulator);
            this->setObjectInfo(simulator);

            // first connect is an internal connection to log info about load status
            connect(this, &IAircraftModelLoader::loadingFinished, this, &IAircraftModelLoader::onLoadingFinished, Qt::QueuedConnection);
            connect(&m_caches, &IMultiSimulatorModelCaches::cacheChanged, this, &IAircraftModelLoader::onCacheChanged);
            connect(&m_settings, &CMultiSimulatorSettings::settingsChanged, this, &IAircraftModelLoader::onSettingsChanged);
        }

        const QString &IAircraftModelLoader::enumToString(IAircraftModelLoader::LoadFinishedInfo info)
        {
            static const QString loaded("cache loaded");
            static const QString skipped("loading skipped");
            static const QString parsed("parsed data");

            switch (info)
            {
            case CacheLoaded: return loaded;
            case LoadingSkipped: return skipped;
            case ParsedData: return parsed;
            default: break;
            }

            static const QString unknown("??");
            return unknown;
        }

        const QString &IAircraftModelLoader::enumToString(IAircraftModelLoader::LoadModeFlag modeFlag)
        {
            static const QString notSet("not set");
            static const QString directly("load directly");
            static const QString background("load in background");
            static const QString cacheFirst("cache first");
            static const QString cacheSkipped("cache skipped");
            static const QString cacheOnly("cacheOnly");

            switch (modeFlag)
            {
            case NotSet: return notSet;
            case LoadDirectly: return directly;
            case LoadInBackground: return background;
            case CacheFirst: return cacheFirst;
            case CacheSkipped: return cacheSkipped;
            case CacheOnly: return cacheOnly;
            default: break;
            }

            static const QString unknown("??");
            return unknown;
        }

        QString IAircraftModelLoader::enumToString(LoadMode mode)
        {
            QStringList modes;
            if (mode.testFlag(NotSet)) modes << enumToString(NotSet);
            if (mode.testFlag(LoadDirectly)) modes << enumToString(LoadDirectly);
            if (mode.testFlag(LoadInBackground)) modes << enumToString(LoadInBackground);
            if (mode.testFlag(CacheFirst)) modes << enumToString(CacheFirst);
            if (mode.testFlag(CacheSkipped)) modes << enumToString(CacheSkipped);
            return modes.join(", ");
        }

        bool IAircraftModelLoader::needsCacheSynchronized(LoadMode mode)
        {
            return mode.testFlag(CacheFirst) || mode.testFlag(CacheOnly);
        }

        IAircraftModelLoader::~IAircraftModelLoader()
        {
            this->gracefulShutdown();
        }

        const CLogCategoryList &IAircraftModelLoader::getLogCategories()
        {
            static const CLogCategoryList cats({ CLogCategory::modelLoader() });
            return cats;
        }

        CStatusMessage IAircraftModelLoader::setCachedModels(const CAircraftModelList &models, const CSimulatorInfo &simulator)
        {
            const CSimulatorInfo usedSimulator = simulator.isSingleSimulator() ? simulator : this->getSimulator(); // support default value
            this->setObjectInfo(usedSimulator);
            return m_caches.setCachedModels(models, usedSimulator);
        }

        CStatusMessage IAircraftModelLoader::replaceOrAddCachedModels(const CAircraftModelList &models, const CSimulatorInfo &simulator)
        {
            if (models.isEmpty()) { return CStatusMessage(this, CStatusMessage::SeverityInfo, "No data"); }
            const CSimulatorInfo sim = simulator.isSingleSimulator() ? simulator : this->getSimulator(); // support default values
            this->setObjectInfo(sim);
            CAircraftModelList allModels(m_caches.getSynchronizedCachedModels(sim));
            const int c = allModels.replaceOrAddModelsWithString(models, Qt::CaseInsensitive);
            if (c > 0)
            {
                return this->setCachedModels(allModels, sim);
            }
            else
            {
                return CStatusMessage(this, CStatusMessage::SeverityInfo, "No data changed");
            }
        }

        void IAircraftModelLoader::onLoadingFinished(const CStatusMessageList &statusMsgs, const CSimulatorInfo &simulator, LoadFinishedInfo info)
        {
            Q_UNUSED(info);
            this->setObjectInfo(simulator);

            // remark: in the past status used to be bool, now it is CStatusMessage
            // so there is some redundancy here between status and m_loadingMessages
            m_loadingInProgress = false;

            const QMap<int, int> counts = statusMsgs.countSeverities();
            const int errors = counts.value(SeverityError);
            const int warnings = counts.value(SeverityWarning);

            if (statusMsgs.hasWarningOrErrorMessages())
            {
                CLogMessage(this).log(m_loadingMessages.worstSeverity(),
                                      "Message loading produced %1 error and %2 warning messages") << errors << warnings;
            }
            else
            {
                CLogMessage(this).info("Loading finished, success for '%1'") << simulator.toQString();
            }
        }

        void IAircraftModelLoader::onCacheChanged(const CSimulatorInfo &simInfo)
        {
            // this detects a loaded cache elsewhere
            Q_UNUSED(simInfo);
        }

        void IAircraftModelLoader::onSettingsChanged(const CSimulatorInfo &simInfo)
        {
            // this detects changed settings elsewhere
            Q_UNUSED(simInfo);
        }

        QStringList IAircraftModelLoader::getInitializedModelDirectories(const QStringList &modelDirectories, const CSimulatorInfo &simulator) const
        {
            QStringList modelDirs = modelDirectories.isEmpty() ? m_settings.getModelDirectoriesOrDefault(simulator) : modelDirectories;
            modelDirs = CFileUtils::fixWindowsUncPaths(modelDirs);
            return CDirectoryUtils::getExistingUnemptyDirectories(modelDirs);
        }

        void IAircraftModelLoader::setObjectInfo(const CSimulatorInfo &simulatorInfo)
        {
            this->setObjectName("Model loader for: '" + simulatorInfo.toQString(true) + "'");
        }

        QStringList IAircraftModelLoader::getModelDirectoriesOrDefault() const
        {
            const QStringList mdirs = m_settings.getModelDirectoriesOrDefault(this->getSimulator());
            return mdirs;
        }

        QString IAircraftModelLoader::getFirstModelDirectoryOrDefault() const
        {
            const QString md = m_settings.getFirstModelDirectoryOrDefault(this->getSimulator());
            return md;
        }

        QStringList IAircraftModelLoader::getModelExcludeDirectoryPatterns() const
        {
            return m_settings.getModelExcludeDirectoryPatternsOrDefault(this->getSimulator());
        }

        CAircraftModelList IAircraftModelLoader::getAircraftModels() const
        {
            return m_caches.getCurrentCachedModels();
        }

        CAircraftModelList IAircraftModelLoader::getCachedAircraftModels(const CSimulatorInfo &simulator) const
        {
            return m_caches.getCachedModels(simulator);
        }

        QDateTime IAircraftModelLoader::getCacheTimestamp() const
        {
            return m_caches.getCurrentCacheTimestamp();
        }

        bool IAircraftModelLoader::hasCachedData() const
        {
            return !m_caches.getCurrentCachedModels().isEmpty();
        }

        CStatusMessage IAircraftModelLoader::clearCache()
        {
            return m_caches.clearCachedModels(m_caches.getCurrentSimulator());
        }

        CStatusMessage IAircraftModelLoader::clearCache(const CSimulatorInfo &simulator)
        {
            return m_caches.clearCachedModels(simulator);
        }

        void IAircraftModelLoader::startLoading(LoadMode mode, const ModelConsolidationCallback &modelConsolidation, const QStringList &modelDirectories)
        {
            if (m_loadingInProgress) { return; }
            m_loadingInProgress = true;
            m_loadingMessages.clear();

            const CSimulatorInfo simulator = this->getSimulator();
            const bool needsCacheSynced = IAircraftModelLoader::needsCacheSynchronized(mode);
            if (needsCacheSynced) { m_caches.synchronizeCache(simulator); }

            const bool useCachedData = !mode.testFlag(CacheSkipped) && this->hasCachedData();
            if (useCachedData && (mode.testFlag(CacheFirst) || mode.testFlag(CacheOnly)))
            {
                // we just just cache data
                static const CStatusMessage status(this, CStatusMessage::SeverityInfo, "Using cached data");
                emit loadingFinished(status, this->getSimulator(), CacheLoaded);
                return;
            }
            if (mode.testFlag(CacheOnly))
            {
                // only cache, but we did not find any data yet (still in progress?)
                // here we rely on the cache load slot, no need to emit here, will
                // be done later in ps_cacheChanged. An alternative was to sync cache here
                m_loadingInProgress = false;
                return;
            }

            // really load from disk?
            const QStringList modelDirs = this->getInitializedModelDirectories(modelDirectories, simulator);
            if (m_skipLoadingEmptyModelDir && modelDirs.isEmpty())
            {
                const CStatusMessage status = CStatusMessage(this, CStatusMessage::SeverityWarning,
                                              "Empty or not existing '%1' directory '%2', skipping read")
                                              << simulator.toQString() << modelDirectories.join(", ");
                m_loadingMessages.push_back(status);
                emit this->loadingFinished(m_loadingMessages, simulator, LoadingSkipped);
                return;
            }

            this->setObjectInfo(simulator);
            this->startLoadingFromDisk(mode, modelConsolidation, modelDirs);
        }

        const CSimulatorInfo IAircraftModelLoader::getSimulator() const
        {
            return m_caches.getCurrentSimulator();
        }

        QString IAircraftModelLoader::getSimulatorAsString() const
        {
            return this->getSimulator().toQString();
        }

        bool IAircraftModelLoader::supportsSimulator(const CSimulatorInfo &simulator)
        {
            return getSimulator().matchesAny(simulator);
        }

        void IAircraftModelLoader::cancelLoading()
        {
            if (!m_loadingInProgress) { return; }
            m_cancelLoading = true;
        }

        void IAircraftModelLoader::gracefulShutdown()
        {
            this->cancelLoading();
            m_loadingInProgress = true; // avoids further startups
        }

        QString IAircraftModelLoader::getModelCacheInfoString() const
        {
            return m_caches.getInfoString();
        }

        QString IAircraftModelLoader::getModelCacheInfoStringFsFamily() const
        {
            return m_caches.getInfoStringFsFamily();
        }

        QString IAircraftModelLoader::getModelCacheCountAndTimestamp() const
        {
            return m_caches.getCacheCountAndTimestamp(this->getSimulator());
        }

        QString IAircraftModelLoader::getModelCacheCountAndTimestamp(const CSimulatorInfo &simulator) const
        {
            return m_caches.getCacheCountAndTimestamp(simulator);
        }

        CSpecializedSimulatorSettings IAircraftModelLoader::getCurrentSimulatorSettings() const
        {
            return m_settings.getSpecializedSettings(this->getSimulator());
        }

        void IAircraftModelLoader::synchronizeModelCache(const CSimulatorInfo &simulator)
        {
            m_caches.synchronizeCache(simulator);
        }

        std::unique_ptr<IAircraftModelLoader> IAircraftModelLoader::createModelLoader(const CSimulatorInfo &simulator)
        {
            Q_ASSERT_X(simulator.isSingleSimulator(), Q_FUNC_INFO, "Single simulator");
            std::unique_ptr<IAircraftModelLoader> loader;
            if (simulator.isXPlane())
            {
                loader = std::make_unique<CAircraftModelLoaderXPlane>();
            }
            else
            {
                loader = CAircraftCfgParser::createModelLoader(simulator);
            }

            if (!loader) { return loader; }

            // make sure the cache is really available, normally this happens in the constructor
            if (loader->getSimulator() != simulator)
            {
                loader->m_caches.setCurrentSimulator(simulator); // mark current simulator and sync caches
            }
            return loader;
        }
    } // ns
} // ns
