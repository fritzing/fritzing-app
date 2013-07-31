#ifndef APPLICATION_H
#define APPLICATION_H

#include <QCoreApplication>
#include <QDir>
#include <QString>
#include <QRectF>
#include <QDomElement>
#include <QSvgRenderer>


struct ConnectorLocation {
    QString svgID;
    QString terminalID;
    QString name;
    int id;
    QRectF bounds;
    QPointF terminalPoint;
    bool hidden;

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
    double stringWidthMM(double fontSize, const QString & string);
    QList<ConnectorLocation *> initConnectors(const QDomElement & root, const QSvgRenderer &, const QString & fzpFilename, const QString & svgFilename);
    double lrtb(QList<ConnectorLocation *> &, const QRectF & viewBox);
    void setHidden(QList<ConnectorLocation *> &);
    bool ensureTerminalPoints(const QString & fzpFilename, const QString & svgFilename, QDomElement & fzpRoot);


protected:
    QString m_fzpPath;
    QDir m_fzpDir;
    QString m_oldSvgPath;
    QDir m_oldSvgDir;
    QString m_newSvgPath;
    QDir m_newSvgDir;
    QString m_filePath;
    QString m_andPath;
    double m_minLeft;
    double m_minTop;
    double m_maxRight;
    double m_maxBottom;
    QList<ConnectorLocation *> m_lefts;
    QList<ConnectorLocation *> m_tops;
    QList<ConnectorLocation *> m_rights;
    QList<ConnectorLocation *> m_bottoms;
    QImage * m_image;
    double m_fudge;
};



#endif // APPLICATION_H
