/****************************************************************************
**
** Copyright (C) 2012 Denis Shienkov <denis.shienkov@gmail.com>
** Copyright (C) 2012 Laszlo Papp <lpapp@kde.org>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtSerialPort module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "consolewindow.h"
#include "ui_consolewindow.h"
#include "console.h"
#include "consolesettings.h"

#include <QFile>
#include <QMessageBox>
#include <QSettings>
#include <QtSerialPort/QSerialPort>

ConsoleWindow::ConsoleWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ConsoleWindow)
{
    QFile styleSheet(":/resources/styles/programwindow.qss");

    this->setObjectName("consoleWindow");
    if (!styleSheet.open(QIODevice::ReadOnly)) {
        qWarning("Unable to open :/resources/styles/programwindow.qss");
    } else {
        QString ss = styleSheet.readAll();
#ifdef Q_OS_MAC
                int paneLoc = 4;
                int tabBarLoc = 0;
#else
                int paneLoc = -1;
                int tabBarLoc = 5;
#endif
                ss = ss.arg(paneLoc).arg(tabBarLoc);
        this->setStyleSheet(ss);
    }

    ui->setupUi(this);
    console = new Console;
    console->setEnabled(false);
    setCentralWidget(console);
    serial = new QSerialPort(this);
    settings = new ConsoleSettings;

    QSettings settings;
    if (!settings.value("consolewindow/state").isNull()) {
        restoreState(settings.value("consolewindow/state").toByteArray());
    }
    if (!settings.value("consolewindow/geometry").isNull()) {
        restoreGeometry(settings.value("consolewindow/geometry").toByteArray());
    }

    ui->actionConnect->setEnabled(true);
    ui->actionDisconnect->setEnabled(false);
    ui->actionQuit->setEnabled(true);
    ui->actionConfigure->setEnabled(true);

    initActionsConnections();

    connect(serial, SIGNAL(error(QSerialPort::SerialPortError)), this,
            SLOT(handleError(QSerialPort::SerialPortError)));

    connect(serial, SIGNAL(readyRead()), this, SLOT(readData()));
    connect(console, SIGNAL(getData(QByteArray)), this, SLOT(writeData(QByteArray)));
}

ConsoleWindow::~ConsoleWindow()
{
    delete settings;
    delete ui;
}

void ConsoleWindow::closeEvent(QCloseEvent *event)
 {
     closeSerialPort();
     QSettings settings;
     settings.setValue("consolewindow/geometry", saveGeometry());
     settings.setValue("consolewindow/tate", saveState());
     QMainWindow::closeEvent(event);
 }

void ConsoleWindow::openSerialPort(const QString portName)
{
    if (portName.isEmpty()) return;

    settings->selectPortName(portName);
    if (serial->isOpen()) {
        if (serial->portName().compare(portName) != 0) {
            closeSerialPort();
            openSerialPort();
        }
    } else {
        openSerialPort();
    }
}

void ConsoleWindow::openSerialPort()
{
    ConsoleSettings::Settings p = settings->settings();
    serial->setPortName(p.name);
    if (serial->open(QIODevice::ReadWrite)) {
        serial->setBaudRate(p.baudRate);
        serial->setDataBits(p.dataBits);
        serial->setParity(p.parity);
        serial->setStopBits(p.stopBits);
        serial->setFlowControl(p.flowControl);
        console->setEnabled(true);
        console->setLocalEchoEnabled(p.localEchoEnabled);
        ui->actionConnect->setEnabled(false);
        ui->actionDisconnect->setEnabled(true);
        ui->actionConfigure->setEnabled(false);
        ui->statusBar->showMessage(tr("Connected to %1 : %2, %3, %4, %5, %6")
                                   .arg(p.name).arg(p.stringBaudRate).arg(p.stringDataBits)
                                   .arg(p.stringParity).arg(p.stringStopBits).arg(p.stringFlowControl));
    } else {
        QMessageBox::critical(this, tr("Error"), serial->errorString());

        ui->statusBar->showMessage(tr("Serial port open error"));
    }
}

void ConsoleWindow::closeSerialPort(const QString portName)
{
    if (portName.isEmpty()) return;
    if (portName.compare(serial->portName()) == 0) {
        closeSerialPort();
    }
}

void ConsoleWindow::closeSerialPort()
{
    if (serial->isOpen()) {
        serial->close();
        console->setEnabled(false);
        ui->actionConnect->setEnabled(true);
        ui->actionDisconnect->setEnabled(false);
        ui->actionConfigure->setEnabled(true);
        ui->statusBar->showMessage(tr("Disconnected"));
    }
}

void ConsoleWindow::about()
{
    QMessageBox::about(this, tr("About Serial Monitor"),
                       tr("This terminal displays the serial communication on the "
                          "selected port, usually between your computer and the "
                          "connected microcontroller."));
}

void ConsoleWindow::writeData(const QByteArray &data)
{
    serial->write(data);
}

void ConsoleWindow::readData()
{
    QByteArray data = serial->readAll();
    console->putData(data);
}

void ConsoleWindow::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError) {
        QMessageBox::critical(this, tr("Critical Error"), serial->errorString());
        closeSerialPort();
    }
}

void ConsoleWindow::initActionsConnections()
{
    connect(ui->actionConnect, SIGNAL(triggered()), this, SLOT(openSerialPort()));
    connect(ui->actionDisconnect, SIGNAL(triggered()), this, SLOT(closeSerialPort()));
    connect(ui->actionQuit, SIGNAL(triggered()), this, SLOT(close()));
    connect(ui->actionConfigure, SIGNAL(triggered()), settings, SLOT(show()));
    connect(ui->actionClear, SIGNAL(triggered()), console, SLOT(clear()));
    connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(about()));
    connect(ui->actionAboutQt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
}
