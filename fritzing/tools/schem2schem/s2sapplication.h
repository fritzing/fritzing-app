#ifndef APPLICATION_H
#define APPLICATION_H

#include <QCoreApplication>
#include <QDir>
#include <QString>
#include <QRectF>
#include <QDomElement>
#include <QSvgRenderer>

class S2SApplication : public QCoreApplication
{
    Q_OBJECT
public:
    S2SApplication(int argc, char *argv[]);
    void start();

protected:
    void usage();
    void message(const QString &);
    bool initArguments();
    void saveFile(const QString & content, const QString & path);

protected:
    QString m_fzpPath;
    QDir m_fzpDir;
    QString m_oldSvgPath;
    QDir m_oldSvgDir;
    QString m_newSvgPath;
    QDir m_newSvgDir;
    QString m_filePath;
    bool m_fzpzStyle;
};



#endif // APPLICATION_H
