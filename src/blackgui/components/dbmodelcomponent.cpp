/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift Project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "dbmodelcomponent.h"
#include "dbmappingcomponent.h"
#include "blackgui/stylesheetutility.h"
#include "blackgui/shortcut.h"
#include "blackmisc/simulation/aircraftmodel.h"
#include "ui_dbmodelcomponent.h"
#include <functional>

using namespace BlackMisc;
using namespace BlackMisc::Network;
using namespace BlackMisc::Simulation;
using namespace BlackGui::Views;
using namespace BlackGui::Models;

namespace BlackGui
{
    namespace Components
    {
        CDbModelComponent::CDbModelComponent(QWidget *parent) :
            QFrame(parent),
            CDbMappingComponentAware(parent),
            ui(new Ui::CDbModelComponent)
        {
            ui->setupUi(this);
            this->setViewWithIndicator(this->ui->tvp_AircraftModel);
            this->ui->tvp_AircraftModel->setAircraftModelMode(CAircraftModelListModel::Database);
            connect(this->ui->tvp_AircraftModel, &CAircraftModelView::requestNewBackendData, this, &CDbModelComponent::ps_reload);
            connect(this->ui->tvp_AircraftModel, &CAircraftModelView::requestStash, this, &CDbModelComponent::requestStash);
            connect(&CStyleSheetUtility::instance(), &CStyleSheetUtility::styleSheetsChanged, this, &CDbModelComponent::ps_onStyleSheetChanged);

            // configure view
            this->ui->tvp_AircraftModel->setFilterWidget(this->ui->filter_AircraftModelFilter);
            this->ui->tvp_AircraftModel->allowDragDropValueObjects(true, false);
            this->ui->tvp_AircraftModel->menuAddItems(CAircraftModelView::MenuCanStashModels);
        }

        CDbModelComponent::~CDbModelComponent()
        {
            gracefulShutdown();
        }

        void CDbModelComponent::setProvider(IWebDataServicesProvider *webDataReaderProvider)
        {
            CWebDataServicesAware::setProvider(webDataReaderProvider);
            webDataReaderProvider->connectDataReadSignal(
                this,
                std::bind(&CDbModelComponent::ps_modelsRead, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
            );
            int c = getModelsCount();
            if (c > 0)
            {
                ps_modelsRead(CEntityFlags::ModelEntity, CEntityFlags::ReadFinished, c);
            }
        }

        bool CDbModelComponent::hasModels() const
        {
            return !this->ui->tvp_AircraftModel->isEmpty();
        }

        void CDbModelComponent::requestUpdatedData()
        {
            QDateTime ts;
            if (!this->ui->tvp_AircraftModel->isEmpty())
            {
                CAircraftModel model(this->ui->tvp_AircraftModel->container().latestObject());
                ts = model.getUtcTimestamp();
            }
            this->triggerRead(CEntityFlags::ModelEntity, ts);
        }

        void CDbModelComponent::ps_modelsRead(CEntityFlags::Entity entity, CEntityFlags::ReadState readState, int count)
        {
            Q_UNUSED(count);
            if (entity.testFlag(CEntityFlags::ModelEntity))
            {
                if (readState == CEntityFlags::ReadFinished || readState == CEntityFlags::ReadFinishedRestricted)
                {
                    this->ui->tvp_AircraftModel->updateContainerMaybeAsync(this->getModels());
                }
            }
        }

        void CDbModelComponent::ps_reload()
        {
            if (!this->hasProvider()) { return; }
            this->triggerRead(CEntityFlags::ModelEntity, QDateTime());
        }

        void CDbModelComponent::ps_onStyleSheetChanged()
        {
            // code goes here
        }

        void CDbModelComponent::ps_stashSelectedModels()
        {
            if (!this->ui->tvp_AircraftModel->hasSelection()) { return; }
            const CAircraftModelList models(this->ui->tvp_AircraftModel->selectedObjects());
            if (!models.isEmpty())
            {
                emit requestStash(models);
            }
        }

        void CDbModelComponent::CStashMenu::customMenu(QMenu &menu) const
        {
            CDbModelComponent *modelComponent = qobject_cast<CDbModelComponent *>(this->parent());
            Q_ASSERT_X(modelComponent, Q_FUNC_INFO, "Cannot access model component");
            if (modelComponent->hasModels())
            {
                menu.addAction(CIcons::appMappings16(), "Stash", modelComponent, SLOT(ps_stashSelectedModels()), CShortcut::keyStash());
            }
            this->nestedCustomMenu(menu);
        }

        CDbModelComponent *CDbModelComponent::CStashMenu::modelComponent() const
        {
            return qobject_cast<CDbModelComponent *>(this->parent());
        }

    } // ns
} // ns
