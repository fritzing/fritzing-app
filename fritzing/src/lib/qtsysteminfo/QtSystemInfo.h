/*
 * File:      QtSystemInfo.h
 * Author:    Adrian Łubik (adrian5632@gmail.com)
 * License:   GNU LGPL v2 or newer
 * Copyright: (c) 2010 Adrian Łubik
 */

#ifndef QTSYSTEMINFO_H
#define QTSYSTEMINFO_H

#define QTSYSTEMINFO_VERSION 0x009000

#include <QObject>
#include <QString>

class QtSystemInfoPrivate;

class QtSystemInfo: public QObject
{
public:
    enum SystemType
    {
        UnknownType = -1,
        UnknownUnix,
        MacOSX,
        Windows,
        WindowsCE,
        Linux,
        UnknownBSD,
        FreeBSD,
        OpenBSD,
        NetBSD
    };

    enum ArchitectureType
    {
        UnknownArchitectureType = -1,
        I386,
        I486,
        I586,
        I686,
        X86_64,
        MPPC,
        MPPC64
    };

    QtSystemInfo(QObject *parent = 0);
    ~QtSystemInfo();

    /**
      * @return Name of currently running system's kernel.
      */
    QString kernelName() const;

    /**
      * @return Version of currently running system's kernel.
      */
    QString kernelVersion() const;

    /**
      * @return Architecture of the running system.
      */
    QString architectureName() const;

    /**
      * @return Type of currently running system's architecture.
      */
    ArchitectureType architecture() const;

    /**
      * @return Type of currently running system.
      */
    SystemType systemType() const;

    /**
      * @return Name of currently running system.
      */
    QString systemName() const;

    /**
      * @return Version of currently running system.
      */
    QString systemVersion() const;

    /**
      * @param format A format of the information to be returned.
      * The following strings will be replaced:
      * %SYS_NAME - replaced with system name.
      * %SYS_VERSION - replaced with system version.
      * %KERN_NAME - replaced with kernel name.
      * %KERN_VERSION - replaced with kernel version.
      * %ARCH - replaced with architecture.
      * If no format given, the method will default to: %SYS_NAME %SYS_VERSION (%KERN_NAME %KERN_VERSION %ARCH).
      *
      * @return Formatted information about the running system.
      */
    QString getSystemInformation(QString format = QString());

private:
    QtSystemInfoPrivate *d;
};


#endif
