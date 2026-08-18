#ifndef BREADBOARDCSVCOMPONENTRESOLVER_H
#define BREADBOARDCSVCOMPONENTRESOLVER_H

#include <QHash>
#include <QString>

class ReferenceModel;

struct BreadboardCsvComponentResolution
{
        bool matched = false;
        bool nativePart = false;

        QString error;

        QString component;
        int pinCount = 0;
        int spacingMil = 0;

        QString moduleId;
        QString source;

        /*
         * Physical package pin number -> Fritzing connector ID.
         *
         * Example:
         *
         *   pin 1 -> connector0
         *   pin 2 -> connector1
         *
         * This mapping is validated against the selected ModelPart.
         * The placement/wiring code must consume this map rather
         * than manufacturing connector IDs itself.
         */
        QHash<int, QString> pinConnectorIds;
};

class BreadboardCsvComponentResolver
{
public:
        static BreadboardCsvComponentResolution resolveNative(
                ReferenceModel *referenceModel,
                const QString &component,
                int pinCount
        );

        static BreadboardCsvComponentResolution resolveGeneric(
                const QString &component,
                int pinCount,
                int spacingMil
        );
};

#endif
