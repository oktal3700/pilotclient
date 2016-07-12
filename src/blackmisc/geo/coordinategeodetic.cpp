/* Copyright (C) 2013
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackmisc/geo/coordinategeodetic.h"
#include "blackmisc/propertyindex.h"
#include "blackmisc/variant.h"

#include <QByteArray>
#include <QtGlobal>
#include <cmath>



using namespace BlackMisc::PhysicalQuantities;
using namespace BlackMisc::Math;

namespace BlackMisc
{
    namespace Geo
    {
        QString CCoordinateGeodetic::convertToQString(bool i18n) const
        {
            QString s = "Geodetic: {%1, %2, %3}";
            return s.arg(this->latitude().valueRoundedWithUnit(6, i18n)).arg(this->longitude().valueRoundedWithUnit(6, i18n)).arg(this->m_geodeticHeight.valueRoundedWithUnit(6, i18n));
        }

        CCoordinateGeodetic CCoordinateGeodetic::fromWgs84(const QString &latitudeWgs84, const QString &longitudeWgs84, const CLength &geodeticHeight)
        {
            CLatitude lat = CLatitude::fromWgs84(latitudeWgs84);
            CLongitude lon = CLongitude::fromWgs84(longitudeWgs84);
            return CCoordinateGeodetic(lat, lon, geodeticHeight);
        }

        PhysicalQuantities::CLength calculateGreatCircleDistance(const ICoordinateGeodetic &coordinate1, const ICoordinateGeodetic &coordinate2)
        {
            static const float earthRadiusMeters = 6371000.8f;
            const QVector3D v1 = coordinate1.normalVector();
            const QVector3D v2 = coordinate2.normalVector();
            const float d = earthRadiusMeters * std::atan2(QVector3D::crossProduct(v1, v2).length(), QVector3D::dotProduct(v1, v2));
            return { d, PhysicalQuantities::CLengthUnit::m() };
        }

        PhysicalQuantities::CAngle calculateBearing(const ICoordinateGeodetic &coordinate1, const ICoordinateGeodetic &coordinate2)
        {
            static const QVector3D northPole { 0, 0, 1 };
            const QVector3D c1 = QVector3D::crossProduct(coordinate1.normalVector(), coordinate2.normalVector());
            const QVector3D c2 = QVector3D::crossProduct(coordinate1.normalVector(), northPole);
            const QVector3D cross = QVector3D::crossProduct(c1, c2);
            const float sinTheta = std::copysign(cross.length(), QVector3D::dotProduct(cross, coordinate1.normalVector()));
            const float cosTheta = QVector3D::dotProduct(c1, c2);
            const float theta = std::atan2(sinTheta, cosTheta);
            return { theta, PhysicalQuantities::CAngleUnit::rad() };
        }

        double calculateEuclideanDistance(const ICoordinateGeodetic &coordinate1, const ICoordinateGeodetic &coordinate2)
        {
            return coordinate1.normalVector().distanceToPoint(coordinate2.normalVector());
        }

        double calculateEuclideanDistanceSquared(const ICoordinateGeodetic &coordinate1, const ICoordinateGeodetic &coordinate2)
        {
            return (coordinate1.normalVector() - coordinate2.normalVector()).lengthSquared();
        }

        CLength ICoordinateGeodetic::calculateGreatCircleDistance(const ICoordinateGeodetic &otherCoordinate) const
        {
            return Geo::calculateGreatCircleDistance((*this), otherCoordinate);
        }

        CAngle ICoordinateGeodetic::calculateBearing(const ICoordinateGeodetic &otherCoordinate) const
        {
            return Geo::calculateBearing((*this), otherCoordinate);
        }

        bool ICoordinateGeodetic::canHandleIndex(const CPropertyIndex &index)
        {
            int i = index.frontCasted<int>();
            return (i >= static_cast<int>(IndexLatitude)) && (i <= static_cast<int>(IndexNormalVector));
        }

        CVariant ICoordinateGeodetic::propertyByIndex(const BlackMisc::CPropertyIndex &index) const
        {
            if (!index.isMyself())
            {
                ColumnIndex i = index.frontCasted<ColumnIndex>();
                switch (i)
                {
                case IndexLatitude:
                    return this->latitude().propertyByIndex(index.copyFrontRemoved());
                case IndexLongitude:
                    return this->longitude().propertyByIndex(index.copyFrontRemoved());
                case IndexLatitudeAsString:
                    return CVariant(this->latitudeAsString());
                case IndexLongitudeAsString:
                    return CVariant(this->longitudeAsString());
                case IndexGeodeticHeight:
                    return this->geodeticHeight().propertyByIndex(index.copyFrontRemoved());
                case IndexGeodeticHeightAsString:
                    return CVariant(this->geodeticHeightAsString());
                case IndexNormalVector:
                    return CVariant::fromValue(this->normalVector());
                default:
                    break;
                }
            }

            const QString m = QString("no property, index ").append(index.toQString());
            Q_ASSERT_X(false, Q_FUNC_INFO, m.toLocal8Bit().constData());
            return CVariant::fromValue(m);
        }

        int ICoordinateGeodetic::comparePropertyByIndex(const CPropertyIndex &index, const ICoordinateGeodetic &compareValue) const
        {
            if (!index.isMyself())
            {
                ColumnIndex i = index.frontCasted<ColumnIndex>();
                switch (i)
                {
                case IndexLatitude:
                    return this->latitude().comparePropertyByIndex(index.copyFrontRemoved(), compareValue.latitude());
                case IndexLongitude:
                    return this->longitude().comparePropertyByIndex(index.copyFrontRemoved(), compareValue.longitude());
                case IndexLatitudeAsString:
                    return this->latitudeAsString().compare(compareValue.latitudeAsString());
                case IndexLongitudeAsString:
                    return this->longitudeAsString().compare(compareValue.longitudeAsString());
                case IndexGeodeticHeight:
                    return this->geodeticHeight().comparePropertyByIndex(index.copyFrontRemoved(), compareValue.geodeticHeight());
                case IndexGeodeticHeightAsString:
                    return this->geodeticHeightAsString().compare(compareValue.geodeticHeightAsString());
                default:
                    break;
                }
            }

            const QString m = QString("no property, index ").append(index.toQString());
            Q_ASSERT_X(false, Q_FUNC_INFO, m.toLocal8Bit().constData());
            return 0;
        }

        CVariant CCoordinateGeodetic::propertyByIndex(const BlackMisc::CPropertyIndex &index) const
        {
            if (index.isMyself()) { return CVariant::from(*this); }
            if (ICoordinateGeodetic::canHandleIndex(index))
            {
                return ICoordinateGeodetic::propertyByIndex(index);
            }
            else
            {
                return CValueObject::propertyByIndex(index);
            }
        }

        void CCoordinateGeodetic::setPropertyByIndex(const CPropertyIndex &index, const CVariant &variant)
        {
            if (index.isMyself()) { (*this) = variant.to<CCoordinateGeodetic>(); return; }
            ICoordinateGeodetic::ColumnIndex i = index.frontCasted<ICoordinateGeodetic::ColumnIndex>();
            switch (i)
            {
            case IndexGeodeticHeight:
                this->m_geodeticHeight.setPropertyByIndex(index.copyFrontRemoved(), variant);
                break;
            case IndexLatitude:
                this->setLatitude(variant.value<CLatitude>());
                break;
            case IndexLongitude:
                this->setLongitude(variant.value<CLongitude>());
                break;
            case IndexLatitudeAsString:
                this->setLatitude(CLatitude::fromWgs84(variant.toQString()));
                break;
            case IndexLongitudeAsString:
                this->setLongitude(CLongitude::fromWgs84(variant.toQString()));
                break;
            case IndexGeodeticHeightAsString:
                this->m_geodeticHeight.parseFromString(variant.toQString());
                break;
            case IndexNormalVector:
                this->setNormalVector(variant.value<QVector3D>());
                break;
            default:
                CValueObject::setPropertyByIndex(index, variant);
                break;
            }
        }

        CCoordinateGeodetic::CCoordinateGeodetic(CLatitude latitude, CLongitude longitude, BlackMisc::PhysicalQuantities::CLength height) :
            m_x(latitude.cos() * longitude.cos()),
            m_y(latitude.cos() * longitude.sin()),
            m_z(latitude.sin()),
            m_geodeticHeight(height)
        {}

        CLatitude CCoordinateGeodetic::latitude() const
        {
            return { std::atan2(m_z, std::hypot(m_x, m_y)), PhysicalQuantities::CAngleUnit::rad() };
        }

        CLongitude CCoordinateGeodetic::longitude() const
        {
            return { std::atan2(m_y, m_x), PhysicalQuantities::CAngleUnit::rad() };
        }

        QVector3D CCoordinateGeodetic::normalVector() const
        {
            return
            {
                static_cast<float>(this->m_x),
                static_cast<float>(this->m_y),
                static_cast<float>(this->m_z)
            };
        }

        std::array<double, 3> CCoordinateGeodetic::normalVectorDouble() const
        {
            return { { this->m_x, this->m_y, this->m_z } };
        }

        void CCoordinateGeodetic::setLatitude(const CLatitude &latitude)
        {
            this->setLatLong(latitude, this->longitude());
        }

        void CCoordinateGeodetic::setLongitude(const CLongitude &longitude)
        {
            this->setLatLong(this->latitude(), longitude);
        }

        void CCoordinateGeodetic::setLatLong(const CLatitude &latitude, const CLongitude &longitude)
        {
            this->m_x = latitude.cos() * longitude.cos();
            this->m_y = latitude.cos() * longitude.sin();
            this->m_z = latitude.sin();
        }

        CCoordinateGeodetic &CCoordinateGeodetic::switchUnit(const CLengthUnit &unit)
        {
            this->m_geodeticHeight.switchUnit(unit);
            return *this;
        }

        CLength ICoordinateWithRelativePosition::calculcateAndUpdateRelativeDistance(const ICoordinateGeodetic &position)
        {
            this->m_relativeDistance = Geo::calculateGreatCircleDistance(*this, position);
            return this->m_relativeDistance;
        }

        CLength ICoordinateWithRelativePosition::calculcateAndUpdateRelativeDistanceAndBearing(const ICoordinateGeodetic &position)
        {
            this->m_relativeDistance = Geo::calculateGreatCircleDistance(*this, position);
            this->m_relativeBearing = Geo::calculateBearing(*this, position);
            return this->m_relativeDistance;
        }

        CVariant ICoordinateWithRelativePosition::propertyByIndex(const CPropertyIndex &index) const
        {
            if (ICoordinateGeodetic::canHandleIndex(index)) { return ICoordinateGeodetic::propertyByIndex(index); }
            if (!index.isMyself())
            {
                ColumnIndex i = index.frontCasted<ColumnIndex>();
                switch (i)
                {
                case IndexRelativeBearing:
                    return this->getRelativeBearing().propertyByIndex(index.copyFrontRemoved());
                case IndexRelativeDistance:
                    return this->getRelativeDistance().propertyByIndex(index.copyFrontRemoved());
                default:
                    break;
                }
            }
            const QString m = QString("no property, index ").append(index.toQString());
            Q_ASSERT_X(false, "ICoordinateWithRelativePosition", m.toLocal8Bit().constData());
            return CVariant::fromValue(m);
        }

        void ICoordinateWithRelativePosition::setPropertyByIndex(const CPropertyIndex &index, const CVariant &variant)
        {
            if (!index.isMyself())
            {
                ColumnIndex i = index.frontCasted<ColumnIndex>();
                switch (i)
                {
                case IndexRelativeBearing:
                    this->m_relativeBearing.setPropertyByIndex(index.copyFrontRemoved(), variant);
                    break;
                case IndexRelativeDistance:
                    this->m_relativeDistance.setPropertyByIndex(index.copyFrontRemoved(), variant);
                    break;
                default:
                    const QString m = QString("no property, index ").append(index.toQString());
                    Q_ASSERT_X(false, "ICoordinateWithRelativePosition", m.toLocal8Bit().constData());
                    break;
                }
            }
        }

        int ICoordinateWithRelativePosition::comparePropertyByIndex(const CPropertyIndex &index, const ICoordinateWithRelativePosition &compareValue) const
        {
            if (ICoordinateGeodetic::canHandleIndex(index)) { return ICoordinateGeodetic::comparePropertyByIndex(index, compareValue); }
            if (!index.isMyself())
            {
                ColumnIndex i = index.frontCasted<ColumnIndex>();
                switch (i)
                {
                case IndexRelativeBearing:
                    return this->m_relativeBearing.comparePropertyByIndex(index.copyFrontRemoved(), compareValue.getRelativeBearing());
                case IndexRelativeDistance:
                    return this->m_relativeDistance.comparePropertyByIndex(index.copyFrontRemoved(), compareValue.getRelativeDistance());
                default:
                    const QString m = QString("no property, index ").append(index.toQString());
                    Q_ASSERT_X(false, Q_FUNC_INFO, m.toLocal8Bit().constData());
                    break;
                }
            }
            return 0;
        }

        ICoordinateWithRelativePosition::ICoordinateWithRelativePosition()
        { }

        bool ICoordinateWithRelativePosition::canHandleIndex(const CPropertyIndex &index)
        {
            int i = index.frontCasted<int>();
            return (i >= static_cast<int>(IndexRelativeDistance)) && (i <= static_cast<int>(IndexRelativeBearing));
        }
    } // namespace
} // namespace
