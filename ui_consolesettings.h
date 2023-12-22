/********************************************************************************
** Form generated from reading UI file 'consolesettings.ui'
**
** Created by: Qt User Interface Compiler version 6.6.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CONSOLESETTINGS_H
#define UI_CONSOLESETTINGS_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_ConsoleSettings
{
public:
    QGridLayout *gridLayout_3;
    QGroupBox *parametersBox;
    QGridLayout *gridLayout_2;
    QLabel *baudRateLabel;
    QComboBox *baudRateBox;
    QLabel *dataBitsLabel;
    QComboBox *dataBitsBox;
    QLabel *parityLabel;
    QComboBox *parityBox;
    QLabel *stopBitsLabel;
    QComboBox *stopBitsBox;
    QLabel *flowControlLabel;
    QComboBox *flowControlBox;
    QGroupBox *selectBox;
    QGridLayout *gridLayout;
    QComboBox *serialPortInfoListBox;
    QLabel *descriptionLabel;
    QLabel *manufacturerLabel;
    QLabel *serialNumberLabel;
    QLabel *locationLabel;
    QLabel *vidLabel;
    QLabel *pidLabel;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer;
    QPushButton *applyButton;
    QGroupBox *additionalOptionsGroupBox;
    QVBoxLayout *verticalLayout;
    QCheckBox *localEchoCheckBox;

    void setupUi(QDialog *ConsoleSettings)
    {
        if (ConsoleSettings->objectName().isEmpty())
            ConsoleSettings->setObjectName("ConsoleSettings");
        ConsoleSettings->resize(400, 336);
        gridLayout_3 = new QGridLayout(ConsoleSettings);
        gridLayout_3->setObjectName("gridLayout_3");
        parametersBox = new QGroupBox(ConsoleSettings);
        parametersBox->setObjectName("parametersBox");
        gridLayout_2 = new QGridLayout(parametersBox);
        gridLayout_2->setObjectName("gridLayout_2");
        baudRateLabel = new QLabel(parametersBox);
        baudRateLabel->setObjectName("baudRateLabel");

        gridLayout_2->addWidget(baudRateLabel, 0, 0, 1, 1);

        baudRateBox = new QComboBox(parametersBox);
        baudRateBox->setObjectName("baudRateBox");

        gridLayout_2->addWidget(baudRateBox, 0, 1, 1, 1);

        dataBitsLabel = new QLabel(parametersBox);
        dataBitsLabel->setObjectName("dataBitsLabel");

        gridLayout_2->addWidget(dataBitsLabel, 1, 0, 1, 1);

        dataBitsBox = new QComboBox(parametersBox);
        dataBitsBox->setObjectName("dataBitsBox");

        gridLayout_2->addWidget(dataBitsBox, 1, 1, 1, 1);

        parityLabel = new QLabel(parametersBox);
        parityLabel->setObjectName("parityLabel");

        gridLayout_2->addWidget(parityLabel, 2, 0, 1, 1);

        parityBox = new QComboBox(parametersBox);
        parityBox->setObjectName("parityBox");

        gridLayout_2->addWidget(parityBox, 2, 1, 1, 1);

        stopBitsLabel = new QLabel(parametersBox);
        stopBitsLabel->setObjectName("stopBitsLabel");

        gridLayout_2->addWidget(stopBitsLabel, 3, 0, 1, 1);

        stopBitsBox = new QComboBox(parametersBox);
        stopBitsBox->setObjectName("stopBitsBox");

        gridLayout_2->addWidget(stopBitsBox, 3, 1, 1, 1);

        flowControlLabel = new QLabel(parametersBox);
        flowControlLabel->setObjectName("flowControlLabel");

        gridLayout_2->addWidget(flowControlLabel, 4, 0, 1, 1);

        flowControlBox = new QComboBox(parametersBox);
        flowControlBox->setObjectName("flowControlBox");

        gridLayout_2->addWidget(flowControlBox, 4, 1, 1, 1);


        gridLayout_3->addWidget(parametersBox, 0, 1, 1, 1);

        selectBox = new QGroupBox(ConsoleSettings);
        selectBox->setObjectName("selectBox");
        gridLayout = new QGridLayout(selectBox);
        gridLayout->setObjectName("gridLayout");
        serialPortInfoListBox = new QComboBox(selectBox);
        serialPortInfoListBox->setObjectName("serialPortInfoListBox");

        gridLayout->addWidget(serialPortInfoListBox, 0, 0, 1, 1);

        descriptionLabel = new QLabel(selectBox);
        descriptionLabel->setObjectName("descriptionLabel");

        gridLayout->addWidget(descriptionLabel, 1, 0, 1, 1);

        manufacturerLabel = new QLabel(selectBox);
        manufacturerLabel->setObjectName("manufacturerLabel");

        gridLayout->addWidget(manufacturerLabel, 2, 0, 1, 1);

        serialNumberLabel = new QLabel(selectBox);
        serialNumberLabel->setObjectName("serialNumberLabel");

        gridLayout->addWidget(serialNumberLabel, 3, 0, 1, 1);

        locationLabel = new QLabel(selectBox);
        locationLabel->setObjectName("locationLabel");

        gridLayout->addWidget(locationLabel, 4, 0, 1, 1);

        vidLabel = new QLabel(selectBox);
        vidLabel->setObjectName("vidLabel");

        gridLayout->addWidget(vidLabel, 5, 0, 1, 1);

        pidLabel = new QLabel(selectBox);
        pidLabel->setObjectName("pidLabel");

        gridLayout->addWidget(pidLabel, 6, 0, 1, 1);


        gridLayout_3->addWidget(selectBox, 0, 0, 1, 1);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        horizontalSpacer = new QSpacerItem(96, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        applyButton = new QPushButton(ConsoleSettings);
        applyButton->setObjectName("applyButton");

        horizontalLayout->addWidget(applyButton);


        gridLayout_3->addLayout(horizontalLayout, 2, 0, 1, 2);

        additionalOptionsGroupBox = new QGroupBox(ConsoleSettings);
        additionalOptionsGroupBox->setObjectName("additionalOptionsGroupBox");
        verticalLayout = new QVBoxLayout(additionalOptionsGroupBox);
        verticalLayout->setObjectName("verticalLayout");
        localEchoCheckBox = new QCheckBox(additionalOptionsGroupBox);
        localEchoCheckBox->setObjectName("localEchoCheckBox");
        localEchoCheckBox->setChecked(true);

        verticalLayout->addWidget(localEchoCheckBox);


        gridLayout_3->addWidget(additionalOptionsGroupBox, 1, 0, 1, 2);


        retranslateUi(ConsoleSettings);

        QMetaObject::connectSlotsByName(ConsoleSettings);
    } // setupUi

    void retranslateUi(QDialog *ConsoleSettings)
    {
        ConsoleSettings->setWindowTitle(QCoreApplication::translate("ConsoleSettings", "Settings", nullptr));
        parametersBox->setTitle(QCoreApplication::translate("ConsoleSettings", "Select Parameters", nullptr));
        baudRateLabel->setText(QCoreApplication::translate("ConsoleSettings", "BaudRate:", nullptr));
        dataBitsLabel->setText(QCoreApplication::translate("ConsoleSettings", "Data bits:", nullptr));
        parityLabel->setText(QCoreApplication::translate("ConsoleSettings", "Parity:", nullptr));
        stopBitsLabel->setText(QCoreApplication::translate("ConsoleSettings", "Stop bits:", nullptr));
        flowControlLabel->setText(QCoreApplication::translate("ConsoleSettings", "Flow control:", nullptr));
        selectBox->setTitle(QCoreApplication::translate("ConsoleSettings", "Select Serial Port", nullptr));
        descriptionLabel->setText(QCoreApplication::translate("ConsoleSettings", "Description:", nullptr));
        manufacturerLabel->setText(QCoreApplication::translate("ConsoleSettings", "Manufacturer:", nullptr));
        serialNumberLabel->setText(QCoreApplication::translate("ConsoleSettings", "Serial number:", nullptr));
        locationLabel->setText(QCoreApplication::translate("ConsoleSettings", "Location:", nullptr));
        vidLabel->setText(QCoreApplication::translate("ConsoleSettings", "Vendor ID:", nullptr));
        pidLabel->setText(QCoreApplication::translate("ConsoleSettings", "Product ID:", nullptr));
        applyButton->setText(QCoreApplication::translate("ConsoleSettings", "Apply", nullptr));
        additionalOptionsGroupBox->setTitle(QCoreApplication::translate("ConsoleSettings", "Additional options", nullptr));
        localEchoCheckBox->setText(QCoreApplication::translate("ConsoleSettings", "Local echo", nullptr));
    } // retranslateUi

};

namespace Ui {
    class ConsoleSettings: public Ui_ConsoleSettings {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CONSOLESETTINGS_H
