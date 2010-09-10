// This file is part of Cosmographia.
//
// Copyright (C) 2010 Chris Laurel <claurel@gmail.com>
//
// Cosmographia is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// Cosmographia is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with Cosmographia. If not, see <http://www.gnu.org/licenses/>.

#include "JPLEphemeris.h"
#include <vesta/Units.h>
#include <QDataStream>
#include <QFile>
#include <QDebug>

using namespace vesta;
using namespace std;


JPLEphemeris::JPLEphemeris()
{
}


JPLEphemeris::~JPLEphemeris()
{
}


ChebyshevPolyTrajectory*
JPLEphemeris::trajectory(JplObjectId id) const
{
    if ((int) id >= (int) ObjectCount)
    {
        return NULL;
    }
    else
    {
        return m_trajectories[(int) id].ptr();
    }
}


void
JPLEphemeris::setTrajectory(JplObjectId id, ChebyshevPolyTrajectory* trajectory)
{
    if (int(id) < int(ObjectCount))
    {
        m_trajectories[int(id)] = trajectory;
    }
}


struct JplEphCoeffInfo
{
    quint32 offset;
    quint32 coeffCount;
    quint32 granuleCount;
};


JPLEphemeris*
JPLEphemeris::load(const string& filename)
{
    const int JplEph_LabelSize                      =   84;
    const unsigned int JplEph_ConstantCount         =  400;
    const unsigned int JplEph_ConstantNameLength    =    6;
    const unsigned int JplEph_ObjectCount           =   12; // Sun, Moon, planets (incl. Pluto), Earth-Moon bary.

    const unsigned int DE406_RecordSize             =  728;

    QFile ephemFile(filename.c_str());
    if (!ephemFile.open(QIODevice::ReadOnly))
    {
        qDebug() << "Ephemeris file is missing!";
        return NULL;
    }

    QDataStream in(&ephemFile);
    in.setByteOrder(QDataStream::BigEndian);

    in.skipRawData(JplEph_LabelSize * 3);
    if (in.status() != QDataStream::Ok)
    {
        return NULL;
    }

    // Skip past the constant names
    in.skipRawData(JplEph_ConstantCount * JplEph_ConstantNameLength);
    if (in.status() != QDataStream::Ok)
    {
        return NULL;
    }

    double startJd = 0.0;
    double endJd = 0.0;
    double daysPerRecord = 0.0;
    in >> startJd;
    in >> endJd;
    in >> daysPerRecord;
    if (in.status() != QDataStream::Ok)
    {
        return NULL;
    }

    // Skip number of constants with values
    in.skipRawData(sizeof(quint32));

    double kmPerAu = 0.0;
    double earthMoonMassRatio = 0.0;
    in >> kmPerAu;
    in >> earthMoonMassRatio;

    JplEphCoeffInfo coeffInfo[JplEph_ObjectCount];
    for (unsigned int objectIndex = 0; objectIndex < JplEph_ObjectCount; ++objectIndex)
    {
        in >> coeffInfo[objectIndex].offset;
        in >> coeffInfo[objectIndex].coeffCount;
        in >> coeffInfo[objectIndex].granuleCount;

        if (in.status() != QDataStream::Ok)
        {
            return NULL;
        }

        // Convert to a zero-based offset
        coeffInfo[objectIndex].offset--;
    }

    qint32 ephemNumber = 0;
    in >> ephemNumber;
    if (ephemNumber != 406)
    {
        return NULL;
    }

    // Skip libration information (offset, coeff. count, granule count)
    in.skipRawData(sizeof(quint32) * 3);
    if (in.status() != QDataStream::Ok)
    {
        return NULL;
    }

    // Skip the rest of the record
    in.skipRawData(DE406_RecordSize * 8 - 2856);
    // Skip the constants
    in.skipRawData(DE406_RecordSize * 8);
    if (in.status() != QDataStream::Ok)
    {
        return NULL;
    }

    unsigned int recordCount = (unsigned int) ((endJd - startJd) / daysPerRecord);

    vector<double> objectCoeffs[JplEph_ObjectCount];

    for (unsigned int i = 0; i < recordCount; ++i)
    {
        double startTime = 0.0;
        double endTime = 0.0;
        in >> startTime >> endTime;

        for (unsigned int objectIndex = 0; objectIndex < JplEph_ObjectCount; ++objectIndex)
        {
            unsigned int coeffCount = coeffInfo[objectIndex].coeffCount * coeffInfo[objectIndex].granuleCount;
            if (coeffCount != 0)
            {
                for (unsigned int j = 0; j < coeffCount; ++j)
                {
                    double x = 0.0;
                    double y = 0.0;
                    double z = 0.0;
                    in >> x >> y >> z;
                    objectCoeffs[objectIndex].push_back(x);
                    objectCoeffs[objectIndex].push_back(y);
                    objectCoeffs[objectIndex].push_back(z);
                }
            }
        }

        // Make sure that we read this record successfully
        if (in.status() != QDataStream::Ok)
        {
            return NULL;
        }
    }

    JPLEphemeris* eph = new JPLEphemeris;
    double startSec = daysToSeconds(startJd - vesta::J2000);
    double secsPerRecord = daysToSeconds(daysPerRecord);

    for (unsigned int objectIndex = 0; objectIndex < JplEph_ObjectCount - 1; ++objectIndex)
    {
        ChebyshevPolyTrajectory* trajectory =
                new ChebyshevPolyTrajectory(objectCoeffs[objectIndex].data(),
                                            coeffInfo[objectIndex].coeffCount - 1,
                                            coeffInfo[objectIndex].granuleCount * recordCount,
                                            startSec,
                                            secsPerRecord / coeffInfo[objectIndex].granuleCount);
        eph->setTrajectory(JplObjectId(objectIndex), trajectory);
    }

    return eph;
}
