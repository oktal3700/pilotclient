/* Copyright (C) 2016
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution. No part of swift project, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 */

//! \file
//! \ingroup sampleweatherdata

#include "weatherdataprinter.h"
#include "blackmisc/logmessage.h"
#include <QTextStream>

#include "weatherdataprinter.h"
#include "blackmisc/aviation/altitude.h"
#include "blackmisc/geo/latitude.h"
#include "blackmisc/geo/longitude.h"
#include "blackmisc/pq/angle.h"
#include "blackmisc/pq/physicalquantity.h"
#include "blackmisc/pq/pressure.h"
#include "blackmisc/pq/speed.h"
#include "blackmisc/pq/temperature.h"
#include "blackmisc/range.h"
#include "blackmisc/sequence.h"
#include "blackmisc/weather/cloudlayer.h"
#include "blackmisc/weather/cloudlayerlist.h"
#include "blackmisc/weather/gridpoint.h"
#include "blackmisc/weather/temperaturelayer.h"
#include "blackmisc/weather/temperaturelayerlist.h"
#include "blackmisc/weather/windlayer.h"
#include "blackmisc/weather/windlayerlist.h"

#include <stdio.h>
#include <QTextStream>

using namespace BlackMisc;
using namespace BlackMisc::Weather;
using namespace BlackMisc::Geo;
using namespace BlackMisc::PhysicalQuantities;
using namespace BlackCore;


CWeatherDataPrinter::CWeatherDataPrinter(QObject *parent) : QObject(parent)
{ }

void CWeatherDataPrinter::fetchAndPrintWeatherData(const CCoordinateGeodetic &position)
{
    QTextStream qtout(stdout);
    qtout << "Fetching weather data. This may take a while..." << endl;

    CWeatherGrid weatherGrid { { "", position } };
    m_weatherManger.requestWeatherGrid(weatherGrid, { this, &CWeatherDataPrinter::printWeatherData });
}

void CWeatherDataPrinter::printWeatherData(const BlackMisc::Weather::CWeatherGrid &weatherGrid)
{
    QTextStream qtout(stdout);
    qtout << "... finished." << endl;
    for (const CGridPoint &gridPoint : weatherGrid)
    {
        qtout << "Latitude:" << gridPoint.getPosition().latitude().toQString() << endl;
        qtout << "Longitude:" << gridPoint.getPosition().longitude().toQString() << endl;
        qtout << "    MSL Pressure: " << gridPoint.getPressureAtMsl().toQString() << endl;

        CTemperatureLayerList temperatureLayers = gridPoint.getTemperatureLayers();
        temperatureLayers.sort([](const CTemperatureLayer & a, const CTemperatureLayer & b) { return a.getLevel() < b.getLevel(); });
        qtout << "    Temperature Layers: " << endl;
        for (const auto &temperatureLayer : as_const(temperatureLayers))
        {
            qtout << "        Level: " << temperatureLayer.getLevel().toQString() << endl;
            qtout << "            Temperature: " << temperatureLayer.getTemperature().toQString() << endl;
            qtout << "            Relative Humidity: " << temperatureLayer.getRelativeHumidity() << " %" << endl;
        }
        qtout << endl;

        CWindLayerList windLayers = gridPoint.getWindLayers();
        windLayers.sort([](const CWindLayer & a, const CWindLayer & b) { return a.getLevel() < b.getLevel(); });
        qtout << "    Wind Layers: " << endl;
        for (const auto &windLayer : as_const(windLayers))
        {
            qtout << "        Level: " << windLayer.getLevel().toQString() << endl;
            qtout << "            Wind: " << windLayer.getDirection().toQString() << " at " << windLayer.getSpeed().toQString() << endl;
        }
        qtout << endl;

        qtout << "    Cloud Layers: " << endl;
        CCloudLayerList cloudLayers = gridPoint.getCloudLayers();
        cloudLayers.sort([](const CCloudLayer & a, const CCloudLayer & b) { return a.getBase() < b.getBase(); });
        for (int i = 0; i < cloudLayers.size(); i++)
        {
            const CCloudLayer &cloudLayer = cloudLayers[i];
            qtout << "        Top: " << cloudLayer.getTop().toQString() << endl;
            qtout << "            Coverage: " << cloudLayer.getCoveragePercent() << " %" << endl;
            qtout << "            Precipitation type: " << cloudLayer.getPrecipitation() << endl;
            qtout << "            Precipitation rate: " << cloudLayer.getPrecipitationRate() << endl;
            qtout << "        Base: " << cloudLayer.getBase().toQString() << endl;
        }
        qtout << endl << endl;
    }
}
