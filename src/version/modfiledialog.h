#ifndef MODFILEDIALOG_H
#define MODFILEDIALOG_H

#include <QDialog>

namespace Ui {
class ModFileDialog;
}

class ModFileDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ModFileDialog(QWidget *parent = 0);
    ~ModFileDialog();

    void setText(const QString & text);
    void addList(const QString & header, const QStringList & list);

protected slots:
    /**
     * override the default closing behavior to send the cleanRepo signal
     */
    void accept();

signals:
    void cleanRepo(ModFileDialog * modFileDialog);

private:
    Ui::ModFileDialog *ui;
};

#endif // MODFILEDIALOG_H
