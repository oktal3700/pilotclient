/* Copyright (C) 2016
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackmisc/simulation/aircraftmodel.h"
#include "blackmisc/simulation/aircraftmodelutils.h"

namespace BlackMisc
{
    namespace Simulation
    {
        int CAircraftModelUtilities::mergeWithDbData(CAircraftModelList &modelToBeModified, const CAircraftModelList &dbModels, bool force)
        {
            if (dbModels.isEmpty() || modelToBeModified.isEmpty()) { return 0; }
            int c = 0;
            for (CAircraftModel &simModel : modelToBeModified)
            {
                if (!force && simModel.hasValidDbKey()) { continue; } // already done
                CAircraftModel dbModel(dbModels.findFirstByModelStringOrDefault(simModel.getModelString()));
                if (!dbModel.hasValidDbKey())
                {
                    continue; // not found
                }
                dbModel.updateMissingParts(simModel, false);
                simModel = dbModel;
                c++;
            }
            return c;
        }

        bool CAircraftModelUtilities::mergeWithVPilotData(CAircraftModelList &modelToBeModified, const CAircraftModelList &vPilotModels, bool force)
        {
            if (vPilotModels.isEmpty() || modelToBeModified.isEmpty()) { return false; }
            for (CAircraftModel &simModel : modelToBeModified)
            {
                if (!force && simModel.hasValidAircraftAndAirlineDesignator()) { continue; }  // already done
                CAircraftModel vPilotModel(vPilotModels.findFirstByModelStringOrDefault(simModel.getModelString()));
                if (!vPilotModel.hasValidDbKey())
                {
                    continue; // not found
                }
                simModel.updateMissingParts(vPilotModel, false);
            }
            return true;
        }
    } // ns
} // ns
