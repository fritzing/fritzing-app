#include "platform.h"

#include <QString>

#ifndef PLATFORMARDUINO_H
#define PLATFORMARDUINO_H

class PlatformArduino : public Platform
{
public:
    PlatformArduino();

    void upload(QWidget *source, const QString &port, const QString &board, const QString &fileLocation);
};

#endif // PLATFORMARDUINO_H
