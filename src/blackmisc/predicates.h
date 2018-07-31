/* Copyright (C) 2013
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKMISC_PREDICATES_H
#define BLACKMISC_PREDICATES_H

#include "integersequence.h"
#include "algorithm.h"

#include <QObject>
#include <functional>
#include <algorithm>

namespace BlackMisc
{
    class CPropertyIndexVariantMap;

    namespace Predicates
    {
        namespace Private
        {
            //! \private
            struct Matches
            {
                const CPropertyIndexVariantMap &m_map;
                Matches(const CPropertyIndexVariantMap &map) : m_map(map) {}
                template <class T> bool operator()(const T &value) const;
            };
        }

        /*!
         * Predicate which tests whether some member functions return some values.
         * \param vs Pairs of { pointer to member function of T, value to compare it against }.
         * \return A unary functor whose operator() which will perform the actual test.
         */
        template <class... Ts>
        auto MemberEqual(Ts... vs)
        {
            return [vs...](const auto &object)
            {
                bool equal = true;
                tupleForEachPair(std::make_tuple(vs...), [ & ](auto member, const auto &value) { equal = equal && (object.*member)() == value; });
                return equal;
            };
        }

        /*!
         * Predicate which compares the return values of some member functions of two objects.
         * \param vs Pointers to member functions of T.
         * \return A binary functor whose operator() which will perform the actual test.
         */
        template <class... Ts>
        auto MemberLess(Ts... vs)
        {
            return [vs...](const auto &a, const auto &b)
            {
                bool less = true;
                bool greater = false;
                tupleForEach(std::make_tuple(vs...), [ & ](auto member)
                {
                    less = less && ! greater && (a.*member)() < (b.*member)();
                    greater = (b.*member)() < (a.*member)();
                });
                Q_UNUSED(greater); // CPP style check
                return less;
            };
        }

        /*!
         * Returns a function object that returns the value returned by one of it's argument member functions.
         */
        template <class T, class R>
        auto MemberTransform(R(T::*memberFunc)() const)
        {
            return [memberFunc](const T &object) { return (object.*memberFunc)(); };
        }

        /*!
         * Returns a predicate that returns true if the isValid() method of the value returned from one of its member functions returns true.
         */
        template <class T, class R>
        auto MemberValid(R(T::*memberFunc)() const)
        {
            return [memberFunc](const T &object) { return (object.*memberFunc)().isValid(); };
        }

        /*!
         * Returns a predicate that returns true if the value returned by its argument's member function can be found in a captured container.
         * \warning The container is captured by reference, so be careful that it remains valid for the lifetime of the predicate.
         */
        template <class T, class R, class C>
        auto MemberIsAnyOf(R(T::*memberFunc)() const, const C &container)
        {
            return [memberFunc, &container](const T &object) { return container.contains((object.*memberFunc)()); };
        }

        /*!
         * Returns a predicate that returns true if its argument compares equal with another, captured value.
         */
        template <class T>
        auto Equals(T &&value)
        {
            return [value = std::forward<T>(value)](const T &object) { return object == value; };
        }

        /*!
         * Returns a predicate that returns true if its arguments compare equal to each other, considering only the captured members.
         */
        template <class... Ts>
        auto EqualsByMembers(Ts... vs)
        {
            return [vs...](const auto &a, const auto &b)
            {
                bool equal = true;
                tupleForEach(std::make_tuple(vs...), [ & ](auto member) { equal = equal && (a.*member)() == (b.*member)(); });
                return equal;
            };
        }

        /*!
         * Returns a predicate that returns true if its argument matches a captured CPropertyIndexVariantMap.
         */
        inline auto Matches(const CPropertyIndexVariantMap &map) -> Private::Matches
        {
            return { map };
        }

    } //namespace Predicates
} //namespace BlackMisc

#endif // guard
