#include "platform.h"

#include <QString>

#ifndef PLATFORMARDUINO_H
#define PLATFORMARDUINO_H

class PlatformArduino : public Platform
{
public:
    PlatformArduino();

    void upload(QString port, QString board, QString fileLocation);
};

#endif // PLATFORMARDUINO_H
