#ifndef BREADBOARDCSVDIPPHYSICALRESOLVER_H
#define BREADBOARDCSVDIPPHYSICALRESOLVER_H

#include "breadboardcsvdipfootprintresolver.h"

#include <QString>

class BreadboardCsvDipPhysicalResolver
{
public:
        static bool apply(
                BreadboardCsvDipFootprint &footprint,
                int spacingMil,
                QString &error
        );
};

#endif
