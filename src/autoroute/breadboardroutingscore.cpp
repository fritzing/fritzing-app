/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2019 Fritzing

Fritzing is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

********************************************************************/

#include "breadboardroutingscore.h"

#include <QtMath>

namespace
{
	constexpr double ScoreEpsilon = 0.0001;

	int compareDouble(double first, double second)
	{
		if (qAbs(first - second) <= ScoreEpsilon) return 0;
		return first < second ? -1 : 1;
	}
}

bool BreadboardRoutingScore::operator<(const BreadboardRoutingScore &other) const
{
	if (failedNets != other.failedNets) return failedNets < other.failedNets;
	if (jumperCount != other.jumperCount) return jumperCount < other.jumperCount;
	if (const int comparison = compareDouble(jumperLength, other.jumperLength); comparison != 0) return comparison < 0;
	if (const int comparison = compareDouble(componentLeadLength, other.componentLeadLength); comparison != 0) return comparison < 0;
	return compareDouble(congestion, other.congestion) < 0;
}

bool BreadboardRoutingScore::operator==(const BreadboardRoutingScore &other) const
{
	return failedNets == other.failedNets
	    && jumperCount == other.jumperCount
	    && compareDouble(jumperLength, other.jumperLength) == 0
	    && compareDouble(componentLeadLength, other.componentLeadLength) == 0
	    && compareDouble(congestion, other.congestion) == 0;
}

QString BreadboardRoutingScore::toString() const
{
	return QString("failedNets=%1 jumperCount=%2 jumperLength=%3 componentLeadLength=%4 congestion=%5")
	        .arg(failedNets)
	        .arg(jumperCount)
	        .arg(jumperLength, 0, 'f', 3)
	        .arg(componentLeadLength, 0, 'f', 3)
	        .arg(congestion, 0, 'f', 3);
}
