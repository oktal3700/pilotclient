/* Copyright (C) 2018
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution. No part of swift project, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 */

//! \file

#ifndef BLACKMISC_SIMULATION_XPLANE_XPLANQTFREEUTILS_H
#define BLACKMISC_SIMULATION_XPLANE_XPLANQTFREEUTILS_H

#include <string>
#include <fstream>
#include <vector>
#include <cctype>
#include <algorithm>

// Strict header only X-Plane model parser utils shared between BlackMisc and XSwiftBus.
// Header only is necessary to no require XSwiftBus to link against BlackMisc.

namespace BlackMisc
{
    namespace Simulation
    {
        namespace XPlane
        {
            namespace QtFreeUtils
            {
                //! Get filename (including all extensions) from a filePath
                inline std::string getFileName(const std::string &filePath)
                {
                    const std::string seperator = "/\\";
                    const std::size_t sepPos = filePath.find_last_of(seperator);
                    if (sepPos != std::string::npos)
                    {
                        return filePath.substr(sepPos + 1, filePath.size() - 1);
                    }
                    else
                    {
                        return filePath;
                    }
                }

                //! Get the name of the parent directory
                inline std::string getDirName(const std::string &filePath)
                {
                    const std::string seperator = "/\\";
                    const std::size_t sepPos = filePath.find_last_of(seperator);
                    if (sepPos != std::string::npos)
                    {
                        std::string dirPath = filePath.substr(0, sepPos);
                        return getFileName(dirPath);
                    }
                    else
                    {
                        return {};
                    }
                }

                //! Get the base name of the file
                inline std::string getBaseName(const std::string &filePath)
                {
                    const std::string seperator = ".";
                    const std::string fileName = getFileName(filePath);
                    std::size_t sepPos = fileName.find(seperator);
                    if (sepPos != std::string::npos)
                    {
                        return fileName.substr(0, sepPos);
                    }
                    else
                    {
                        return fileName;
                    }
                }

                //! Split string by delimiter and maxSplitCount times
                inline std::vector<std::string> split(const std::string &str, size_t maxSplitCount = 0, const std::string &delimiter = " ")
                {
                    std::string s(str);
                    size_t pos = 0;
                    std::vector<std::string> tokens;
                    while ((pos = s.find(delimiter)) != std::string::npos)
                    {
                        tokens.push_back(s.substr(0, pos));
                        s.erase(0, pos + delimiter.length());
                        if (maxSplitCount > 0 && tokens.size() == maxSplitCount) { break; }
                    }
                    tokens.push_back(s);
                    return tokens;
                }

                //! ACF properties
                struct AcfProperties
                {
                    std::string aircraftIcaoCode;   //!< Aircraft ICAO code
                    std::string modelDescription;   //!< Model description
                    std::string modelName;          //!< Model name
                    std::string author;             //!< Model author
                    std::string modelString;        //!< Generated model string
                };

                //! Get the model string for a flyable aircraft
                inline std::string stringForFlyableModel(const AcfProperties &acfProperties, const std::string &acfFile)
                {
                    if (! acfProperties.author.empty())
                    {
                        if (! acfProperties.modelName.empty())
                        {
                            if (acfProperties.modelName.find(acfProperties.author) != std::string::npos) { return acfProperties.modelName; }
                            else { return acfProperties.author + ' ' + acfProperties.modelName; }
                        }
                        else if (! acfProperties.aircraftIcaoCode.empty())
                        {
                            return acfProperties.author + ' ' + acfProperties.aircraftIcaoCode;
                        }
                    }
                    return getDirName(acfFile) + ' ' + getBaseName(acfFile);
                }

                //! String to lower case
                inline std::string toLower(std::string s)
                {
                    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c)
                    {
                        return std::tolower(c);
                    });
                    return s;
                }

                //! Extract ACF properties from an aircraft file
                inline AcfProperties extractAcfProperties(const std::string &filePath)
                {
                    std::ifstream fs(filePath, std::ios::in);
                    if (!fs.is_open()) { return {}; }

                    std::string i;
                    std::string version;
                    std::string acf;
                    std::getline(fs, i);
                    i = toLower(i);
                    std::getline(fs, version);
                    version = toLower(version);
                    std::getline(fs, acf);
                    acf = toLower(acf);

                    AcfProperties acfProperties;

                    if (i == "i" && version.find("version") != std::string::npos && acf == "acf")
                    {
                        std::string line;
                        while (std::getline(fs, line))
                        {
                            auto tokens = split(line, 2);
                            if (tokens.size() < 3 || tokens.at(0) != "P") { continue; }

                            if (tokens.at(1) == "acf/_ICAO")
                            {
                                acfProperties.aircraftIcaoCode = tokens.at(2);
                            }
                            else if (tokens.at(1) == "acf/_descrip")
                            {
                                acfProperties.modelDescription = "[ACF] " + tokens.at(2);
                            }
                            else if (tokens.at(1) == "acf/_name")
                            {
                                acfProperties.modelName = tokens.at(2);
                            }
                            else if (tokens.at(1) == "acf/_studio")
                            {
                                acfProperties.author = tokens.at(2);
                            }
                            else if (tokens.at(1) == "acf/_author")
                            {
                                if (!acfProperties.author.empty()) { continue; }
                                std::string author = tokens.at(2);
                                size_t pos = author.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_");
                                author = author.substr(0, pos);
                                if (author.empty()) { continue; }
                                acfProperties.author = author;
                            }
                        }
                    }

                    fs.close();
                    acfProperties.modelString = stringForFlyableModel(acfProperties, filePath);
                    return acfProperties;
                }
            }
        } // namespace
    } // namespace
} // namespace

#endif // guard

