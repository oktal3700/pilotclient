/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKMISC_STRINGUTILS_H
#define BLACKMISC_STRINGUTILS_H

#include "blackmisc/blackmiscexport.h"
#include "blackmisc/range.h"

#include <QByteArray>
#include <QDataStream>
#include <QDateTime>
#include <QDebug>
#include <QList>
#include <QMapIterator>
#include <QString>
#include <QStringRef>
#include <QTextStream>
#include <QtGlobal>
#include <iosfwd>
#include <string>
#include <algorithm>

template <class T1, class T2> class QMap;

//! Free functions in BlackMisc
namespace BlackMisc
{
    //! Return a string with characters removed that match the given predicate.
    template <class F> QString removeChars(const QString &s, F predicate)
    {
        QString result;
        std::copy_if(s.begin(), s.end(), std::back_inserter(result), [ = ](auto c) { return !predicate(c); });
        return result;
    }

    //! True if any character in the string matches the given predicate.
    template <class F> bool containsChar(const QString &s, F predicate)
    {
        return std::any_of(s.begin(), s.end(), predicate);
    }

    //! Index of first character in the string matching the given predicate, or -1 if not found.
    template <class F> int indexOfChar(const QString &s, F predicate)
    {
        auto it = std::find_if(s.begin(), s.end(), predicate);
        if (it == s.end()) { return -1; }
        return static_cast<int>(std::distance(s.begin(), it));
    }

    //! Split a string into multiple strings, using a predicate function to identify the split points.
    //! \warning The returned refs are only valid during the lifetime of the original string.
    template <class F> QList<QStringRef> splitStringRefs(const QString &s, F predicate)
    {
        QList<QStringRef> result;
        auto notPredicate = [ = ](auto c) { return !predicate(c); };
        auto begin = s.begin();
        while (true)
        {
            begin = std::find_if(begin, s.end(), notPredicate);
            if (begin == s.end()) { return result; }
            auto end = std::find_if(begin, s.end(), predicate);
            result.push_back(QStringRef(&s, std::distance(s.begin(), begin), std::distance(begin, end)));
            begin = end;
        }
    }

    //! Split a string into multiple lines. Blank lines are skipped.
    //! \warning The returned refs are only valid during the lifetime of the original string.
    BLACKMISC_EXPORT QList<QStringRef> splitLinesRefs(const QString &s);

    //! It would be risky to call splitStringRefs with an rvalue, so forbid it.
    template <class F> void splitStringRefs(const QString &&, F) = delete;

    //! It would be risky to call splitLinesRefs with an rvalue, so forbid it.
    void splitLinesRefs(const QString &&) = delete;

    //! Split a string into multiple strings, using a predicate function to identify the split points.
    template <class F> QStringList splitString(const QString &s, F predicate)
    {
        return makeRange(splitStringRefs(s, predicate)).transform([](QStringRef sr) { return sr.toString(); });
    }

    //! Split a string into multiple lines. Blank lines are skipped.
    BLACKMISC_EXPORT QStringList splitLines(const QString &s);

    //! A map converted to string
    template<class K, class V> QString qmapToString(const QMap<K, V> &map)
    {
        QString s;
        const QString kv("%1: %2 ");
        QMapIterator<K, V> i(map);
        while (i.hasNext())
        {
            i.next();
            s.append(
                kv.arg(i.key()).arg(i.value())
            );
        }
        return s.trimmed();
    }

    //! String with digits only
    inline bool isDigitsOnlyString(const QString &testString)
    {
        return !containsChar(testString, [](QChar c) { return !c.isDigit(); });
    }

    //! String only with digits
    inline QString digitOnlyString(const QString &string)
    {
        return removeChars(string, [](QChar c) { return !c.isDigit(); });
    }

    //! Bool to on/off
    BLACKMISC_EXPORT QString boolToOnOff(bool v, bool  i18n = false);

    //! Bool to yes / no
    BLACKMISC_EXPORT QString boolToYesNo(bool v, bool  i18n = false);

    //! Bool to true / false
    BLACKMISC_EXPORT QString boolToTrueFalse(bool v, bool  i18n = false);

    //! Convert string to bool
    BLACKMISC_EXPORT bool stringToBool(const QString &boolString);

    //! Fuzzy compare for short strings (like ICAO designators)
    //! \return int 0..100 (100 is perfect match)
    BLACKMISC_EXPORT int fuzzyShortStringComparision(const QString &str1, const QString &str2, Qt::CaseSensitivity cs = Qt::CaseSensitive);

    //! Int to hex value
    BLACKMISC_EXPORT QString intToHex(int value, int digits = 2);

    //! Int to hex value (per byte, 2 digits)
    BLACKMISC_EXPORT QString bytesToHexString(const QByteArray &bytes);

    //! Byte array from hex value string per byte, 2 digits
    BLACKMISC_EXPORT QByteArray byteArrayFromHexString(const QString &hexString);

    //! Strip a designator from a combined string
    BLACKMISC_EXPORT QString stripDesignatorFromCompleterString(const QString &candidate);

    //! Strip a designator from a combined string
    BLACKMISC_EXPORT QStringList textCodecNames(bool simpleNames, bool mibNames);

    //! Remove accents / diacritic marks from a string
    BLACKMISC_EXPORT QString removeAccents(const QString &candidate);

    //! Case insensitive string compare
    BLACKMISC_EXPORT bool caseInsensitiveStringCompare(const QString &c1, const QString &c2);

    //! Get a simplified upper case name for searching by removing all characters except A-Z
    BLACKMISC_EXPORT QString simplifyNameForSearch(const QString &name);

    //! Parse multiple date time formats
    //! \remark potentially slow, so only to be used when format is unknown
    BLACKMISC_EXPORT QDateTime parseMultipleDateTimeFormats(const QString &dateTimeString);

    namespace Mixin
    {
        /*!
         * CRTP class template from which a derived class can inherit string streaming operations.
         *
         * \tparam Derived Must implement a public method QString convertToQString(bool i18n = false) const.
         *
         * \see BLACKMISC_DECLARE_USING_MIXIN_STRING
         */
        template <class Derived>
        class String
        {
        public:
            //! Stream << overload to be used in debugging messages
            friend QDebug operator<<(QDebug debug, const Derived &obj)
            {
                debug << obj.stringForStreaming();
                return debug;
            }

            //! Operator << when there is no debug stream
            friend QNoDebug operator<<(QNoDebug nodebug, const Derived &obj)
            {
                Q_UNUSED(obj);
                return nodebug;
            }

            //! Operator << based on text stream
            friend QTextStream &operator<<(QTextStream &stream, const Derived &obj)
            {
                stream << obj.stringForStreaming();
                return stream;
            }

            //! Operator << for QDataStream
            friend QDataStream &operator<<(QDataStream &stream, const Derived &obj)
            {
                stream << obj.stringForStreaming();
                return stream;
            }

            //! Stream operator << for std::cout
            friend std::ostream &operator<<(std::ostream &ostr, const Derived &obj)
            {
                ostr << obj.stringForStreaming().toStdString();
                return ostr;
            }

            //! Cast as QString
            QString toQString(bool i18n = false) const { return derived()->convertToQString(i18n); }

            //! Cast to pretty-printed QString
            QString toFormattedQString(bool i18n = false) const { return derived()->toQString(i18n); }

            //! To std string
            std::string toStdString(bool i18n = false) const { return derived()->convertToQString(i18n).toStdString(); }

            //! String for streaming operators
            QString stringForStreaming() const { return derived()->convertToQString(); }

        private:
            const Derived *derived() const { return static_cast<const Derived *>(this); }
            Derived *derived() { return static_cast<Derived *>(this); }
        };

        /*!
         * When a derived class and a base class both inherit from Mixin::String,
         * the derived class uses this macro to disambiguate the inherited members.
         */
#       define BLACKMISC_DECLARE_USING_MIXIN_STRING(DERIVED)                \
            using ::BlackMisc::Mixin::String<DERIVED>::toQString;           \
            using ::BlackMisc::Mixin::String<DERIVED>::toFormattedQString;  \
            using ::BlackMisc::Mixin::String<DERIVED>::toStdString;         \
            using ::BlackMisc::Mixin::String<DERIVED>::stringForStreaming;
    } // ns
} // ns

#endif // guard
