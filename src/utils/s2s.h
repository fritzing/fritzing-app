#ifndef S2S_H
#define S2S_H

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
    bool displayPinNumber;

    enum Side {
        Unknown,
        Left,
        Top,
        Right,
        Bottom
    };
};

class S2S : public QObject
{
    Q_OBJECT
public:
    S2S(bool fzpzStyle);

    bool onefzp(QString & fzpFilePath, QString & schematicFilePath);
    void setSvgDirs(QDir & oldDir, QDir & newDir);


signals:
    void messageSignal(const QString & message);

protected:
    void message(const QString &);
    void saveFile(const QString & content, const QString & path);
    double stringWidthMM(double fontSize, const QString & string);
    QList<ConnectorLocation *> initConnectors(const QDomElement & root, const QSvgRenderer &, const QString & fzpFilename, const QString & svgFilename);
    double lrtb(QList<ConnectorLocation *> &, const QRectF & viewBox);
    void setHidden(QList<ConnectorLocation *> &);
    bool ensureTerminalPoints(const QString & fzpFilename, const QString & svgFilename, QDomElement & fzpRoot);
    double spaceTitle(QStringList & titles, int openUnits);


protected:
    bool m_fzpzStyle;
    QString m_andPath;
    double m_minLeft;
    double m_minTop;
    double m_maxRight;
    double m_maxBottom;
    QList<ConnectorLocation *> m_lefts;
    QList<ConnectorLocation *> m_tops;
    QList<ConnectorLocation *> m_rights;
    QList<ConnectorLocation *> m_bottoms;
    double m_fudge;
    QDir m_oldSvgDir;
    QDir m_newSvgDir;
    QImage * m_image;
};



#endif // S2S_H
