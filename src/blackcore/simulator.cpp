/* Copyright (C) 2013
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "simulator.h"

namespace BlackCore
{
    ISimulator::ISimulator(QObject *parent) : QObject(parent)
    { }

    void ISimulator::emitSimulatorCombinedStatus()
    {
        emit simulatorStatusChanged(isConnected(), isSimulating(), isPaused());
    }
} // namespace
