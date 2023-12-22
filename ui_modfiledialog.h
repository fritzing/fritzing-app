/********************************************************************************
** Form generated from reading UI file 'modfiledialog.ui'
**
** Created by: Qt User Interface Compiler version 6.6.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MODFILEDIALOG_H
#define UI_MODFILEDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_ModFileDialog
{
public:
    QVBoxLayout *verticalLayout;
    QLabel *label;
    QLabel *decision;
    QListWidget *listWidget;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *ModFileDialog)
    {
        if (ModFileDialog->objectName().isEmpty())
            ModFileDialog->setObjectName("ModFileDialog");
        ModFileDialog->resize(400, 300);
        verticalLayout = new QVBoxLayout(ModFileDialog);
        verticalLayout->setObjectName("verticalLayout");
        label = new QLabel(ModFileDialog);
        label->setObjectName("label");

        verticalLayout->addWidget(label);

        decision = new QLabel(ModFileDialog);
        decision->setObjectName("decision");

        verticalLayout->addWidget(decision);

        listWidget = new QListWidget(ModFileDialog);
        listWidget->setObjectName("listWidget");

        verticalLayout->addWidget(listWidget);

        buttonBox = new QDialogButtonBox(ModFileDialog);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(ModFileDialog);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, ModFileDialog, qOverload<>(&QDialog::accept));
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, ModFileDialog, qOverload<>(&QDialog::reject));

        QMetaObject::connectSlotsByName(ModFileDialog);
    } // setupUi

    void retranslateUi(QDialog *ModFileDialog)
    {
        ModFileDialog->setWindowTitle(QCoreApplication::translate("ModFileDialog", "Modified files", nullptr));
        label->setText(QString());
        decision->setText(QCoreApplication::translate("ModFileDialog", "decision", nullptr));
    } // retranslateUi

};

namespace Ui {
    class ModFileDialog: public Ui_ModFileDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MODFILEDIALOG_H
