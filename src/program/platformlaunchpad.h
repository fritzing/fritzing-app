#include "platform.h"

#ifndef PLATFORMLAUNCHPAD_H
#define PLATFORMLAUNCHPAD_H

class PlatformLaunchpad : public Platform
{
public:
    PlatformLaunchpad();

    void upload(QString port, QString board, QString fileLocation);
};

#endif // PLATFORMLAUNCHPAD_H
