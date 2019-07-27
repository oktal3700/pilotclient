/* Copyright (C) 2018
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution. No part of swift project, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 */

//! \cond PRIVATE_TESTS
//! \file
//! \ingroup testblackmisc

#include "blackmisc/simulation/xplane/qtfreeutils.h"
#include "blackmisc/simulation/settings/xswiftbussettings.h"
#include "blackmisc/directoryutils.h"
#include "test.h"

#include <QTest>

using namespace BlackMisc;
using namespace BlackMisc::Simulation::XPlane::QtFreeUtils;
using namespace BlackMisc::Simulation::Settings;

namespace BlackMiscTest
{
    //! X-Plane utils test
    class CTestXPlane : public QObject
    {
        Q_OBJECT

    private slots:
        void getFileNameTest();
        void getDirNameTest();
        void getBaseNameTest();
        void splitTest();
        void acfPropertiesTest();
        void xSwiftBusSettingsTest();
    };

    void CTestXPlane::getFileNameTest()
    {
        QCOMPARE(QString::fromStdString(getFileName("c:\\foo\\bar\\test.txt")), QString("test.txt"));
        QCOMPARE(QString::fromStdString(getFileName("c:/foo/bar/test.txt")), QString("test.txt"));
        QCOMPARE(QString::fromStdString(getFileName("/foo/bar/test.txt")), QString("test.txt"));
        QCOMPARE(QString::fromStdString(getFileName("foo/test.txt")), QString("test.txt"));
        QCOMPARE(QString::fromStdString(getFileName("test.txt")), QString("test.txt"));
        QCOMPARE(QString::fromStdString(getFileName("foo/test.txt.txt")), QString("test.txt.txt"));
        QCOMPARE(QString::fromStdString(getFileName("foo/bar")), QString("bar"));
    }

    void CTestXPlane::getDirNameTest()
    {
        QCOMPARE(QString::fromStdString(getDirName("c:\\foo\\bar\\test.txt")), QString("bar"));
        QCOMPARE(QString::fromStdString(getDirName("c:/foo/bar/test.txt")), QString("bar"));
        QCOMPARE(QString::fromStdString(getDirName("/foo/bar/test.txt")), QString("bar"));
        QCOMPARE(QString::fromStdString(getDirName("foo/test.txt")), QString("foo"));
        QCOMPARE(QString::fromStdString(getDirName("test.txt")), QString());
        QCOMPARE(QString::fromStdString(getDirName("foo/test.txt.txt")), QString("foo"));
    }

    void CTestXPlane::getBaseNameTest()
    {
        QCOMPARE(QString::fromStdString(getBaseName("c:\\foo\\bar\\test.txt")), QString("test"));
        QCOMPARE(QString::fromStdString(getBaseName("c:/foo/bar/test.txt")), QString("test"));
        QCOMPARE(QString::fromStdString(getBaseName("/foo/bar/test.txt")), QString("test"));
        QCOMPARE(QString::fromStdString(getBaseName("foo/test.txt")), QString("test"));
        QCOMPARE(QString::fromStdString(getBaseName("test.txt")), QString("test"));
        QCOMPARE(QString::fromStdString(getBaseName("foo/test.txt.txt")), QString("test"));
    }

    void CTestXPlane::splitTest()
    {
        const std::string sentence("This is a test!");
        std::vector<std::string> result = split(sentence);
        QVERIFY(result.size() == 4);
        QCOMPARE(QString::fromStdString(result[0]), QString("This"));
        QCOMPARE(QString::fromStdString(result[3]), QString("test!"));

        std::vector<std::string> result2 = split(sentence, 1);
        QVERIFY(result2.size() == 2);
        QCOMPARE(QString::fromStdString(result2[0]), QString("This"));
        QCOMPARE(QString::fromStdString(result2[1]), QString("is a test!"));

        std::vector<std::string> result3 = split(sentence, 0, ".");
        QVERIFY(result3.size() == 1);
        QCOMPARE(QString::fromStdString(result3[0]), QString("This is a test!"));

        std::vector<std::string> result4 = split("");
        QVERIFY(result4.size() == 1);
        QCOMPARE(QString::fromStdString(result4[0]), QString(""));
    }

    void CTestXPlane::acfPropertiesTest()
    {
        QString testAcfFile = CDirectoryUtils::testFilesDirectory() + "/testaircraft.acf";
        AcfProperties acfProperties = extractAcfProperties(testAcfFile.toStdString());
        QCOMPARE(QString::fromStdString(acfProperties.aircraftIcaoCode), QString("BE58"));
        QCOMPARE(QString::fromStdString(acfProperties.modelDescription), QString("[ACF] Beechcraft Baron B58"));
        QCOMPARE(QString::fromStdString(acfProperties.author), QString("swift project"));
        QCOMPARE(QString::fromStdString(acfProperties.modelName), QString("Beechcraft Baron 58"));
        QCOMPARE(QString::fromStdString(acfProperties.modelString), QString("swift project Beechcraft Baron 58"));
    }

    void CTestXPlane::xSwiftBusSettingsTest()
    {
        CXSwiftBusSettings s = CXSwiftBusSettings::defaultValue();
        s.setMaxPlanes(33);
        s.setMaxDrawDistanceNM(11.11);
        s.setDrawingLabels(false);
        s.setFollowAircraftDistanceM(123);
        s.setCurrentUtcTime();
        QString json = s.toXSwiftBusJsonStringQt();

        qDebug() << json;
        qDebug() << s.toQString();

        CXSwiftBusSettings s2(json);
        QCOMPARE(s.getMaxDrawDistanceNM(), s2.getMaxDrawDistanceNM());
        QCOMPARE(s.getMaxPlanes(), s2.getMaxPlanes());
        QCOMPARE(s.isDrawingLabels(), s2.isDrawingLabels());
        QCOMPARE(s.getDBusServerAddressQt(), s2.getDBusServerAddressQt());
        QCOMPARE(s.getFollowAircraftDistanceM(), s2.getFollowAircraftDistanceM());
        QCOMPARE(s.getMSecsSinceEpoch(), s2.getMSecsSinceEpoch());

        s.setDBusServerAddressQt(CDBusServer::sessionBusAddress());
        json = s.toXSwiftBusJsonStringQt();
        s2 = CXSwiftBusSettings(json);
        QCOMPARE(CDBusServer::sessionBusAddress(), s2.getDBusServerAddressQt());

        // standard value object test
        json = s.toJsonString(); // standard swift/Qt
        qDebug() << json;

        s2.fromJson(json);
        QCOMPARE(s.getMaxDrawDistanceNM(), s2.getMaxDrawDistanceNM());
        QCOMPARE(s.getMaxPlanes(), s2.getMaxPlanes());
        QCOMPARE(s.isDrawingLabels(), s2.isDrawingLabels());
        QCOMPARE(s.getDBusServerAddressQt(), s2.getDBusServerAddressQt());
    }
}

//! main
BLACKTEST_MAIN(BlackMiscTest::CTestXPlane);

#include "testxplane.moc"

//! \endcond
