/********************************************************************************
** Form generated from reading UI file 'modfiledialog.ui'
**
** Created by: Qt User Interface Compiler version 5.6.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MODFILEDIALOG_H
#define UI_MODFILEDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QHeaderView>
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
            ModFileDialog->setObjectName(QStringLiteral("ModFileDialog"));
        ModFileDialog->resize(400, 300);
        verticalLayout = new QVBoxLayout(ModFileDialog);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        label = new QLabel(ModFileDialog);
        label->setObjectName(QStringLiteral("label"));

        verticalLayout->addWidget(label);

        decision = new QLabel(ModFileDialog);
        decision->setObjectName(QStringLiteral("decision"));

        verticalLayout->addWidget(decision);

        listWidget = new QListWidget(ModFileDialog);
        listWidget->setObjectName(QStringLiteral("listWidget"));

        verticalLayout->addWidget(listWidget);

        buttonBox = new QDialogButtonBox(ModFileDialog);
        buttonBox->setObjectName(QStringLiteral("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(ModFileDialog);
        QObject::connect(buttonBox, SIGNAL(accepted()), ModFileDialog, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), ModFileDialog, SLOT(reject()));

        QMetaObject::connectSlotsByName(ModFileDialog);
    } // setupUi

    void retranslateUi(QDialog *ModFileDialog)
    {
        ModFileDialog->setWindowTitle(QApplication::translate("ModFileDialog", "Modified files", 0));
        label->setText(QString());
        decision->setText(QApplication::translate("ModFileDialog", "decision", 0));
    } // retranslateUi

};

namespace Ui {
    class ModFileDialog: public Ui_ModFileDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MODFILEDIALOG_H
