#include "platform.h"

#include <QString>

#ifndef PLATFORMPICAXE_H
#define PLATFORMPICAXE_H

class PlatformPicaxe : public Platform
{
public:
    PlatformPicaxe();

    void upload(QString port, QString board, QString fileLocation);
};

#endif // PLATFORMPICAXE_H
