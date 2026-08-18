#ifndef BREADBOARDCSVLIBREPCBPROVIDER_H
#define BREADBOARDCSVLIBREPCBPROVIDER_H

#include <QHash>
#include <QList>
#include <QString>

struct BreadboardCsvLibrePcbResolution
{
        bool matched = false;

        QString error;

        QString identifier;
        QString mpn;
        QString manufacturer;

        QString deviceUuid;
        QString componentUuid;
        QString packageUuid;

        QString deviceFilePath;
        QString componentFilePath;
        QString packageFilePath;

        QString packageName;

        int pinCount = 0;
        int pitchMil = 0;
        int spacingMil = 0;

        /*
         * Physical package pin number -> electrical signal name.
         *
         * This mapping comes from:
         *
         *   Package pad UUID
         *       -> Device signal UUID
         *       -> Component signal name
         *
         * It is deliberately independent of Fritzing connector IDs.
         */
        QHash<int, QString> pinSignalNames;
};

struct BreadboardCsvLibrePcbCandidate
{
        QString mpn;
        QString manufacturer;
        QString deviceUuid;
};

class BreadboardCsvLibrePcbProvider
{
public:
        /*
         * Search is candidate discovery only.
         *
         * A returned candidate is NOT an automatic alias or identity
         * decision. The caller must require explicit selection before
         * persisting a CSV-label -> exact-MPN association.
         *
         * Matching uses literal case-insensitive substring matching on
         * MPNs in the LibrePCB cache. SQLite remains only an index.
         */
        static QList<BreadboardCsvLibrePcbCandidate> findMpnCandidates(
                const QString &librariesRoot,
                const QString &query,
                QString &error,
                int maxResults = 50
        );

        /*
         * librariesRoot is the LibrePCB workspace directory containing
         * cache_vN.sqlite and the LibrePCB remote library directories.
         *
         * Example:
         *
         *   ~/LibrePCB-Workspace/data/libraries
         *
         * The SQLite cache is used only as an index to locate the exact
         * Device, Component and Package files. Electrical/package data
         * is validated from the .lp files themselves.
         */
        static BreadboardCsvLibrePcbResolution resolveExactMpn(
                const QString &librariesRoot,
                const QString &identifier,
                int expectedPinCount = 0
        );
};

#endif
