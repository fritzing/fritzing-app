#ifndef APPLICATION_H
#define APPLICATION_H

#include <QCoreApplication>
#include <QDir>
#include <QString>
#include <QRectF>


struct ConnectorLocation {
    QString svgID;
    QString terminalID;
    QString name;
    QRectF bounds;
    QPointF terminalPoint;

    enum Side {
        Unknown,
        Left,
        Top,
        Right,
        Bottom
    };
};

class S2SApplication : public QCoreApplication
{
public:
    S2SApplication(int argc, char *argv[]);
    void start();

protected:
    void usage();
    void message(const QString &);
    bool initArguments();
    void saveFile(const QString & content, const QString & path);
    void onefzp(QString & filename);

protected:
    QString m_fzpPath;
    QDir m_fzpDir;
    QString m_oldSvgPath;
    QDir m_oldSvgDir;
    QString m_newSvgPath;
    QDir m_newSvgDir;
    QString m_filePath;
    QString m_andPath;
};



#endif // APPLICATION_H
