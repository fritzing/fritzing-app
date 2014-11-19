#include "platformpicaxe.h"

#include <QProcess>
#include <QFileInfo>
#include <QSettings>

#include "programtab.h"

PlatformPicaxe::PlatformPicaxe() : Platform(QString("PICAXE"))
{
    setReferenceUrl(QUrl("http://www.picaxe.com/BASIC-Commands"));
    setCanProgram(true);
    setIdeName("PICAXE Compilers");
    setDownloadUrl(QUrl("http://www.picaxe.com/Software/Drivers/PICAXE-Compilers/"));
    setMinVersion("2.0");
    setExtensions(QStringList() << ".bas" << ".BAS");

    QMap<QString, QString> boards;
    boards.insert("PICAXE-08", "picaxe08");
    boards.insert("PICAXE-08M", "picaxe08m");
    boards.insert("PICAXE-08M2", "picaxe08m2");
    boards.insert("PICAXE-08M2LE", "picaxe08m2le");
    boards.insert("PICAXE-14M", "picaxe14m");
    boards.insert("PICAXE-14M2", "picaxe14m2");
    boards.insert("PICAXE-18", "picaxe18");
    boards.insert("PICAXE-18A", "picaxe18a");
    boards.insert("PICAXE-18M", "picaxe18m");
    boards.insert("PICAXE-18M2", "picaxe18m2");
    boards.insert("PICAXE-18X", "picaxe18x");
    boards.insert("PICAXE-20", "picaxe20");
    boards.insert("PICAXE-20M2", "picaxe20m2");
    boards.insert("PICAXE-20X2", "picaxe20x2");
    boards.insert("PICAXE-28", "picaxe28");
    boards.insert("PICAXE-28A", "picaxe28a");
    boards.insert("PICAXE-28X", "picaxe28x");
    boards.insert("PICAXE-28X1", "picaxe28x1");
    boards.insert("PICAXE-28X2", "picaxe28x2");
    boards.insert("PICAXE-40X", "picaxe28x");
    boards.insert("PICAXE-40X1", "picaxe28x1");
    boards.insert("PICAXE-40X2", "picaxe28x2");
    setBoards(boards);

    setDefaultBoardName("PICAXE-08M");
}

void PlatformPicaxe::upload(QWidget *source, const QString &port, const QString &board, const QString &fileLocation)
{
    // see http://www.picaxe.com/docs/beta_compiler.pdf
    QProcess * process = new QProcess(this);
    process->setProcessChannelMode(QProcess::MergedChannels);
    process->setReadChannel(QProcess::StandardOutput);

    connect(process, SIGNAL(finished(int, QProcess::ExitStatus)), source, SLOT(programProcessFinished(int, QProcess::ExitStatus)));
    connect(process, SIGNAL(readyReadStandardOutput()), source, SLOT(programProcessReadyRead()));

    QFileInfo cmdFileInfo(getCommandLocation());
    QString cmd(cmdFileInfo.absoluteDir().absolutePath().append("/").append(getBoards().value(board)));

    QStringList args;
    args.append(QString("-c%1").arg(port));
    args.append(fileLocation);

    ProgramTab *tab = qobject_cast<ProgramTab *>(source);
    if (tab)
        tab->appendToConsole(tr("Running %1 %2").arg(cmd).arg(args.join(" ")));
    process->start(cmd, args);
}
