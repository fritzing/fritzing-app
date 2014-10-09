#include "platformarduino.h"

PlatformArduino::PlatformArduino() : Platform(QString("Arduino"))
{
    setReferenceUrl(QUrl(QString("http://arduino.cc/en/Reference/")));
    setDownloadUrl(QUrl("http://arduino.cc/en/Main/Software"));
    setMinVersion("1.5.2");
    setCanProgram(true);
    setExtensions(QStringList() << ".ino" << ".pde");
}

void PlatformArduino::upload(QString port, QString board, QString fileLocation)
{

}
