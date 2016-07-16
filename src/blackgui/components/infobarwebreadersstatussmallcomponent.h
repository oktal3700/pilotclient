/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKGUI_COMPONENTS_INFOBARWEBREADERSSTATUSSMALLCOMPONENT_H
#define BLACKGUI_COMPONENTS_INFOBARWEBREADERSSTATUSSMALLCOMPONENT_H

#include "infobarwebreadersstatuscomponent.h"
#include <QScopedPointer>

namespace Ui { class CInfoBarWebReadersStatusSmallComponent; }
namespace BlackGui
{
    namespace Components
    {
        /*!
         * Smaller version of CInfoBarWebReadersStatusComponent
         */
        class BLACKGUI_EXPORT CInfoBarWebReadersStatusSmallComponent : public CInfoBarWebReadersStatusBase
        {
            Q_OBJECT

        public:
            //! Constructor
            explicit CInfoBarWebReadersStatusSmallComponent(QWidget *parent = nullptr);

            //! Destructor
            ~CInfoBarWebReadersStatusSmallComponent();

        private:
            QScopedPointer<Ui::CInfoBarWebReadersStatusSmallComponent> ui;
        };
    } // ns
} // ns
#endif // guard
