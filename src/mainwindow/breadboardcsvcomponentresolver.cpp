#include "breadboardcsvcomponentresolver.h"

#include "../model/modelpart.h"
#include "../referencemodel/referencemodel.h"
#include "../viewlayer.h"

#include <QList>
#include <QRegularExpression>
#include <QStringList>

namespace {

bool exactIdentityMatch(
        ModelPart *modelPart,
        const QString &component
)
{
        if (modelPart == nullptr) {
                return false;
        }

        const QString wanted =
                component.trimmed();

        if (wanted.isEmpty()) {
                return false;
        }

        if (
                modelPart->title().trimmed().compare(
                        wanted,
                        Qt::CaseInsensitive
                ) == 0
        ) {
                return true;
        }

        for (const QString &tag : modelPart->tags()) {
                if (
                        tag.trimmed().compare(
                                wanted,
                                Qt::CaseInsensitive
                        ) == 0
                ) {
                        return true;
                }
        }

        const QHash<QString, QString> &properties =
                modelPart->properties();

        static const QStringList identityPropertyNames = {
                "part number",
                "chip"
        };

        for (const QString &propertyName : identityPropertyNames) {
                const QString value =
                        properties.value(
                                propertyName
                        ).trimmed();

                if (
                        !value.isEmpty() &&
                        value.compare(
                                wanted,
                                Qt::CaseInsensitive
                        ) == 0
                ) {
                        return true;
                }
        }

        return false;
}

bool parseDipBreadboardImage(
        ModelPart *modelPart,
        int &pins,
        int &spacingMil
)
{
        pins = 0;
        spacingMil = 0;

        if (modelPart == nullptr) {
                return false;
        }

        const QString image =
                modelPart->imageFileName(
                        ViewLayer::BreadboardView
                );

        /*
         * Existing Fritzing DIP images include forms such as:
         *
         *   breadboard/generic_ic_dip_8_300mil_bread.svg
         *   breadboard/generic_ic_dip_v2_40_600mil.svg
         *
         * We deliberately require the geometry to be encoded in
         * the breadboard image name before trusting the part for
         * automatic physical placement.
         */
        static const QRegularExpression pattern(
                R"((?:^|/)generic_ic_dip(?:_v2)?_([1-9][0-9]*)_([1-9][0-9]*)mil(?:_[^/]*)?\.svg$)",
                QRegularExpression::CaseInsensitiveOption
        );

        const QRegularExpressionMatch match =
                pattern.match(image);

        if (!match.hasMatch()) {
                return false;
        }

        bool pinsOk = false;
        bool spacingOk = false;

        const int parsedPins =
                match.captured(1).toInt(
                        &pinsOk
                );

        const int parsedSpacing =
                match.captured(2).toInt(
                        &spacingOk
                );

        if (
                !pinsOk ||
                !spacingOk ||
                parsedPins < 2 ||
                (parsedPins % 2) != 0
        ) {
                return false;
        }

        pins = parsedPins;
        spacingMil = parsedSpacing;

        return true;
}

bool buildSequentialPinMap(
        ModelPart *modelPart,
        int pinCount,
        QHash<int, QString> &pinConnectorIds
)
{
        pinConnectorIds.clear();

        if (
                modelPart == nullptr ||
                pinCount < 2
        ) {
                return false;
        }

        const auto &connectors =
                modelPart->connectors();

        if (connectors.size() != pinCount) {
                return false;
        }

        /*
         * Do not merely assume connectorN exists.
         *
         * Validate every physical package pin against the actual
         * ModelPart connector hash. Parts that use some other
         * connector convention are not yet safe for automatic CSV
         * wiring and are rejected here.
         */
        for (int pin = 1; pin <= pinCount; ++pin) {
                const QString connectorId =
                        QString("connector%1")
                                .arg(pin - 1);

                if (!connectors.contains(connectorId)) {
                        pinConnectorIds.clear();
                        return false;
                }

                pinConnectorIds.insert(
                        pin,
                        connectorId
                );
        }

        return true;
}

struct NativeCandidate
{
        ModelPart *modelPart = nullptr;
        int spacingMil = 0;
        QHash<int, QString> pinConnectorIds;
};

} // namespace

BreadboardCsvComponentResolution
BreadboardCsvComponentResolver::resolveGeneric(
        const QString &component,
        int pinCount,
        int spacingMil
)
{
        BreadboardCsvComponentResolution result;

        result.component =
                component.trimmed();

        result.pinCount =
                pinCount;

        if (result.component.isEmpty()) {
                result.error =
                        "Component name is empty.";

                return result;
        }

        if (
                pinCount < 2 ||
                (pinCount % 2) != 0
        ) {
                result.error =
                        QString(
                                "%1 has invalid DIP pin count %2."
                        )
                                .arg(result.component)
                                .arg(pinCount);

                return result;
        }

        if (
                spacingMil != 300 &&
                spacingMil != 600
        ) {
                result.error =
                        QString(
                                "%1 has unsupported DIP spacing %2 mil."
                        )
                                .arg(result.component)
                                .arg(spacingMil);

                return result;
        }

        result.matched = true;
        result.nativePart = false;

        result.spacingMil =
                spacingMil;

        result.moduleId =
                QString(
                        "generic_ic_dip_v2_%1_%2mil"
                )
                        .arg(pinCount)
                        .arg(spacingMil);

        result.source =
                "Fritzing generated generic DIP";

        for (int pin = 1; pin <= pinCount; ++pin) {
                result.pinConnectorIds.insert(
                        pin,
                        QString("connector%1")
                                .arg(pin - 1)
                );
        }

        return result;
}


BreadboardCsvComponentResolution
BreadboardCsvComponentResolver::resolveNative(
        ReferenceModel *referenceModel,
        const QString &component,
        int pinCount
)
{
        BreadboardCsvComponentResolution result;

        result.component =
                component.trimmed();

        result.pinCount =
                pinCount;

        if (referenceModel == nullptr) {
                result.error =
                        "Fritzing reference model is not available.";

                return result;
        }

        if (result.component.isEmpty()) {
                result.error =
                        "Component name is empty.";

                return result;
        }

        if (
                pinCount < 2 ||
                (pinCount % 2) != 0
        ) {
                result.error =
                        QString(
                                "%1 has invalid inferred DIP pin count %2."
                        )
                                .arg(result.component)
                                .arg(pinCount);

                return result;
        }

        const QList<ModelPart *> matches =
                referenceModel->search(
                        result.component,
                        false
                );

        QList<NativeCandidate> candidates;
        QStringList rejectedExactParts;

        for (ModelPart *modelPart : matches) {
                if (
                        modelPart == nullptr ||
                        !exactIdentityMatch(
                                modelPart,
                                result.component
                        )
                ) {
                        continue;
                }

                int imagePins = 0;
                int imageSpacingMil = 0;

                if (
                        !parseDipBreadboardImage(
                                modelPart,
                                imagePins,
                                imageSpacingMil
                        )
                ) {
                        rejectedExactParts.append(
                                QString(
                                        "%1 [%2]: breadboard image does not "
                                        "identify a supported DIP geometry"
                                )
                                        .arg(
                                                modelPart->title(),
                                                modelPart->moduleID()
                                        )
                        );

                        continue;
                }

                if (imagePins != pinCount) {
                        rejectedExactParts.append(
                                QString(
                                        "%1 [%2]: image says %3 pins, "
                                        "CSV geometry says %4"
                                )
                                        .arg(
                                                modelPart->title(),
                                                modelPart->moduleID()
                                        )
                                        .arg(imagePins)
                                        .arg(pinCount)
                        );

                        continue;
                }

                if (
                        imageSpacingMil != 300 &&
                        imageSpacingMil != 600
                ) {
                        rejectedExactParts.append(
                                QString(
                                        "%1 [%2]: unsupported DIP spacing "
                                        "%3 mil"
                                )
                                        .arg(
                                                modelPart->title(),
                                                modelPart->moduleID()
                                        )
                                        .arg(imageSpacingMil)
                        );

                        continue;
                }

                QHash<int, QString> pinConnectorIds;

                if (
                        !buildSequentialPinMap(
                                modelPart,
                                pinCount,
                                pinConnectorIds
                        )
                ) {
                        rejectedExactParts.append(
                                QString(
                                        "%1 [%2]: connector IDs are not a "
                                        "complete connector0..connector%3 set"
                                )
                                        .arg(
                                                modelPart->title(),
                                                modelPart->moduleID()
                                        )
                                        .arg(pinCount - 1)
                        );

                        continue;
                }

                NativeCandidate candidate;

                candidate.modelPart =
                        modelPart;

                candidate.spacingMil =
                        imageSpacingMil;

                candidate.pinConnectorIds =
                        pinConnectorIds;

                candidates.append(
                        candidate
                );
        }

        if (candidates.size() > 1) {
                QStringList descriptions;

                for (const NativeCandidate &candidate : candidates) {
                        descriptions.append(
                                QString("%1 [%2]")
                                        .arg(
                                                candidate.modelPart->title(),
                                                candidate.modelPart->moduleID()
                                        )
                        );
                }

                result.error =
                        QString(
                                "%1 matched multiple compatible native "
                                "Fritzing DIP parts. The importer will not "
                                "guess:\n%2"
                        )
                                .arg(result.component)
                                .arg(descriptions.join("\n"));

                return result;
        }

        if (candidates.isEmpty()) {
                if (!rejectedExactParts.isEmpty()) {
                        result.error =
                                QString(
                                        "Fritzing contains exact-name matches "
                                        "for %1, but none are safe for automatic "
                                        "breadboard placement:\n%2"
                                )
                                        .arg(result.component)
                                        .arg(
                                                rejectedExactParts.join("\n")
                                        );
                }

                /*
                 * No error is required when the local Fritzing
                 * library simply does not know this component.
                 * Later providers can continue resolution.
                 */
                return result;
        }

        const NativeCandidate &candidate =
                candidates.first();

        result.matched = true;
        result.nativePart = true;

        result.spacingMil =
                candidate.spacingMil;

        result.moduleId =
                candidate.modelPart->moduleID();

        result.source =
                QString(
                        "Fritzing native part: %1"
                )
                        .arg(
                                candidate.modelPart->title()
                        );

        result.pinConnectorIds =
                candidate.pinConnectorIds;

        return result;
}
