
#include <QApplication>
#include <QDir>
#include <QString>
#include <QFile>
#include <iostream>
#include <QtDebug>
#include <QDomDocument>
#include <QDomElement>
#include <QStringList>
#include <QMatrix>




QDir globalDir;

void usage() {
	std::cout << "move arduino pads:" << std::endl << std::endl;
	std::cout << "usage:  artreeno  infilename outfilename" << std::endl << std::endl;
    std::cout << std::endl;
}

void parsePath(const QString & path, double & x, double & y) {
	QString temp = path;
	temp.remove(0,1);
	QStringList strings = temp.split(",");
	if (strings.count() < 2) return;

	x = strings[0].toDouble();
	strings = strings[1].split("v");
	y = strings[0].toDouble();

}

void handleFile(const QString & infilename, double dx, double dy, const QString & outfilename) {
	std::cout << "\t" << infilename.toStdString() << std::endl;
	QFile file(infilename);

	QString errorStr;
	int errorLine;
	int errorColumn;
	QDomDocument domDocument;

	if (!domDocument.setContent(&file, true, &errorStr, &errorLine, &errorColumn)) {
		std::cout << "file isn't svg file (1)" << std::endl;
		return;
	}

	QDomElement root = domDocument.documentElement();
	if (root.isNull()) {
		std::cout << "file isn't svg file (2)" << std::endl;
		return;
	}

    if (root.tagName() != "svg") {
		std::cout << "can't find svg tag" << std::endl;
        return;
    }

	QString viewBoxStr = root.attribute("viewBox");
	QStringList strings = viewBoxStr.split(" ");
	double vbWidth = strings[2].toDouble();
	double vbHeight = strings[3].toDouble();
	double multiplier = 2.54;
	QString wStr = root.attribute("width");
	wStr.chop(2);
	double width = multiplier * wStr.toDouble() * 100;
	QString hStr = root.attribute("height");
	hStr.chop(2);
	double height = multiplier * hStr.toDouble() * 100;

	QDomElement g = root.firstChildElement("g");
	if (g.isNull()) {
		std::cout << "can't find g element" << std::endl;
        return;
	}

	QString id = g.attribute("id");
	if (id.compare("copper0") != 0) {
		std::cout << "can't copper0 attribute" << std::endl;
        return;
	}

	QHash<QString, QString> viewImages;

	QMatrix matrix;
	matrix.translate(width / 2, height / 2);
	matrix.rotate(-90);
	matrix.translate(-width / 2, -height / 2);

	bool firstPath = true;
	QDomElement path = g.firstChildElement("path");
	while (!path.isNull()) {
		double x = 0, y = 0;
		parsePath(path.attribute("d"), x, y);
		if (firstPath) {
			dx = x - dx;
			dy = y - dy;
			firstPath = false;
		}

		QDomElement circle = domDocument.createElement ( "circle");
		g.appendChild(circle);
		QPointF n((x - dx) * width / vbWidth, (y - dy) * height / vbHeight);
		QPointF r = matrix.map(n);
		circle.setAttribute("id", path.attribute("id"));
		circle.setAttribute("cx", r.x() + 400 - 83);		// should be 400 here, but not sure why the offset is necessary
		circle.setAttribute("cy", r.y() + 1260 - 692.15 + 160);	// should be 1260 - 692.15 (width of arduino sideways) 
		circle.setAttribute("r", QString::number(2.7 * width / vbWidth));
		circle.setAttribute("fill", "none");
		circle.setAttribute("stroke", "#FFBF00");
		circle.setAttribute("stroke-width", QString::number(2.2 * width / vbWidth));

		path = path.nextSiblingElement("path");
	}

	path = g.firstChildElement("path");
	while (!path.isNull()) {
		g.removeChild(path);
		path = g.firstChildElement("path");
	}

	QFile data(outfilename);
	if (data.open(QFile::WriteOnly | QFile::Truncate)) {
		QTextStream out(&data);
		out << domDocument.toString();
		data.close();
	}
}



int main(int argc, char * argv[])
{
    if ((argc < 3)) {
        usage();
        return 0;
    }

	handleFile(argv[1], 86.064, 10.764, argv[2]);

}

