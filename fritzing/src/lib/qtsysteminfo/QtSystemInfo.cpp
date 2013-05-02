/*
 * File:      QtSystemInfo.cpp
 * Author:    Adrian Łubik (adrian5632@gmail.com)
 * License:   GNU LGPL v2 or newer
 * Copyright: (c) 2010 Adrian Łubik
 */

#include "QtSystemInfo.h"

#include <QtGlobal>

static const char *UNKNOWN = "UNKNOWN";

class QtSystemInfoPrivate
{
public:
    QtSystemInfoPrivate() {}
    virtual ~QtSystemInfoPrivate() {}

    virtual QtSystemInfo::ArchitectureType architecture() = 0;
    virtual QtSystemInfo::SystemType systemType() = 0;
    virtual QString architectureName() = 0;
    virtual QString kernelName() = 0;
    virtual QString kernelVersion() = 0;
    virtual QString systemName() = 0;
    virtual QString systemVersion() = 0;
};

// -------------------------------------------------- //
//                      Any Unix                      //
// -------------------------------------------------- //

#ifdef Q_OS_UNIX
#include <sys/utsname.h>

class QtSystemInfoUnix: public QtSystemInfoPrivate
{
public:
    QtSystemInfoUnix():m_kernelName(UNKNOWN), m_kernelVersion(UNKNOWN), m_architectureName(UNKNOWN), m_architecture(QtSystemInfo::UnknownArchitectureType)
    {
        struct utsname buffer;

        if (uname(&buffer) == 0)
        {
            m_kernelVersion = buffer.release;
            m_kernelName = buffer.sysname;
            m_architectureName = buffer.machine;

            if (m_architectureName == "i386")
                m_architecture = QtSystemInfo::I386;
            else if (m_architectureName == "i486")
                m_architecture = QtSystemInfo::I486;
            else if (m_architectureName == "i586")
                m_architecture = QtSystemInfo::I586;
            else if (m_architectureName == "i686")
                m_architecture = QtSystemInfo::I686;
            else if (m_architectureName == "x86_64")
                m_architecture = QtSystemInfo::X86_64;
            else if (m_architectureName == "ppc")
                m_architecture = QtSystemInfo::MPPC;
            else if (m_architectureName == "ppc64")
                m_architecture = QtSystemInfo::MPPC64;
        }
    }

    virtual QString architectureName()
    {
        return m_architectureName;
    }

    virtual QtSystemInfo::ArchitectureType architecture()
    {
        return m_architecture;
    }

    virtual QString kernelName()
    {
        return m_kernelName;
    }

    virtual QString kernelVersion()
    {
        return m_kernelVersion;
    }

    virtual QtSystemInfo::SystemType systemType()
    {
        return QtSystemInfo::UnknownUnix;
    }

    virtual QString systemName()
    {
        return "Unix";
    }

    virtual QString systemVersion()
    {
        return UNKNOWN;
    }

private:
    QString m_kernelName;
    QString m_kernelVersion;
    QString m_architectureName;
    QtSystemInfo::ArchitectureType m_architecture;
};
#endif

// -------------------------------------------------- //
//                      Mac OS X                      //
// -------------------------------------------------- //

#ifdef Q_OS_MAC
#include <QSysInfo>

class QtSystemInfoMac: public QtSystemInfoUnix
{
public:
    QtSystemInfoMac() {}

    virtual QString systemName()
    {
        return "Mac OS X";
    }

    virtual QString systemVersion()
    {
        switch (QSysInfo::MacintoshVersion)
        {
        case QSysInfo::MV_10_0:
            return "10.0";
        case QSysInfo::MV_10_1:
            return "10.1";
        case QSysInfo::MV_10_2:
            return "10.2";
        case QSysInfo::MV_10_3:
            return "10.3";
        case QSysInfo::MV_10_4:
            return "10.4";
        case QSysInfo::MV_10_5:
            return "10.5";
        case QSysInfo::MV_10_6:
            return "10.6";
        default:
            return UNKNOWN;
        }
    }

    virtual QtSystemInfo::SystemType systemType()
    {
        return QtSystemInfo::MacOSX;
    }
};
#endif

// -------------------------------------------------- //
//                       Linux                        //
// -------------------------------------------------- //

#ifdef Q_OS_LINUX
#include <QFile>
#include <QRegExp>

static bool scanSuSEReleaseFile(QString &name, QString &version)
{
    name.clear();
    version.clear();

    QFile releaseFile("/etc/SuSE-release");

    if (releaseFile.exists())
    {
        name = "SuSE";

        if (releaseFile.open(QFile::ReadOnly))
        {
            while (!releaseFile.atEnd())
            {
                QString line = releaseFile.readLine(500).trimmed();
                if (line.startsWith("VERSION = "))
                {
                    version = line.remove(0, 10);
                    releaseFile.close();
                    return true;
                }
            }

            releaseFile.close();
        }
    }

    return false;
}

static bool scanRedHatReleaseFile(QString &name, QString &version)
{
    name.clear();
    version.clear();

    QFile releaseFile("/etc/redhat-release");

    if (releaseFile.exists() && releaseFile.open(QFile::ReadOnly))
    {
        QRegExp fedoraRegExp("(^Fedora)\\s+[release]*\\s+([0-9]{1,2})\\s+" );
        QRegExp pclinuxosRegExp("(^PCLinuxOS)\\s+release\\s+([0-9]{4})\\s+\\(PCLinuxOS\\)\\s+for");

        QString line = releaseFile.readLine(500);
        releaseFile.close();

        if (line.contains(fedoraRegExp))
        {
            name = fedoraRegExp.cap(1);
            version = fedoraRegExp.cap(2);

            return true;
        }
        else if (line.contains(pclinuxosRegExp))
        {
            name = pclinuxosRegExp.cap(1);
            version = pclinuxosRegExp.cap(2);

            return true;
        }
    }

    return false;
}

static bool scanLSBReleaseFile(QString &name, QString &version)
{
    name.clear();
    version.clear();

    QFile releaseFile("/etc/lsb-release");

    if (releaseFile.exists() && releaseFile.open(QFile::ReadOnly))
    {
        while (!releaseFile.atEnd())
        {
            QString line = releaseFile.readLine(500).trimmed();

            if (line.startsWith("DISTRIB_ID=") && line.count() > 11)
            {
                name = line.mid(11);

                if (!name.isEmpty() && !version.isEmpty())
                {
                    releaseFile.close();
                    return true;
                }
            }
            else if (line.startsWith("DISTRIB_RELEASE=") && line.count() > 16)
            {
                version = line.mid(16);

                if (!name.isEmpty() && !version.isEmpty())
                {
                    releaseFile.close();
                    return true;
                }
            }
        }
    }

    return false;
}

static bool scanDebianReleaseFile(QString &name, QString &version)
{
    name.clear();
    version.clear();

    QFile releaseFile("/etc/debian_version");

    if (releaseFile.exists() && releaseFile.open(QFile::ReadOnly))
    {
        QRegExp debianRegExp("(^[0-9]{1}\\.[0-9]\\s*$)");

        QString line = releaseFile.readLine(100);
        releaseFile.close();

        if (line.startsWith("lenny/sid"))
        {
            name = "Debian";
            version = "4.0";

            return true;
        }
        else if (line.contains(debianRegExp))
        {
            name = "Debian";
            version = debianRegExp.cap(1);

            return true;
        }
    }

    return false;
}


class QtSystemInfoLinux: public QtSystemInfoUnix
{
public:
    QtSystemInfoLinux()
    {
        if (scanSuSEReleaseFile(m_distributionName, m_distributionVersion))
            return;
        if (scanRedHatReleaseFile(m_distributionName, m_distributionVersion))
            return;
        if (scanLSBReleaseFile(m_distributionName, m_distributionVersion))
            return;
        if (scanDebianReleaseFile(m_distributionName, m_distributionVersion))
            return;

       m_distributionName = "Linux";
       m_distributionVersion = UNKNOWN;
    }

    virtual QtSystemInfo::SystemType systemType()
    {
        return QtSystemInfo::Linux;
    }

    virtual QString systemName()
    {
        return m_distributionName;
    }

    virtual QString systemVersion()
    {
        return m_distributionVersion;
    }

private:
    QString m_distributionName;
    QString m_distributionVersion;
};

#endif

// -------------------------------------------------- //
//                        BSD                         //
// -------------------------------------------------- //

#if defined(Q_OS_BSD4) && !defined(Q_OS_DARWIN)
#include <QFile>
#include <QRegExp>

static QString getFreeBSDDistro()
{
    if (QFile::exists("/usr/PCBSD"))
        return "PCBSD";

    return "FreeBSD";
}

class QtSystemInfoBSD: public QtSystemInfoUnix
{
public:
    QtSystemInfoBSD() {}

    virtual QtSystemInfo::SystemType systemType()
    {
#  if defined(Q_OS_OPENBSD)
        return QtSystemInfo::OpenBSD;
#  elif defined(Q_OS_NETBSD)
        return QtSystemInfo::NetBSD;
#  elif defined(Q_OS_FREEBSD)
        return QtSystemInfo::FreeBSD;
#  else
        return QtSystemInfo::UnknownBSD;
#  endif
    }

    virtual QString systemName()
    {
#  if defined(Q_OS_OPENBSD)
        return "OpenBSD";
#  elif defined(Q_OS_NETBSD)
        return "NetBSD";
#  elif defined(Q_OS_FREEBSD)
        return getFreeBSDDistro();
#  else
        return "BSD4";
#  endif
    }

    virtual QString systemVersion()
    {
        QString version = kernelVersion();
        QRegExp versionRx("^(\\d+\\.\\d+(\\.\\d+)?)");

        if (version.contains(versionRx))
            version = versionRx.cap(1);

        return version;
    }
};
#endif

// -------------------------------------------------- //
//                      Windows                       //
// -------------------------------------------------- //

#ifdef Q_OS_WIN
#include <QSysInfo>

class QtSystemInfoWindows: public QtSystemInfoPrivate
{
public:
    QtSystemInfoWindows() {}

    virtual QtSystemInfo::SystemType systemType()
    {
#  ifdef Q_OS_WINCE
        return QtSystemInfo::WindowsCE;
#  else
        return QtSystemInfo::Windows;
#  endif
    }

    virtual QtSystemInfo::ArchitectureType architecture()
    {
#  if defined(Q_OS_WINCE)
        return QtSystemInfo::UnknownArchitectureType;
#  elif defined(Q_OS_WIN64)
        return QtSystemInfo::X86_64;
#  else
        return QtSystemInfo::I386;
#  endif
    }

    virtual QString architectureName()
    {
        switch (architecture())
        {
        case QtSystemInfo::X86_64:
            return "x86_64";
        case QtSystemInfo::I386:
            return "i386";
        default:
            return UNKNOWN;
        }
    }

    virtual QString kernelName()
    {
        if (QSysInfo::WindowsVersion & QSysInfo::WV_NT_based)
            return "NT";
        else if (QSysInfo::WindowsVersion & QSysInfo::WV_CE_based)
            return "CE";

        return UNKNOWN;
    }

    virtual QString kernelVersion()
    {
        switch (QSysInfo::WindowsVersion)
        {
        case QSysInfo::WV_4_0:
            return "4.0";
        case QSysInfo::WV_5_0:
            return "5.0";
        case QSysInfo::WV_5_1:
            return "5.1";
        case QSysInfo::WV_5_2:
            return "5.2";
        case QSysInfo::WV_6_0:
            return "6.0";
        case QSysInfo::WV_6_1:
            return "6.1";
        case QSysInfo::WV_CE:
            return "1.0";
        case QSysInfo::WV_CENET:
            return ".NET";
        case QSysInfo::WV_CE_5:
            return "5.x";
        case QSysInfo::WV_CE_6:
            return "6.x";
        default:
            return UNKNOWN;
        }
    }

    virtual QString systemName()
    {
#  ifdef Q_OS_WINCE
        return "Windows CE";
#  else
        return "Windows";
#  endif
    }

    virtual QString systemVersion()
    {
        switch (QSysInfo::WindowsVersion)
        {
        case QSysInfo::WV_NT:
            return "NT";
        case QSysInfo::WV_2000:
            return "2000";
        case QSysInfo::WV_XP:
            return "XP";
        case QSysInfo::WV_2003:
#  ifdef Q_OS_WIN64
            return "XP Proffessional 64";
#  else
            return "2003 Server";
#  endif
        case QSysInfo::WV_VISTA:
            return "Vista / Server 2008";
        case QSysInfo::WV_WINDOWS7:
            return "7 / Server 2008 R2";
        default:
            return UNKNOWN;
        }
    }
};
#endif

// -------------------------------------------------- //
//                     Main class                     //
// -------------------------------------------------- //

QtSystemInfo::QtSystemInfo(QObject *parent):
        QObject(parent),
#if defined(Q_OS_UNIX)
#  if defined(Q_OS_MAC)
        d(new QtSystemInfoMac())
#  elif defined(Q_OS_LINUX)
        d(new QtSystemInfoLinux())
#  elif defined(Q_OS_BSD4)
        d(new QtSystemInfoBSD)
#  else
        d(new QtSystemInfoUnix())
#  endif
#elif defined(Q_OS_WIN)
        d(new QtSystemInfoWindows())
#endif
{
}

QtSystemInfo::~QtSystemInfo()
{
    delete d;
}

QString QtSystemInfo::architectureName() const
{
    return d->architectureName();
}

QtSystemInfo::ArchitectureType QtSystemInfo::architecture() const
{
    return d->architecture();
}

QString QtSystemInfo::kernelVersion() const
{
    return d->kernelVersion();
}

QString QtSystemInfo::kernelName() const
{
    return d->kernelName();
}

QString QtSystemInfo::systemName() const
{
    return d->systemName();
}

QString QtSystemInfo::systemVersion() const
{
    return d->systemVersion();
}

QtSystemInfo::SystemType QtSystemInfo::systemType() const
{
    return d->systemType();
}

QString QtSystemInfo::getSystemInformation(QString format)
{
    if (format.isEmpty())
        format = "%SYS_NAME %SYS_VERSION (%KERN_NAME %KERN_VERSION %ARCH).";

    format.replace("%SYS_NAME",     systemName(),       Qt::CaseInsensitive);
    format.replace("%SYS_VERSION",  systemVersion(),    Qt::CaseInsensitive);
    format.replace("%SYS_RELEASE",  systemVersion(),    Qt::CaseInsensitive);
    format.replace("%KERN_NAME",    kernelName(),       Qt::CaseInsensitive);
    format.replace("%KERN_RELEASE", kernelVersion(),    Qt::CaseInsensitive);
    format.replace("%KERN_VERSION", kernelVersion(),    Qt::CaseInsensitive);
    format.replace("%ARCH",         architectureName(), Qt::CaseInsensitive);

    return format;
}
