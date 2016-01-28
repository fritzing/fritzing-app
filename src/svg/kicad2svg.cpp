/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2016 Fritzing

Fritzing is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Fritzing is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Fritzing.  If not, see <http://www.gnu.org/licenses/>.

********************************************************************

$Revision: 6904 $:
$Author: irascibl@gmail.com $:
$Date: 2013-02-26 16:26:03 +0100 (Di, 26. Feb 2013) $

********************************************************************/

#include "kicad2svg.h"
#include "../utils/textutils.h"
#include "../version/version.h"

#include <QFileInfo>
#include <QDateTime>
#include <QTextDocument>

Kicad2Svg::Kicad2Svg() {
}

QString Kicad2Svg::makeMetadata(const QString & filename, const QString & type, const QString & name) 
{
	QFileInfo fileInfo(filename);

	QDateTime now = QDateTime::currentDateTime();
	QString dt = now.toString("dd/MM/yyyy hh:mm:ss");

	m_title = QString("<title>%1</title>\n").arg(fileInfo.fileName());
	m_description = QString("<desc>Kicad %3 '%2' from file '%1' converted by Fritzing</desc>\n")
			.arg(TextUtils::stripNonValidXMLCharacters(TextUtils::escapeAnd(fileInfo.fileName())))
			.arg(TextUtils::stripNonValidXMLCharacters(TextUtils::escapeAnd(name)))
			.arg(type);

	QString metadata("<metadata xmlns:fz='http://fritzing.org/kicadmetadata/1.0/' xmlns:rdf='http://www.w3.org/1999/02/22-rdf-syntax-ns#'>\n");
	metadata += "<rdf:RDF>";
	metadata += "<rdf:Description rdf:about=''>\n";
	metadata += m_attribute.arg("kicad filename").arg(fileInfo.fileName());
	metadata += m_attribute.arg(QString("kicad %1").arg(type))
							.arg(TextUtils::stripNonValidXMLCharacters(TextUtils::escapeAnd(name)));
	metadata += m_attribute.arg("fritzing version").arg(Version::versionString());
	metadata += m_attribute.arg("conversion date").arg(dt);
	metadata += m_attribute.arg("dist-license").arg("GPL");
	metadata += m_attribute.arg("use-license").arg("unlimited");
	metadata += m_attribute.arg("author").arg("KICAD project");
	metadata += m_attribute.arg("license-url").arg("http://www.gnu.org/licenses/gpl.html");

	return metadata;
}

QString Kicad2Svg::endMetadata() {
	QString metadata = "</rdf:Description>";
	metadata += "</rdf:RDF>";
	metadata += "</metadata>";
	return metadata;
}



