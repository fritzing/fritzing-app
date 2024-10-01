#include "platform.h"

#include <QString>

#ifndef PLATFORMARDUINO_H
#define PLATFORMARDUINO_H

class PlatformArduino : public Platform
{
	Q_OBJECT
public:
	PlatformArduino();

	void upload(QWidget *source, const QString &port, const QString &board, const QString &fileLocation);
	bool usingArduinoCLI();
};

#endif // PLATFORMARDUINO_H
