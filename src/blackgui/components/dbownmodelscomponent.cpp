/* Copyright (C) 2016
 * swift project Community / Contributors
 *
 * This file is part of swift Project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackcore/webdataservices.h"
#include "blackcore/db/databaseutils.h"
#include "blackgui/components/dbownmodelscomponent.h"
#include "blackgui/components/simulatorselector.h"
#include "blackgui/guiapplication.h"
#include "blackgui/menus/aircraftmodelmenus.h"
#include "blackgui/menus/menuaction.h"
#include "blackgui/models/aircraftmodellistmodel.h"
#include "blackgui/views/aircraftmodelview.h"
#include "blackmisc/icons.h"
#include "blackmisc/logmessage.h"
#include "blackmisc/statusmessage.h"
#include "ui_dbownmodelscomponent.h"

#include <QAction>
#include <QIcon>
#include <QtGlobal>
#include <QFileDialog>
#include <QMessageBox>

using namespace BlackMisc;
using namespace BlackMisc::Simulation;
using namespace BlackCore::Db;
using namespace BlackGui::Menus;
using namespace BlackGui::Views;
using namespace BlackGui::Models;

namespace BlackGui
{
    namespace Components
    {
        CDbOwnModelsComponent::CDbOwnModelsComponent(QWidget *parent) :
            COverlayMessagesFrame(parent),
            ui(new Ui::CDbOwnModelsComponent)
        {
            ui->setupUi(this);
            ui->comp_SimulatorSelector->setMode(CSimulatorSelector::RadioButtons);
            ui->tvp_OwnAircraftModels->setAircraftModelMode(CAircraftModelListModel::OwnAircraftModelMappingTool);
            ui->tvp_OwnAircraftModels->addFilterDialog();
            ui->tvp_OwnAircraftModels->setDisplayAutomatically(true);
            ui->tvp_OwnAircraftModels->setCustomMenu(new CLoadModelsMenu(this, true));
            ui->tvp_OwnAircraftModels->setSimulatorForLoading(ui->comp_SimulatorSelector->getValue());

            connect(ui->tvp_OwnAircraftModels, &CAircraftModelView::requestUpdate, this, &CDbOwnModelsComponent::requestOwnModelsUpdate);
            connect(ui->tvp_OwnAircraftModels, &CAircraftModelView::jsonLoadCompleted, this, &CDbOwnModelsComponent::onViewDiskLoadingFinished, Qt::QueuedConnection);
            connect(ui->comp_SimulatorSelector, &CSimulatorSelector::changed, this, &CDbOwnModelsComponent::onSimulatorSelectorChanged);
            connect(&CMultiAircraftModelLoaderProvider::multiModelLoaderInstance(), &CMultiAircraftModelLoaderProvider::loadingFinished, this, &CDbOwnModelsComponent::onModelLoaderLoadingFinished, Qt::QueuedConnection);
            connect(&CMultiAircraftModelLoaderProvider::multiModelLoaderInstance(), &CMultiAircraftModelLoaderProvider::diskLoadingStarted, this, &CDbOwnModelsComponent::onModelLoaderDiskLoadingStarted, Qt::QueuedConnection);
            connect(&CMultiAircraftModelLoaderProvider::multiModelLoaderInstance(), &CMultiAircraftModelLoaderProvider::cacheChanged, this, &CDbOwnModelsComponent::onCacheChanged, Qt::QueuedConnection);

            // Last selection isPinned -> no sync needed
            ui->comp_SimulatorSelector->setRememberSelectionAndSetToLastSelection();
            const CSimulatorInfo simulator = ui->comp_SimulatorSelector->getValue();
            if (simulator.isSingleSimulator())
            {
                m_simulator = simulator;
                const bool success = this->initModelLoader(simulator, IAircraftModelLoader::CacheOnly);
                if (!success)
                {
                    CLogMessage(this).error("Init of model loader failed in component");
                }
            }

            // menu
            ui->tvp_OwnAircraftModels->setCustomMenu(new CConsolidateWithDbDataMenu(ui->tvp_OwnAircraftModels, this, false));
        }

        CDbOwnModelsComponent::~CDbOwnModelsComponent()
        {
            // void
        }

        const CLogCategoryList &CDbOwnModelsComponent::getLogCategories()
        {
            static const CLogCategoryList l({ CLogCategory::modelGui(), CLogCategory::guiComponent() });
            return l;
        }

        CAircraftModelView *CDbOwnModelsComponent::view() const
        {
            return ui->tvp_OwnAircraftModels;
        }

        CAircraftModelListModel *CDbOwnModelsComponent::model() const
        {
            return ui->tvp_OwnAircraftModels->derivedModel();
        }

        bool CDbOwnModelsComponent::requestModelsInBackground(const CSimulatorInfo &simulator, bool onlyIfNotEmpty)
        {
            this->setSimulator(simulator);
            if (onlyIfNotEmpty && this->getOwnModelsCount() > 0) { return false; }
            const IAircraftModelLoader::LoadMode mode = onlyIfNotEmpty ? IAircraftModelLoader::InBackgroundNoCache : IAircraftModelLoader::LoadInBackground;
            this->requestSimulatorModels(simulator, mode);
            return true;
        }

        CAircraftModel CDbOwnModelsComponent::getOwnModelForModelString(const QString &modelString) const
        {
            return this->getOwnModels().findFirstByModelStringOrDefault(modelString);
        }

        CAircraftModelList CDbOwnModelsComponent::getOwnModels() const
        {
            return this->getOwnCachedModels(this->getOwnModelsSimulator());
        }

        CAircraftModelList CDbOwnModelsComponent::getOwnCachedModels(const CSimulatorInfo &simulator) const
        {
            static const CAircraftModelList empty;
            if (!m_modelLoader) { return empty; }
            return m_modelLoader->getCachedModels(simulator);
        }

        CAircraftModelList CDbOwnModelsComponent::getOwnSelectedModels() const
        {
            return ui->tvp_OwnAircraftModels->selectedObjects();
        }

        const CSimulatorInfo CDbOwnModelsComponent::getOwnModelsSimulator() const
        {
            return ui->comp_SimulatorSelector->getValue();
        }

        void CDbOwnModelsComponent::setSimulator(const CSimulatorInfo &simulator)
        {
            Q_ASSERT_X(simulator.isSingleSimulator(), Q_FUNC_INFO, "Need single simulator");
            if (m_simulator == simulator) { return; }

            // changed simulator
            m_simulator = simulator;
            this->requestSimulatorModelsWithCacheInBackground(simulator);
            ui->comp_SimulatorSelector->setValue(simulator);
            ui->le_Simulator->setText(simulator.toQString());
            ui->tvp_OwnAircraftModels->setSimulatorForLoading(simulator);
        }

        void CDbOwnModelsComponent::onSimulatorSelectorChanged()
        {
            const CSimulatorInfo simulator(ui->comp_SimulatorSelector->getValue());
            this->setSimulator(simulator);
        }

        int CDbOwnModelsComponent::getOwnModelsCount() const
        {
            if (!m_modelLoader) { return 0; }
            return m_modelLoader->getCachedModelsCount(this->getOwnModelsSimulator());
        }

        QString CDbOwnModelsComponent::getInfoString() const
        {
            if (!m_modelLoader) { return ""; }
            return m_modelLoader->getInfoString();
        }

        QString CDbOwnModelsComponent::getInfoStringFsFamily() const
        {
            if (!m_modelLoader) { return ""; }
            return m_modelLoader->getInfoStringFsFamily();
        }

        CStatusMessage CDbOwnModelsComponent::updateViewAndCache(const CAircraftModelList &models)
        {
            const CStatusMessage m  = m_modelLoader->setCachedModels(models, this->getOwnModelsSimulator());
            if (m.isSuccess())
            {
                ui->tvp_OwnAircraftModels->updateContainerMaybeAsync(models);
            }
            return m;
        }

        void CDbOwnModelsComponent::gracefulShutdown()
        {
            // void
        }

        void CDbOwnModelsComponent::setModelsForSimulator(const CAircraftModelList &models, const CSimulatorInfo &simulator)
        {
            if (!m_modelLoader) { return; }
            m_modelLoader->setCachedModels(models, simulator);
            ui->tvp_OwnAircraftModels->replaceOrAddModelsWithString(models);
        }

        int CDbOwnModelsComponent::updateModelsForSimulator(const CAircraftModelList &models, const CSimulatorInfo &simulator)
        {
            if (!m_modelLoader) { return 0; }
            const int c = m_modelLoader->updateModelsForSimulator(models, simulator);
            const CAircraftModelList allModels(m_modelLoader->getCachedModels(simulator));
            ui->tvp_OwnAircraftModels->updateContainerMaybeAsync(allModels);
            return c;
        }

        bool CDbOwnModelsComponent::initModelLoader(const CSimulatorInfo &simulator, IAircraftModelLoader::LoadMode load)
        {
            // called when simulator is changed / init
            Q_ASSERT_X(simulator.isSingleSimulator(), Q_FUNC_INFO, "Need single simulator");

            // already loaded
            if (!m_modelLoader || !m_modelLoader->supportsSimulator(simulator))
            {
                m_modelLoader = CMultiAircraftModelLoaderProvider::multiModelLoaderInstance().loaderInstance(simulator);
                if (m_modelLoader) { m_modelLoader->startLoading(load); }
            }
            this->setSaveFileName(simulator);
            return m_modelLoader && m_modelLoader->supportsSimulator(simulator);
        }

        void CDbOwnModelsComponent::setSaveFileName(const CSimulatorInfo &sim)
        {
            Q_ASSERT_X(sim.isSingleSimulator(), Q_FUNC_INFO, "Need single simulator");
            const QString n("simulator models " + sim.toQString(true));
            ui->tvp_OwnAircraftModels->setSaveFileName(n);
        }

        QString CDbOwnModelsComponent::directorySelector(const CSimulatorInfo &simulatorInfo)
        {
            const QString text("Open directory (%1)");
            const QString dir = QFileDialog::getExistingDirectory(nullptr, text.arg(simulatorInfo.toQString()), "",
                                QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
            return dir;
        }

        void CDbOwnModelsComponent::CLoadModelsMenu::customMenu(CMenuActions &menuActions)
        {
            if (!sGui || sGui->isShuttingDown()) { return; }

            // for the moment I use all sims, I could restrict to CSimulatorInfo::getLocallyInstalledSimulators();
            const CSimulatorInfo sims =  CSimulatorInfo::allSimulators();
            const bool noSims = sims.isNoSimulator() || sims.isUnspecified();
            if (!noSims)
            {
                CDbOwnModelsComponent *ownModelsComp = qobject_cast<CDbOwnModelsComponent *>(this->parent());
                Q_ASSERT_X(ownModelsComp, Q_FUNC_INFO, "Cannot access parent");

                if (m_loadActions.isEmpty()) { m_loadActions = QList<QAction *>({nullptr, nullptr, nullptr, nullptr}); }
                menuActions.addMenuSimulator();
                if (sims.isFSX())
                {
                    if (!m_loadActions[0])
                    {
                        m_loadActions[0] = new QAction(CIcons::appModels16(), "FSX models", this);
                        connect(m_loadActions[0], &QAction::triggered, ownModelsComp, [ownModelsComp](bool checked)
                        {
                            Q_UNUSED(checked);
                            ownModelsComp->requestSimulatorModelsWithCacheInBackground(CSimulatorInfo::fsx());
                        });
                    }
                    menuActions.addAction(m_loadActions[0], CMenuAction::pathSimulator());
                }
                if (sims.isP3D())
                {
                    if (!m_loadActions[1])
                    {
                        m_loadActions[1] = new QAction(CIcons::appModels16(), "P3D models", this);
                        connect(m_loadActions[1], &QAction::triggered, ownModelsComp, [ownModelsComp](bool checked)
                        {
                            Q_UNUSED(checked);
                            ownModelsComp->requestSimulatorModelsWithCacheInBackground(CSimulatorInfo::p3d());
                        });
                    }
                    menuActions.addAction(m_loadActions[1], CMenuAction::pathSimulator());
                }
                if (sims.isFS9())
                {
                    if (!m_loadActions[2])
                    {
                        m_loadActions[2] = new QAction(CIcons::appModels16(), "FS9 models", this);
                        connect(m_loadActions[2], &QAction::triggered, ownModelsComp, [ownModelsComp](bool checked)
                        {
                            Q_UNUSED(checked);
                            ownModelsComp->requestSimulatorModelsWithCacheInBackground(CSimulatorInfo::fs9());
                        });
                    }
                    menuActions.addAction(m_loadActions[2], CMenuAction::pathSimulator());
                }
                if (sims.isXPlane())
                {
                    if (!m_loadActions[3])
                    {
                        m_loadActions[3] = new QAction(CIcons::appModels16(), "XPlane models", this);
                        connect(m_loadActions[3], &QAction::triggered, ownModelsComp, [ownModelsComp](bool checked)
                        {
                            Q_UNUSED(checked);
                            ownModelsComp->requestSimulatorModelsWithCacheInBackground(CSimulatorInfo::xplane());
                        });
                    }
                    menuActions.addAction(m_loadActions[3], CMenuAction::pathSimulator());
                }

                // with models loaded I allow a refresh reload
                // I need those models because I want to merge with DB data in the loader
                if (sGui && sGui->getWebDataServices() && sGui->getWebDataServices()->getModelsCount() > 0)
                {
                    if (m_reloadActions.isEmpty()) { m_reloadActions = QList<QAction *>({nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}); }
                    menuActions.addMenu(CIcons::refresh16(), "Force model reload", CMenuAction::pathSimulatorModelsReload());
                    if (sims.isFSX())
                    {
                        if (!m_reloadActions[0])
                        {
                            m_reloadActions[0] = new QAction(CIcons::appModels16(), "FSX models", this);
                            connect(m_reloadActions[0], &QAction::triggered, ownModelsComp, [ownModelsComp](bool checked)
                            {
                                Q_UNUSED(checked);
                                ownModelsComp->requestSimulatorModels(CSimulatorInfo::fsx(), IAircraftModelLoader::InBackgroundNoCache);
                            });

                            m_reloadActions[1] = new QAction(CIcons::appModels16(), "FSX models from directory", this);
                            connect(m_reloadActions[1], &QAction::triggered, ownModelsComp, [ownModelsComp](bool checked)
                            {
                                Q_UNUSED(checked);
                                const CSimulatorInfo sim(CSimulatorInfo::FSX);
                                const QString dir = CDbOwnModelsComponent::directorySelector(sim);
                                if (!dir.isEmpty())
                                {
                                    ownModelsComp->requestSimulatorModels(sim, IAircraftModelLoader::InBackgroundNoCache, QStringList(dir));
                                }
                            });
                        }
                        menuActions.addAction(m_reloadActions[0], CMenuAction::pathSimulatorModelsReload());
                        menuActions.addAction(m_reloadActions[1], CMenuAction::pathSimulatorModelsReload());
                    }
                    if (sims.isP3D())
                    {
                        if (!m_reloadActions[2])
                        {
                            m_reloadActions[2] = new QAction(CIcons::appModels16(), "P3D models", this);
                            connect(m_reloadActions[2], &QAction::triggered, ownModelsComp, [ownModelsComp](bool checked)
                            {
                                Q_UNUSED(checked);
                                ownModelsComp->requestSimulatorModels(CSimulatorInfo::p3d(), IAircraftModelLoader::InBackgroundNoCache);
                            });

                            m_reloadActions[3] = new QAction(CIcons::appModels16(), "P3D models from directoy", this);
                            connect(m_reloadActions[3], &QAction::triggered, ownModelsComp, [ownModelsComp](bool checked)
                            {
                                Q_UNUSED(checked);
                                const CSimulatorInfo sim(CSimulatorInfo::P3D);
                                const QString dir = CDbOwnModelsComponent::directorySelector(sim);
                                if (!dir.isEmpty())
                                {
                                    ownModelsComp->requestSimulatorModels(sim, IAircraftModelLoader::InBackgroundNoCache, QStringList(dir));
                                }
                            });
                        }
                        menuActions.addAction(m_reloadActions[2], CMenuAction::pathSimulatorModelsReload());
                        menuActions.addAction(m_reloadActions[3], CMenuAction::pathSimulatorModelsReload());
                    }
                    if (sims.isFS9())
                    {
                        if (!m_reloadActions[4])
                        {
                            m_reloadActions[4] = new QAction(CIcons::appModels16(), "FS9 models", this);
                            connect(m_reloadActions[4], &QAction::triggered, ownModelsComp, [ownModelsComp](bool checked)
                            {
                                Q_UNUSED(checked);
                                ownModelsComp->requestSimulatorModels(CSimulatorInfo::fs9(), IAircraftModelLoader::InBackgroundNoCache);
                            });

                            m_reloadActions[5] = new QAction(CIcons::appModels16(), "FS9 models from directoy", this);
                            connect(m_reloadActions[5], &QAction::triggered, ownModelsComp, [ownModelsComp](bool checked)
                            {
                                Q_UNUSED(checked);
                                const CSimulatorInfo sim(CSimulatorInfo::FS9);
                                const QString dir = CDbOwnModelsComponent::directorySelector(sim);
                                if (!dir.isEmpty())
                                {
                                    ownModelsComp->requestSimulatorModels(sim, IAircraftModelLoader::InBackgroundNoCache, QStringList(dir));
                                }
                            });
                        }
                        menuActions.addAction(m_reloadActions[4], CMenuAction::pathSimulatorModelsReload());
                        menuActions.addAction(m_reloadActions[5], CMenuAction::pathSimulatorModelsReload());
                    }
                    if (sims.isXPlane())
                    {
                        if (!m_reloadActions[6])
                        {
                            m_reloadActions[6] = new QAction(CIcons::appModels16(), "XPlane models", this);
                            connect(m_reloadActions[6], &QAction::triggered, ownModelsComp, [ownModelsComp](bool checked)
                            {
                                Q_UNUSED(checked);
                                ownModelsComp->requestSimulatorModels(CSimulatorInfo::xplane(), IAircraftModelLoader::InBackgroundNoCache);
                            });
                            m_reloadActions[7] = new QAction(CIcons::appModels16(), "XPlane models from directoy", this);
                            connect(m_reloadActions[7], &QAction::triggered, ownModelsComp, [ownModelsComp](bool checked)
                            {
                                Q_UNUSED(checked);
                                const CSimulatorInfo sim(CSimulatorInfo::XPLANE);
                                const QString dir = CDbOwnModelsComponent::directorySelector(sim);
                                if (!dir.isEmpty())
                                {
                                    ownModelsComp->requestSimulatorModels(sim, IAircraftModelLoader::InBackgroundNoCache, QStringList(dir));
                                }
                            });
                        }
                        menuActions.addAction(m_reloadActions[6], CMenuAction::pathSimulatorModelsReload());
                        menuActions.addAction(m_reloadActions[7], CMenuAction::pathSimulatorModelsReload());
                    }

                }
                else
                {
                    // dummy action grayed out
                    CMenuAction a = menuActions.addAction(CIcons::refresh16(), "Force model reload impossible, no DB data", CMenuAction::pathSimulator());
                    a.setActionEnabled(false); // gray out
                }

                if (m_clearCacheActions.isEmpty()) { m_clearCacheActions = QList<QAction *>({nullptr, nullptr, nullptr, nullptr}); }
                menuActions.addMenu(CIcons::delete16(), "Clear model caches", CMenuAction::pathSimulatorModelsClearCache());
                if (sims.isFSX())
                {
                    if (!m_clearCacheActions[0])
                    {
                        m_clearCacheActions[0] = new QAction(CIcons::appModels16(), "Clear FSX cache", this);
                        connect(m_loadActions[0], &QAction::triggered, ownModelsComp, [ownModelsComp](bool checked)
                        {
                            Q_UNUSED(checked);
                            ownModelsComp->clearSimulatorCache(CSimulatorInfo::fsx());
                        });
                    }
                    menuActions.addAction(m_clearCacheActions[0], CMenuAction::pathSimulatorModelsClearCache());
                }
                if (sims.isP3D())
                {
                    if (!m_clearCacheActions[1])
                    {
                        m_clearCacheActions[1] = new QAction(CIcons::appModels16(), "Clear P3D cache", this);
                        connect(m_clearCacheActions[1], &QAction::triggered, ownModelsComp, [ownModelsComp](bool checked)
                        {
                            Q_UNUSED(checked);
                            ownModelsComp->clearSimulatorCache(CSimulatorInfo::p3d());
                        });
                    }
                    menuActions.addAction(m_clearCacheActions[1], CMenuAction::pathSimulatorModelsClearCache());
                }
                if (sims.isFS9())
                {
                    if (!m_clearCacheActions[2])
                    {
                        m_clearCacheActions[2] = new QAction(CIcons::appModels16(), "Clear FS9 cache", this);
                        connect(m_clearCacheActions[2], &QAction::triggered, ownModelsComp, [ownModelsComp](bool checked)
                        {
                            Q_UNUSED(checked);
                            ownModelsComp->clearSimulatorCache(CSimulatorInfo::fs9());
                        });
                    }
                    menuActions.addAction(m_clearCacheActions[2], CMenuAction::pathSimulatorModelsClearCache());
                }
                if (sims.isXPlane())
                {
                    if (!m_clearCacheActions[3])
                    {
                        m_clearCacheActions[3] = new QAction(CIcons::appModels16(), "Clear XPlane cache", this);
                        connect(m_clearCacheActions[3], &QAction::triggered, ownModelsComp, [ownModelsComp](bool checked)
                        {
                            Q_UNUSED(checked);
                            ownModelsComp->clearSimulatorCache(CSimulatorInfo::xplane());
                        });
                    }
                    menuActions.addAction(m_clearCacheActions[3], CMenuAction::pathSimulatorModelsClearCache());
                }
            }
            this->nestedCustomMenu(menuActions);
        }

        void CDbOwnModelsComponent::requestOwnModelsUpdate()
        {
            if (!m_modelLoader) { return; }
            ui->tvp_OwnAircraftModels->updateContainerMaybeAsync(this->getOwnModels());
        }

        void CDbOwnModelsComponent::loadInstalledModels(const CSimulatorInfo &simulator, IAircraftModelLoader::LoadMode mode, const QStringList &modelDirectories)
        {
            if (!m_modelLoader) { return; }

            // here m_modelLoader is still the "current" loader
            if (m_modelLoader->isLoadingInProgress())
            {
                if (m_modelLoader->supportsSimulator(simulator))
                {
                    const CStatusMessage msg = CLogMessage(this).warning("Loading for '%1' already in progress, will NOT load.") << simulator.toQString();
                    this->showOverlayMessage(msg);
                    return;
                }
                else
                {
                    const CStatusMessage msg = CLogMessage(this).warning("Loading for another simulator '%1' already in progress. Loading might be slow.") << simulator.toQString();
                    this->showOverlayMessage(msg);
                }
            }

            if (!this->initModelLoader(simulator))
            {
                const CStatusMessage msg = CLogMessage(this).error("Cannot init model loader for %1") << simulator.toQString();
                this->showOverlayMessage(msg);
                return;
            }

            // Do not check for empty models die here, as depending on mode we could still load
            // will be checked in model loader
            CLogMessage(this).info("Starting loading for '%1' in mode '%2'") << simulator.toQString() << IAircraftModelLoader::enumToString(mode);
            ui->tvp_OwnAircraftModels->showLoadIndicator();
            Q_ASSERT_X(sGui && sGui->getWebDataServices(), Q_FUNC_INFO, "missing web data services");
            m_modelLoader->startLoading(mode, static_cast<int (*)(CAircraftModelList &, bool)>(&CDatabaseUtils::consolidateModelsWithDbData), modelDirectories);
        }

        void CDbOwnModelsComponent::onModelLoaderDiskLoadingStarted(const CSimulatorInfo &simulator, IAircraftModelLoader::LoadMode mode)
        {
            const CStatusMessage msg = CLogMessage(this).info("Started disk loading for '%1' in mode '%2'") << simulator.toQString(true) << IAircraftModelLoader::enumToString(mode);
            this->showOverlayMessage(msg, 5000);
        }

        void CDbOwnModelsComponent::onModelLoaderLoadingFinished(const CStatusMessageList &statusMessages, const CSimulatorInfo &simulator, IAircraftModelLoader::LoadFinishedInfo info)
        {
            Q_ASSERT_X(simulator.isSingleSimulator(), Q_FUNC_INFO, "Expect single simulator");
            if (statusMessages.isSuccess() && m_modelLoader)
            {
                const CAircraftModelList models(m_modelLoader->getCachedModels(simulator));
                const int modelsLoaded = models.size();
                ui->tvp_OwnAircraftModels->updateContainerMaybeAsync(models);
                if (modelsLoaded < 1)
                {
                    // loading ok, but no data
                    CLogMessage(this).warning("Loading completed for simulator '%1', but no models") << simulator;
                }

                emit this->successfullyLoadedModels(simulator);
            }
            else
            {
                ui->tvp_OwnAircraftModels->clear();
                CLogMessage(this).error("Loading of models failed, simulator '%1'") << simulator.toQString();
            }

            if (statusMessages.hasWarningOrErrorMessages())
            {
                this->showOverlayMessages(statusMessages);
            }

            // cache loads may occur in background, do not adjust UI settings
            if (info == IAircraftModelLoader::CacheLoaded) { return; }

            // parsed loads normally explicit displaying this simulator
            this->setSimulator(simulator);
        }

        void CDbOwnModelsComponent::onViewDiskLoadingFinished(const CStatusMessage &status)
        {
            if (status.isFailure()) { return; }
            QMessageBox msgBox(QMessageBox::Question, "Loaded models from disk", "Loaded models from disk file.\nSave to cache or just temporarily keep them?\n\nHint: Saving them will override the loaded models from the simulator.\nNormally you would not want that (cancel).", QMessageBox::Save | QMessageBox::Cancel, this);
            msgBox.setDefaultButton(QMessageBox::Cancel);
            const QMessageBox::StandardButton reply = static_cast<QMessageBox::StandardButton>(msgBox.exec());
            if (reply != QMessageBox::Cancel) { return; }
            const CAircraftModelList models = ui->tvp_OwnAircraftModels->container();
            if (models.isEmpty()) { return; }
            const CSimulatorInfo simulator = ui->comp_SimulatorSelector->getValue();
            m_modelLoader->setModelsForSimulator(models, simulator);
        }

        void CDbOwnModelsComponent::onCacheChanged(const CSimulatorInfo &simulator)
        {
            const CAircraftModelList models(m_modelLoader->getCachedModels(simulator));
            ui->tvp_OwnAircraftModels->updateContainerMaybeAsync(models);
        }

        void CDbOwnModelsComponent::requestSimulatorModels(const CSimulatorInfo &simulator, IAircraftModelLoader::LoadMode mode, const QStringList &modelDirectories)
        {
            this->loadInstalledModels(simulator, mode, modelDirectories);
        }

        void CDbOwnModelsComponent::requestSimulatorModelsWithCacheInBackground(const CSimulatorInfo &simulator)
        {
            this->requestSimulatorModels(simulator, IAircraftModelLoader::InBackgroundWithCache);
        }

        void CDbOwnModelsComponent::clearSimulatorCache(const CSimulatorInfo &simulator)
        {
            if (!m_modelLoader) { return; }
            m_modelLoader->clearCachedModels(simulator);
        }
    } // ns
} // ns
