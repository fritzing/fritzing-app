#ifndef BREADBOARDCOORDINATE_H
#define BREADBOARDCOORDINATE_H

#include <QString>

struct BreadboardCoordinate
{
	enum class Kind {
		Invalid,
		TerminalStrip,
		PowerRail
	};

	Kind kind = Kind::Invalid;

	int board = 0;

	QChar row;
	int column = 0;

	QString rail;
	QChar polarity;

	QString connectorId;
	QString error;
};

class BreadboardCoordinateParser
{
public:
	static BreadboardCoordinate parse(const QString &text);
	static bool isValidRailPosition(int position);
};

#endif
