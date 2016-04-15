/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKMISC_SIMULATION_FSCOMMON_AIRCRAFTCFGPARSER_H
#define BLACKMISC_SIMULATION_FSCOMMON_AIRCRAFTCFGPARSER_H

#include "blackmisc/blackmiscexport.h"
#include "blackmisc/worker.h"
#include "blackmisc/pixmap.h"
#include "blackmisc/datacache.h"
#include "blackmisc/simulation/aircraftmodelloader.h"
#include "blackmisc/simulation/fscommon/aircraftcfgentrieslist.h"
#include "blackmisc/simulation/data/modelcaches.h"

#include <QPointer>

namespace BlackMisc
{
    namespace Simulation
    {
        namespace FsCommon
        {
            //! Utility, parsing the aircraft.cfg files
            class BLACKMISC_EXPORT CAircraftCfgParser : public BlackMisc::Simulation::IAircraftModelLoader
            {
                Q_OBJECT

            public:
                //! Constructor
                CAircraftCfgParser(const BlackMisc::Simulation::CSimulatorInfo &simInfo, const QString &rootDirectory, const QStringList &exludes = {});

                //! Virtual destructor
                virtual ~CAircraftCfgParser();

                //! Get parsed aircraft cfg entries list
                const CAircraftCfgEntriesList &getAircraftCfgEntriesList() const { return m_parsedCfgEntriesList; }

                //! \name Interface functions
                //! @{
                virtual bool isLoadingFinished() const override;
                virtual bool areModelFilesUpdated() const override;
                //! @}

                //! Create an parser object for given simulator
                static std::unique_ptr<CAircraftCfgParser> createModelLoader(const BlackMisc::Simulation::CSimulatorInfo &simInfo);

            protected:
                //! \name Interface functions
                //! @{
                virtual void startLoadingFromDisk(LoadMode mode, const BlackMisc::Simulation::CAircraftModelList &dbModels) override;
                //! @}

            private slots:
                //! Cache changed
                void ps_cacheChanged();

            private:
                //! Section within file
                enum FileSection
                {
                    General,
                    Fltsim,
                    Unknown
                };

                //! Perform the parsing
                //! \threadsafe
                CAircraftCfgEntriesList performParsing(const QString &directory, const QStringList &excludeDirectories, bool *ok);

                //! Fix the content read
                static QString fixedStringContent(const QVariant &qv);

                //! Value from settings, fixed string
                static QString fixedStringContent(const QSettings &settings, const QString &key);

                //! Content after "="
                static QString getFixedIniLineContent(const QString &line);

                CAircraftCfgEntriesList      m_parsedCfgEntriesList; //!< parsed entries
                QPointer<BlackMisc::CWorker> m_parserWorker;         //!< worker will destroy itself, so weak pointer

                //! Files to be used
                static const QString &fileFilter();
            };
        } // namespace
    } // namespace
} // namespace

#endif // guard
