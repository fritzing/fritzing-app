#include "s2sapplication.h"

#include "stdio.h"

#include <QStringList>
#include <QTextStream>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QtDebug>
#include <QRegExp>
#include <qmath.h>
#include <QDomNodeList>
#include <QDomDocument>
#include <QDomElement>
#include <QSvgRenderer>
#include <QFontDatabase>
#include <QFont>
#include <QFontInfo>
#include <QResource>
#include <QImage>
#include <QPainter>
#include <QBitArray>

#include "../../src/utils/textutils.h"
#include "../../src/utils/schematicrectconstants.h"
#include "../../src/utils/s2s.h"

#include <limits>


/////////////////////////////////
//
//	TODO:  
//
//      pin numbers in brd2svg--something to add to params files?
//	
//      copy old schematics to obsolete (with prefix)
//      readonly old style vs. read/write new style
//
//      update wire thickness in fritzing schematic view
//      update part label size (and font?) in fritzing
//
//      controller_wiringmini a0 pin is fucked
//
//      update internal schematics generators
//      update brd2svg schematics generator
//

 
///////////////////////////////////////////////////////

S2SApplication::S2SApplication(int argc, char *argv[]) : QCoreApplication(argc, argv)
{
    m_fzpzStyle = false;
}

void S2SApplication::start() {
    if (!initArguments()) {
        usage();
        return;
    }

	int ix = QFontDatabase::addApplicationFont(":/resources/fonts/DroidSans.ttf");
    if (ix < 0) return;

	ix = QFontDatabase::addApplicationFont(":/resources/fonts/DroidSans-Bold.ttf");
    if (ix < 0) return;

	ix = QFontDatabase::addApplicationFont(":/resources/fonts/DroidSansMono.ttf");
    if (ix < 0) return;

	ix = QFontDatabase::addApplicationFont(":/resources/fonts/OCRA.ttf");
    if (ix < 0) return;

    m_fzpDir.setPath(m_fzpPath);
    m_oldSvgDir.setPath(m_oldSvgPath);
    m_newSvgDir.setPath(m_newSvgPath);

    QFile file(m_filePath);
    if (!file.open(QFile::ReadOnly)) {
        message(QString("unable to open %1").arg(m_filePath));
        return;
    }

    S2S s2s(m_fzpzStyle);
    s2s.setSvgDirs(m_oldSvgDir, m_newSvgDir);

    QTextStream in(&file);
    while ( !in.atEnd() )
    {
        QString line = in.readLine().trimmed();
        QString schematicFileName;
        s2s.onefzp(m_fzpDir.absoluteFilePath(line), schematicFileName);   
    }
}

void S2SApplication::usage() {
    message("\nusage: s2s "
                "-ff <path to fzps> "
                "-f <path to list of fzps (each item in list must be a relative path)> "
                "-os <path to old schematic svgs (e.g. '.../pdb/core')> "
                "-ns <path to new schematic svgs> "
                "[-fzpz (translate to fzpz style filenames)] "
                "\n"
    );
}

bool S2SApplication::initArguments() {
    QStringList args = QCoreApplication::arguments();
    for (int i = 0; i < args.length(); i++) {
        if ((args[i].compare("-h", Qt::CaseInsensitive) == 0) ||
            (args[i].compare("-help", Qt::CaseInsensitive) == 0) ||
            (args[i].compare("--help", Qt::CaseInsensitive) == 0))
        {
            return false;
        }

        if (args[i].compare("-fzpz", Qt::CaseInsensitive) == 0) {
            m_fzpzStyle = true;
            continue;
        }

		if (i + 1 < args.length()) {
			if ((args[i].compare("-ff", Qt::CaseInsensitive) == 0) ||
				(args[i].compare("--ff", Qt::CaseInsensitive) == 0))
			{
				m_fzpPath = args[++i];
			}
			else if ((args[i].compare("-os", Qt::CaseInsensitive) == 0) ||
				(args[i].compare("--os", Qt::CaseInsensitive) == 0))
			{
				 m_oldSvgPath = args[++i];
			}
			else if ((args[i].compare("-ns", Qt::CaseInsensitive) == 0) ||
				(args[i].compare("--ns", Qt::CaseInsensitive) == 0))
			{
				 m_newSvgPath = args[++i];
			}
			else if ((args[i].compare("-f", Qt::CaseInsensitive) == 0) ||
				(args[i].compare("--f", Qt::CaseInsensitive) == 0))
			{
				m_filePath = args[++i];
			}
		}
    }

    if (m_fzpPath.isEmpty()) {
        message("-ff fzp path parameter missing");
        usage();
        return false;
    }

    if (m_oldSvgPath.isEmpty()) {
        message("-os old svg path parameter missing");
        usage();
        return false;
    }

    if (m_newSvgPath.isEmpty()) {
        message("-ns new svg path parameter missing");
        usage();
        return false;
    }

    if (m_filePath.isEmpty()) {
        message("-f fzp list file parameter missing");
        usage();
        return false;
    }

    QStringList paths;
    paths << m_fzpPath << m_oldSvgPath << m_newSvgPath;
    foreach (QString path, paths) {
        QDir directory(path);
        if (!directory.exists()) {
            message(QString("path '%1' not found").arg(path));
            return false;
        }
    }

    QFile file(m_filePath);
    if (!file.exists()) {
        message(QString("file path '%1' not found").arg(m_filePath));
        return false;
    }

    return true;
}

void S2SApplication::message(const QString & msg) {
   // QTextStream cout(stdout);
  //  cout << msg;
   // cout.flush();

	qDebug() << msg;
}

