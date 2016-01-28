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

#ifndef BOUNDEDREGEXPVALIDATOR_H
#define BOUNDEDREGEXPVALIDATOR_H

#include <QRegExpValidator>
#include <limits>

typedef double (*Converter)(const QString &, const QString & symbol);

class BoundedRegExpValidator : public QRegExpValidator 
{
public:
	BoundedRegExpValidator(QObject * parent) : QRegExpValidator(parent) {
		m_max = std::numeric_limits<double>::max();
		m_min = std::numeric_limits<double>::min();
		m_converter = NULL;
		m_symbol = "";
	}

	void setBounds(double min, double max) {
		m_min = min;
		m_max = max;
	}

	void setConverter(Converter converter) {
		m_converter = converter;
	}

	void setSymbol(const QString & symbol) {
		m_symbol = symbol;
	}

	QValidator::State validate ( QString & input, int & pos ) const {
		QValidator::State state = QRegExpValidator::validate(input, pos);
		if (state == QValidator::Invalid) return state;
		if (state == QValidator::Intermediate) return state;
		if (m_converter == NULL) return state;

		double converted = m_converter(input, m_symbol);
		if (converted < m_min) return QValidator::Invalid;
		if (converted > m_max) return QValidator::Invalid;

		return QValidator::Acceptable;
	}

protected:
	double m_min;
	double m_max;
	QString m_symbol;
	Converter m_converter;
};

#endif
