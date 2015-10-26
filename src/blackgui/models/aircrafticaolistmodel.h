/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKGUI_AIRCRAFTICAOLISTMODEL_H
#define BLACKGUI_AIRCRAFTICAOLISTMODEL_H

#include "blackgui/blackguiexport.h"
#include <QAbstractItemModel>
#include "blackmisc/aviation/aircrafticaocodelist.h"
#include "blackgui/models/listmodelbase.h"

namespace BlackGui
{
    namespace Models
    {
        //! Airport list model
        class BLACKGUI_EXPORT CAircraftIcaoCodeListModel :
            public CListModelBase<BlackMisc::Aviation::CAircraftIcaoCode, BlackMisc::Aviation::CAircraftIcaoCodeList, true>
        {
        public:
            //! Constructor
            explicit CAircraftIcaoCodeListModel(QObject *parent = nullptr);

            //! Destructor
            virtual ~CAircraftIcaoCodeListModel() {}
        };
    }
}
#endif // guard
