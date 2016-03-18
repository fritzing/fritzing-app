#include "platformarduino.h"
#include "programtab.h"

#include <QDir>
#include <QFileInfo>
#include <QProcess>

PlatformArduino::PlatformArduino() : Platform(QString("Arduino"))
{
    setReferenceUrl(QUrl(QString("http://arduino.cc/en/Reference/")));
    setIdeName("Arduino IDE");
    setDownloadUrl(QUrl("http://arduino.cc/en/Main/Software"));
    setMinVersion("1.5.2");
    setCanProgram(true);
    setExtensions(QStringList() << ".ino" << ".pde");

    QMap<QString, QString> boards;
    // https://github.com/arduino/Arduino/blob/master/hardware/arduino/avr/boards.txt
    boards.insert("Arduino UNO", "arduino:avr:uno");
    boards.insert("Arduino Yún", "arduino:avr:yun");
    boards.insert("Arduino Mega/2560", "arduino:avr:mega");
    boards.insert("Arduino Duemilanove/Diecemila", "arduino:avr:diecimila");
    boards.insert("Arduino Nano", "arduino:avr:nano");
    boards.insert("Arduino Mega ADK", "arduino:avr:megaADK");
    boards.insert("Arduino Leonardo", "arduino:avr:leonardo");
    boards.insert("Arduino Micro", "arduino:avr:micro");
    boards.insert("Arduino Esplora", "arduino:avr:Esplora");
    boards.insert("Arduino Mini", "arduino:avr:mini");
    boards.insert("Arduino Ethernet", "arduino:avr:ethernet");
    boards.insert("Arduino Fio", "arduino:avr:fio");
    boards.insert("Arduino BT", "arduino:avr:bt");
    boards.insert("Lilypad Arduino USB", "arduino:avr:LilyPadUSB");
    boards.insert("LilyPad Arduino ", "arduino:avr:lilypad");
    boards.insert("Arduino Pro/Pro Mini", "arduino:avr:pro");
    boards.insert("Arduino NG or older", "arduino:avr:atmegang");
    boards.insert("Arduino Robot Control", "arduino:avr:robotControl");
    boards.insert("Arduino Robot Motor", "arduino:avr:robotMotor");
    boards.insert("Arduino Gemma", "arduino:avr:gemma");
    // https://github.com/arduino/Arduino/blob/master/hardware/arduino/sam/boards.txt
    boards.insert("Arduino Due (Programming Port)", "arduino:sam:arduino_due_x_dbg");
    boards.insert("Arduino Due (Native USB Port)", "arduino:sam:arduino_due_x");
    // Note: Edison, Galileo, 101 are programmed via Arduino IDE Add-ons,
    //       not sure if the command line is supported for them
    setBoards(boards);

    setDefaultBoardName("Arduino UNO");
}

void PlatformArduino::upload(QWidget *source, const QString &port, const QString &board, const QString &fileLocation)
{
    QProcess * process = new QProcess(this);
    process->setProcessChannelMode(QProcess::MergedChannels);
    process->setReadChannel(QProcess::StandardOutput);
    connect(process, SIGNAL(finished(int, QProcess::ExitStatus)), source, SLOT(programProcessFinished(int, QProcess::ExitStatus)));
    connect(process, SIGNAL(readyReadStandardOutput()), source, SLOT(programProcessReadyRead()));

    // Make sure .ino is in its own folder with same name (as required by Arduino compiler),
    // otherwise create a subfolder and copy the file there.
    QFileInfo fileInfo(fileLocation);
    QString tmpFilePath = fileInfo.absoluteFilePath();
    QString dirName = fileInfo.dir().dirName();
    QString sketchName = fileInfo.baseName();
    if (dirName.compare(sketchName, Qt::CaseInsensitive) != 0) {
        QString tmpSketchName(sketchName.append("_TMP"));
        fileInfo.dir().mkdir(tmpSketchName);
        tmpFilePath = fileInfo.absolutePath().append("/").append(tmpSketchName).append("/")
                .append(fileInfo.baseName().append("_TMP.").append(fileInfo.suffix()));
        if (QFile::exists(tmpFilePath)) QFile::remove(tmpFilePath);
        QFile::copy(fileInfo.absoluteFilePath(), tmpFilePath);
    }

    QStringList args;
    // see https://github.com/arduino/Arduino/blob/ide-1.5.x/build/shared/manpage.adoc
    //args.append(QString("--verbose"));
    args.append(QString("--board"));
    args.append(getBoards().value(board));
    args.append(QString("--port"));
    args.append(port);
    args.append(QString("--upload"));
    args.append(QDir::toNativeSeparators(tmpFilePath));

    ProgramTab *tab = qobject_cast<ProgramTab *>(source);
    if (tab)
        tab->appendToConsole(tr("Running %1 %2").arg(getCommandLocation()).arg(args.join(" ")));
    process->start(getCommandLocation(), args);
}
