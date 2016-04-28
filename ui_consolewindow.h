/********************************************************************************
** Form generated from reading UI file 'consolewindow.ui'
**
** Created by: Qt User Interface Compiler version 5.2.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CONSOLEWINDOW_H
#define UI_CONSOLEWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ConsoleWindow
{
public:
    QAction *actionAbout;
    QAction *actionAboutQt;
    QAction *actionConnect;
    QAction *actionDisconnect;
    QAction *actionConfigure;
    QAction *actionClear;
    QAction *actionQuit;
    QWidget *centralWidget;
    QVBoxLayout *verticalLayout;
    QMenuBar *menuBar;
    QMenu *menuCalls;
    QMenu *menuTools;
    QToolBar *mainToolBar;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *ConsoleWindow)
    {
        if (ConsoleWindow->objectName().isEmpty())
            ConsoleWindow->setObjectName(QStringLiteral("ConsoleWindow"));
        ConsoleWindow->resize(400, 300);
        actionAbout = new QAction(ConsoleWindow);
        actionAbout->setObjectName(QStringLiteral("actionAbout"));
        actionAboutQt = new QAction(ConsoleWindow);
        actionAboutQt->setObjectName(QStringLiteral("actionAboutQt"));
        actionConnect = new QAction(ConsoleWindow);
        actionConnect->setObjectName(QStringLiteral("actionConnect"));
        actionDisconnect = new QAction(ConsoleWindow);
        actionDisconnect->setObjectName(QStringLiteral("actionDisconnect"));
        actionConfigure = new QAction(ConsoleWindow);
        actionConfigure->setObjectName(QStringLiteral("actionConfigure"));
        actionClear = new QAction(ConsoleWindow);
        actionClear->setObjectName(QStringLiteral("actionClear"));
        actionQuit = new QAction(ConsoleWindow);
        actionQuit->setObjectName(QStringLiteral("actionQuit"));
        QIcon icon;
        icon.addFile(QStringLiteral(":/images/application-exit.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionQuit->setIcon(icon);
        centralWidget = new QWidget(ConsoleWindow);
        centralWidget->setObjectName(QStringLiteral("centralWidget"));
        verticalLayout = new QVBoxLayout(centralWidget);
        verticalLayout->setSpacing(6);
        verticalLayout->setContentsMargins(11, 11, 11, 11);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        ConsoleWindow->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(ConsoleWindow);
        menuBar->setObjectName(QStringLiteral("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 400, 22));
        menuCalls = new QMenu(menuBar);
        menuCalls->setObjectName(QStringLiteral("menuCalls"));
        menuTools = new QMenu(menuBar);
        menuTools->setObjectName(QStringLiteral("menuTools"));
        ConsoleWindow->setMenuBar(menuBar);
        mainToolBar = new QToolBar(ConsoleWindow);
        mainToolBar->setObjectName(QStringLiteral("mainToolBar"));
        ConsoleWindow->addToolBar(Qt::TopToolBarArea, mainToolBar);
        statusBar = new QStatusBar(ConsoleWindow);
        statusBar->setObjectName(QStringLiteral("statusBar"));
        ConsoleWindow->setStatusBar(statusBar);

        menuBar->addAction(menuCalls->menuAction());
        menuBar->addAction(menuTools->menuAction());
        menuCalls->addAction(actionConnect);
        menuCalls->addAction(actionDisconnect);
        menuCalls->addSeparator();
        menuCalls->addAction(actionQuit);
        menuTools->addAction(actionConfigure);
        menuTools->addAction(actionClear);
        mainToolBar->addAction(actionConnect);
        mainToolBar->addAction(actionDisconnect);
        mainToolBar->addAction(actionConfigure);
        mainToolBar->addAction(actionClear);

        retranslateUi(ConsoleWindow);

        QMetaObject::connectSlotsByName(ConsoleWindow);
    } // setupUi

    void retranslateUi(QMainWindow *ConsoleWindow)
    {
        ConsoleWindow->setWindowTitle(QApplication::translate("ConsoleWindow", "Serial Monitor", 0));
        actionAbout->setText(QApplication::translate("ConsoleWindow", "&About", 0));
#ifndef QT_NO_TOOLTIP
        actionAbout->setToolTip(QApplication::translate("ConsoleWindow", "About program", 0));
#endif // QT_NO_TOOLTIP
        actionAbout->setShortcut(QApplication::translate("ConsoleWindow", "Alt+A", 0));
        actionAboutQt->setText(QApplication::translate("ConsoleWindow", "About Qt", 0));
        actionConnect->setText(QApplication::translate("ConsoleWindow", "C&onnect", 0));
#ifndef QT_NO_TOOLTIP
        actionConnect->setToolTip(QApplication::translate("ConsoleWindow", "Connect to serial port", 0));
#endif // QT_NO_TOOLTIP
        actionConnect->setShortcut(QApplication::translate("ConsoleWindow", "Ctrl+O", 0));
        actionDisconnect->setText(QApplication::translate("ConsoleWindow", "&Disconnect", 0));
#ifndef QT_NO_TOOLTIP
        actionDisconnect->setToolTip(QApplication::translate("ConsoleWindow", "Disconnect from serial port", 0));
#endif // QT_NO_TOOLTIP
        actionDisconnect->setShortcut(QApplication::translate("ConsoleWindow", "Ctrl+D", 0));
        actionConfigure->setText(QApplication::translate("ConsoleWindow", "&Configure", 0));
#ifndef QT_NO_TOOLTIP
        actionConfigure->setToolTip(QApplication::translate("ConsoleWindow", "Configure serial port", 0));
#endif // QT_NO_TOOLTIP
        actionConfigure->setShortcut(QApplication::translate("ConsoleWindow", "Alt+C", 0));
        actionClear->setText(QApplication::translate("ConsoleWindow", "C&lear", 0));
#ifndef QT_NO_TOOLTIP
        actionClear->setToolTip(QApplication::translate("ConsoleWindow", "Clear data", 0));
#endif // QT_NO_TOOLTIP
        actionClear->setShortcut(QApplication::translate("ConsoleWindow", "Alt+L", 0));
        actionQuit->setText(QApplication::translate("ConsoleWindow", "&Quit", 0));
        actionQuit->setShortcut(QApplication::translate("ConsoleWindow", "Ctrl+Q", 0));
        menuCalls->setTitle(QApplication::translate("ConsoleWindow", "Monitor", 0));
        menuTools->setTitle(QApplication::translate("ConsoleWindow", "Tools", 0));
    } // retranslateUi

};

namespace Ui {
    class ConsoleWindow: public Ui_ConsoleWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CONSOLEWINDOW_H
