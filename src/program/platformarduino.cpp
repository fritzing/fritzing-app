#include "platformarduino.h"

PlatformArduino::PlatformArduino() : Platform(QString("Arduino"))
{
    setReferenceUrl(QUrl(QString("http://arduino.cc/en/Reference/")));
    setDownloadUrl(QUrl("http://arduino.cc/en/Main/Software"));
    setMinVersion("1.5.2");
    setCanProgram(true);
    setExtensions(QStringList() << ".ino" << ".pde");

    QMap<QString, QString> boards;
    boards.insert("Arduino UNO", "arduino:avr:uno");
    boards.insert("Arduino Mega or Mega 2560", "arduino:avr:mega");
    boards.insert("Arduino Yún", "arduino:avr:yun");
    setBoards(boards);
}

void PlatformArduino::upload(QString port, QString board, QString fileLocation)
{

}
