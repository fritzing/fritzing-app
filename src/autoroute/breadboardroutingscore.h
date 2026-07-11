/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2019 Fritzing

Fritzing is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

********************************************************************/

#ifndef BREADBOARDROUTINGSCORE_H
#define BREADBOARDROUTINGSCORE_H

#include <QString>

// Breadboard quality is deliberately lexicographic. A lower-priority
// improvement must never compensate for a failed net or an extra jumper.
struct BreadboardRoutingScore
{
	int failedNets = 0;
	int jumperCount = 0;
	double jumperLength = 0.0;
	double componentLeadLength = 0.0;
	double congestion = 0.0;

	bool operator<(const BreadboardRoutingScore &other) const;
	bool operator==(const BreadboardRoutingScore &other) const;
	QString toString() const;
};

#endif
