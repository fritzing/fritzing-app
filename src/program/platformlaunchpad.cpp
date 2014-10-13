#include "platformlaunchpad.h"

PlatformLaunchpad::PlatformLaunchpad() : Platform(QString("Launchpad"))
{
    setReferenceUrl(QUrl("http://energia.nu/reference/"));
    setDownloadUrl(QUrl("http://www.ti.com/tool/msp430-flasher"));
    // http://energia.nu/download/ doesn't seem to have command line support
    setMinVersion("1.0");
    setCanProgram(true);
    setExtensions(QStringList() << ".txt");
}

void PlatformLaunchpad::upload(const QString &port, const QString &board, const QString &fileLocation, QTextEdit *console)
{
    // see http://www.ti.com/tool/msp430-flasher
    // http://processors.wiki.ti.com/index.php/MSP430_Flasher_-_Command_Line_Programmer
}
