#include "platformpicaxe.h"

PlatformPicaxe::PlatformPicaxe() : Platform(QString("PICAXE"))
{
    setReferenceUrl(QUrl("http://www.picaxe.com/BASIC-Commands"));
    setCanProgram(true);
    setDownloadUrl(QUrl("http://www.picaxe.com/Software/Drivers/PICAXE-Compilers/"));
    setMinVersion("2.0");
    setExtensions(QStringList() << ".bas" << ".BAS");
}

void PlatformPicaxe::upload(QString port, QString board, QString fileLocation)
{
    // see http://www.picaxe.com/docs/beta_compiler.pdf

}
