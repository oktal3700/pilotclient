/* Copyright (C) 2016
 * swift project Community / Contributors
 *
 * This file is part of swift Project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKGUI_COMPONENTS_DISTRIBUTORPREFERENCESCOMPONENT_H
#define BLACKGUI_COMPONENTS_DISTRIBUTORPREFERENCESCOMPONENT_H

#include "blackmisc/settingscache.h"
#include "blackmisc/simulation/aircraftmodelsetloader.h"
#include "blackmisc/simulation/modelsettings.h"
#include "blackmisc/simulation/simulatorinfo.h"

#include <QFrame>
#include <QObject>
#include <QScopedPointer>

class QWidget;

namespace Ui { class CDistributorPreferencesComponent; }
namespace BlackGui
{
    class COverlayMessagesFrame;

    namespace Components
    {
        /*!
         * Set and order distributors (to be used for model set)
         */
        class CDistributorPreferencesComponent : public QFrame
        {
            Q_OBJECT

        public:
            //! Constructor
            explicit CDistributorPreferencesComponent(QWidget *parent = nullptr);

            //! Destructor
            virtual ~CDistributorPreferencesComponent();

        private slots:
            //! Changed preferences
            void ps_preferencesChanged();

            //! Load all distributors
            void ps_loadAll();

            //! Load all distributors for current simulator
            void ps_loadAllForSimulator();

            //! Load distributors from set
            void ps_loadDistributorsFromSet();

            //! Save the preferences
            void ps_save();

            //! Simulator has been changed
            void ps_simulatorChanged(const BlackMisc::Simulation::CSimulatorInfo &simulator);

            // Init
            void ps_deferredInit();

        private:
            QScopedPointer<Ui::CDistributorPreferencesComponent> ui;
            BlackGui::COverlayMessagesFrame *m_overlayMessageFrame = nullptr;
            BlackMisc::Simulation::CAircraftModelSetLoader m_modelSetLoader { this };
            BlackMisc::CSetting<BlackMisc::Simulation::TDistributorListPreferences> m_distributorPreferences { this, &CDistributorPreferencesComponent::ps_preferencesChanged };

            //! Update
            void updateContainerMaybeAsync(const BlackMisc::Simulation::CDistributorList &models, bool sortByOrder = true);
        };
    } // ns
} // ns

#endif // guard
