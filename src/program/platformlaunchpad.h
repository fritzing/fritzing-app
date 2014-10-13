#include "platform.h"

#ifndef PLATFORMLAUNCHPAD_H
#define PLATFORMLAUNCHPAD_H

class PlatformLaunchpad : public Platform
{
public:
    PlatformLaunchpad();

    void upload(const QString &port, const QString &board, const QString &fileLocation, QTextEdit *console);
};

#endif // PLATFORMLAUNCHPAD_H
