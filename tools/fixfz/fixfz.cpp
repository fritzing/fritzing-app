#include "fixfz.h"
#include <QApplication>
#include <QDir>
#include <QString>
#include <QFile>
#include <iostream>
#include <QtDebug>
#include <QDomDocument>
#include <QDomElement>


QDir globalDir;

void usage() {
	std::cout << "update fz files:" << std::endl << std::endl;
	std::cout << "usage:  fixfz  partsfolder" << std::endl << std::endl;
    std::cout << std::endl;
}


bool findFile(QDir & dir, QFile & resultFile, QString filename) {
	if (!dir.exists()) return false;

	QFileInfoList files = dir.entryInfoList();
	foreach(QFileInfo fileInfo, files) {
		if (fileInfo.isFile()) {
			if (fileInfo.absoluteFilePath().endsWith(filename)) {
				resultFile.setFileName(fileInfo.absoluteFilePath());
				return true;
			}
			continue;
		}

		if (fileInfo.fileName().compare(".") == 0) continue;
		if (fileInfo.fileName().compare("..") == 0) continue;

		if (fileInfo.isDir()) {
			QDir temp(fileInfo.absoluteFilePath());
			if (findFile(temp, resultFile, filename)) {
				return true;
			}
		}
	}

	return false;
}

void handleFile(const QString & filename) {
	std::cout << "\t" << filename.toStdString() << std::endl;
	QFile file(filename);

	QString errorStr;
	int errorLine;
	int errorColumn;
	QDomDocument domDocument;

	if (!domDocument.setContent(&file, true, &errorStr, &errorLine, &errorColumn)) {
		std::cout << "file isn't fritzing file (1)" << std::endl;
		return;
	}

	QDomElement root = domDocument.documentElement();
	if (root.isNull()) {
		std::cout << "file isn't fritzing file (2)" << std::endl;
		return;
	}

    if (root.tagName() != "module") {
		std::cout << "can't find module tag" << std::endl;
        return;
    }


	QDomElement views = root.firstChildElement("views");
	if (views.isNull()) {
		std::cout << "can't find views element" << std::endl;
        return;
	}

	QHash<QString, QString> viewImages;

	QDomElement view = views.firstChildElement();
	while (!view.isNull()) {
		QDomElement layers = view.firstChildElement("layers");
		if (!layers.isNull()) {
			QDomElement layer = layers.firstChildElement("layer");
			while (!layer.isNull()) {
				QString image = layer.attribute("image");
				if (!image.isEmpty()) {
					layers.setAttribute("image", image);
					layer.removeAttribute("image");
				}

				layer = layer.nextSiblingElement("layer");
			}
		}

		viewImages.insert(view.tagName(), layers.attribute("image"));

		view = view.nextSiblingElement();
	}

	QDomElement connectors = root.firstChildElement("connectors");
	if (views.isNull()) {
		std::cout << "can't find connectors element" << std::endl;
        return;
	}

	QDomElement connector = connectors.firstChildElement("connector");
	while (!connector.isNull()) {
		QDomElement views = connector.firstChildElement("views");
		if (views.isNull()) {
			std::cout << "can't find views element" << std::endl;
		}

		QDomElement view = views.firstChildElement();
		while (!view.isNull()) {
			QDomElement pin = view.firstChildElement("pin");
			if (!pin.isNull()) {
				pin.setTagName("p");
				QString id = pin.attribute("svgId");
				QString image = viewImages.value(view.tagName());	
				QFile imageFile;
				if (findFile(globalDir, imageFile, image)) {
					if (imageFile.open(QIODevice::ReadOnly)) {
						QString temp = imageFile.readAll();
						imageFile.close();
						if (!temp.contains(id)) {
							std::cout << "--- missing " << id.toStdString() << " in " << view.tagName().toStdString() << " in " << imageFile.fileName().toStdString() << " ---" << std::endl;
						}
					}
				}
			}

			view = view.nextSiblingElement();
		}

		connector = connector.nextSiblingElement("connector");
	}

	QFile data(filename);
	if (data.open(QFile::WriteOnly | QFile::Truncate)) {
		QTextStream out(&data);
		out << domDocument.toString();
		data.close();
	}
}

void runDir(QDir & dir) {
	if (!dir.exists()) return;

	QFileInfoList files = dir.entryInfoList();
	foreach(QFileInfo file, files) {
		if (file.isFile()) {
			if (file.fileName().endsWith(".fz")) {
				handleFile(file.absoluteFilePath());
			}
			continue;
		}

		if (file.fileName().compare(".") == 0) continue;
		if (file.fileName().compare("..") == 0) continue;

		if (file.isDir()) {
			QDir temp(file.absoluteFilePath());
			runDir(temp);
		}
	}
}

int main(int argc, char * argv[])
{
    if ((argc < 2)) {
        usage();
        return 0;
    }

	globalDir.setPath(argv[1]);
	runDir(globalDir);
}

