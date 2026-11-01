#include "platformarduino.h"
#include "programtab.h"

#include <QDir>
#include <QFileInfo>
#if QT_CONFIG(process)
#include <QProcess>
#endif
#ifdef Q_OS_WASM
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>
#include "../utils/folderutils.h"
#include "webserialport_wasm.h"
#endif

PlatformArduino::PlatformArduino() : Platform(QString("Arduino"))
{
	setReferenceUrl(QUrl(QString("https://www.arduino.cc/reference/tr/")));
	setIdeName("Arduino CLI");
	setDownloadUrl(QUrl("https://github.com/arduino/arduino-cli"));
	setMinVersion("1.0.0");
	setCanProgram(true);
	setExtensions(QStringList() << ".ino" << ".pde");

	QMap<QString, QString> boards;
	// https://github.com/arduino/Arduino/blob/master/hardware/arduino/avr/boards.txt
	boards.insert("Arduino UNO", "arduino:avr:uno");
	boards.insert("Arduino Yún", "arduino:avr:yun");
	boards.insert("Arduino Mega/2560", "arduino:avr:mega");
	boards.insert("Arduino Duemilanove/Diecemila", "arduino:avr:diecimila");
	boards.insert("Arduino Nano", "arduino:avr:nano");
	boards.insert("Arduino Mega ADK", "arduino:avr:megaADK");
	boards.insert("Arduino Leonardo", "arduino:avr:leonardo");
	boards.insert("Arduino Micro", "arduino:avr:micro");
	boards.insert("Arduino Esplora", "arduino:avr:Esplora");
	boards.insert("Arduino Mini", "arduino:avr:mini");
	boards.insert("Arduino Ethernet", "arduino:avr:ethernet");
	boards.insert("Arduino Fio", "arduino:avr:fio");
	boards.insert("Arduino BT", "arduino:avr:bt");
	boards.insert("Lilypad Arduino USB", "arduino:avr:LilyPadUSB");
	boards.insert("LilyPad Arduino ", "arduino:avr:lilypad");
	boards.insert("Arduino Pro/Pro Mini", "arduino:avr:pro");
	boards.insert("Arduino NG or older", "arduino:avr:atmegang");
	boards.insert("Arduino Robot Control", "arduino:avr:robotControl");
	boards.insert("Arduino Robot Motor", "arduino:avr:robotMotor");
	boards.insert("Arduino Gemma", "arduino:avr:gemma");
	// https://github.com/arduino/Arduino/blob/master/hardware/arduino/sam/boards.txt
	boards.insert("Arduino Due (Programming Port)", "arduino:sam:arduino_due_x_dbg");
	boards.insert("Arduino Due (Native USB Port)", "arduino:sam:arduino_due_x");
	// Note: Edison, Galileo, 101 are programmed via Arduino IDE Add-ons,
	//       not sure if the command line is supported for them
	setBoards(boards);

	setDefaultBoardName("Arduino UNO");
}

void PlatformArduino::upload(QWidget *source, const QString &port, const QString &board, const QString &fileLocation)
{
#ifdef Q_OS_WASM
	// The browser cannot run arduino-cli: the Fritzing server compiles the sketch
	// (docker/wasm/compile-service) and the browser uploads it over Web Serial
	auto *tab = qobject_cast<ProgramTab *>(source);
	if (tab == nullptr) return;

	QFile file(fileLocation);
	if (!file.open(QIODevice::ReadOnly)) {
		tab->uploadFinished(false, tr("Unable to read %1").arg(fileLocation));
		return;
	}
	QJsonObject body;
	body["fqbn"] = getBoards().value(board);
	body["sketch"] = QString::fromUtf8(file.readAll());
	file.close();

	tab->appendToConsole(tr("Compiling for %1 on the Fritzing server...").arg(board));
	QNetworkRequest request{QUrl(WebSerial::compileServiceUrl())};
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
	auto * manager = new QNetworkAccessManager(this);
	QNetworkReply * reply = manager->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
	QPointer<ProgramTab> tabPointer(tab);
	QString sketchName = QFileInfo(fileLocation).completeBaseName();
	connect(reply, &QNetworkReply::finished, this, [this, reply, manager, tabPointer, port, board, sketchName]() {
		reply->deleteLater();
		manager->deleteLater();
		if (!tabPointer) return;

		int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
		QJsonObject result = QJsonDocument::fromJson(reply->readAll()).object();
		if (result.isEmpty()) {
			tabPointer->uploadFinished(false, (status == 502 || status == 504 || status == 404)
			                           ? tr("The compile service is not available. Start the Fritzing web app with "
			                                "docker/wasm/compose.yaml to upload code.")
			                           : tr("Compiling failed: %1").arg(reply->errorString()));
			return;
		}
		QString log = result["log"].toString().trimmed();
		if (!log.isEmpty()) tabPointer->appendToConsole(log);
		if (!result["ok"].toBool()) {
			tabPointer->uploadFinished(false, tr("Compiling failed."));
			return;
		}

		QString hex = result["hex"].toString();
		if (!WebSerial::canFlash(board)) {
			QString path = FolderUtils::getSaveFileName(nullptr, QString(), sketchName + ".hex");
			QFile hexFile(path);
			if (hexFile.open(QIODevice::WriteOnly)) {
				hexFile.write(hex.toLatin1());
				hexFile.close();
				FolderUtils::offerDownload(path);
			}
			tabPointer->uploadFinished(true, tr("The browser cannot upload to %1, so the compiled sketch was downloaded as %2. "
			                                    "Upload it with a programming tool.").arg(board, QFileInfo(path).fileName()));
			return;
		}
		if (port.isEmpty() || port == WebSerial::ChoosePortLocation) {
			tabPointer->uploadFinished(false, tr("Compiled. Choose a port to upload to the board."));
			return;
		}

		tabPointer->appendToConsole(tr("Uploading to %1 on %2...").arg(board, port));
		WebSerial::flash(tabPointer, port, board, hex,
		                 [tabPointer](const QString & message) {
		                     if (tabPointer) tabPointer->appendToConsole(message);
		                 },
		                 [tabPointer](bool ok, const QString & message) {
		                     if (!tabPointer) return;
		                     tabPointer->uploadFinished(ok, ok ? tr("Upload finished.") : tr("Upload failed: %1").arg(message));
		                 });
	});
#elif !QT_CONFIG(process)
	Platform::upload(source, port, board, fileLocation);
#else
	auto * process = new QProcess(this);
	process->setProcessChannelMode(QProcess::MergedChannels);
	process->setReadChannel(QProcess::StandardOutput);
	connect(process, SIGNAL(finished(int, QProcess::ExitStatus)), source, SLOT(programProcessFinished(int, QProcess::ExitStatus)));
	connect(process, SIGNAL(readyReadStandardOutput()), source, SLOT(programProcessReadyRead()));

	// Make sure .ino is in its own folder with same name (as required by Arduino compiler),
	// otherwise create a subfolder and copy the file there.
	QFileInfo fileInfo(fileLocation);
	QString tmpFilePath = fileInfo.absoluteFilePath();
	QString dirName = fileInfo.dir().dirName();
	QString sketchName = fileInfo.baseName();
	if (dirName.compare(sketchName, Qt::CaseInsensitive) != 0) {
		QString tmpSketchName(sketchName.append("_TMP"));
		fileInfo.dir().mkdir(tmpSketchName);
		tmpFilePath = fileInfo.absolutePath().append("/").append(tmpSketchName).append("/")
		              .append(fileInfo.baseName().append("_TMP.").append(fileInfo.suffix()));
		if (QFile::exists(tmpFilePath)) QFile::remove(tmpFilePath);
		QFile::copy(fileInfo.absoluteFilePath(), tmpFilePath);
	}

	QStringList args;
	if (!usingArduinoCLI()) {
		// see https://github.com/arduino/Arduino/blob/ide-1.5.x/build/shared/manpage.adoc
		//args.append(QString("--verbose"));
		args.append(QString("--board"));
	}else {
		// see https://github.com/arduino/arduino-cli
		//args.append(QString("--verbose"));
		args.append(QString("compile"));
		args.append(QString("-b"));
	}
	args.append(getBoards().value(board));
	args.append(QString("--port"));
	args.append(port);
	args.append(QString("--upload"));
	args.append(QDir::toNativeSeparators(tmpFilePath));

	auto *tab = qobject_cast<ProgramTab *>(source);
	if (tab != nullptr)
		tab->appendToConsole(tr("Running %1 %2").arg(getCommandLocation()).arg(args.join(" ")));
	process->start(getCommandLocation(), args);

#endif
}

#if QT_CONFIG(process)
bool PlatformArduino::usingArduinoCLI() {
	//We make an error and analize if we are using the arduino-cli or not
	auto * process = new QProcess(this);
	QStringList args;
	args.append(QString("--board"));
	args.append(QString(" sssssss"));

	process->start(getCommandLocation(), args);
	process->waitForFinished(500);
	QString version = process->readAllStandardError();

	return version.contains("arduino-cli");
}
#endif
