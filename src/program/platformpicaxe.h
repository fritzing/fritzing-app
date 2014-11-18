#include "platform.h"

#include <QString>

#ifndef PLATFORMPICAXE_H
#define PLATFORMPICAXE_H

class PlatformPicaxe : public Platform
{
public:
    PlatformPicaxe();

    void upload(QWidget *source, const QString &port, const QString &board, const QString &fileLocation);
};

#endif // PLATFORMPICAXE_H
