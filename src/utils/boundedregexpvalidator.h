/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2019 Fritzing

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

********************************************************************/

#ifndef BOUNDEDREGEXPVALIDATOR_H
#define BOUNDEDREGEXPVALIDATOR_H

#include <QRegExpValidator>
#include <QColor>
#include <QLineEdit>
#include <limits>
#include "focusoutcombobox.h"
#include "../utils/textutils.h"

typedef double (*Converter)(const QString &, const QString & symbol);

class BoundedRegExpValidator : public QRegExpValidator
{
	Q_OBJECT

signals:
	void sendState(QValidator::State) const;

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

	QValidator::State validate ( QString & input, int & pos ) const override {
		QValidator::State state = QRegExpValidator::validate(input, pos);
		input.replace(m_symbol, "");
		if(!m_symbol.isEmpty())
			input.append(m_symbol);
		state = QRegExpValidator::validate(input, pos);

		if ( state == QValidator::Acceptable ) {
			double converted = m_converter(input, m_symbol);
			if (converted < m_min) state = QValidator::Intermediate;
			if (converted > m_max) state = QValidator::Intermediate;
		}
		emit sendState(state);
		return state;
	}

	virtual void fixup ( QString& input) const override {
		if (m_converter == NULL) {
			if (!input.endsWith(m_symbol)) input.append(m_symbol);
			return;
		}
		double divider = 10;
		if (m_max - m_min > 1000)
			divider = 1000;

		double converted = m_converter(input, m_symbol);
		for (int i = 0; i < 10; i++) {
			if (converted > m_max) {
				converted /= divider;
			} else if (converted < m_min) {
				converted *= divider;
			} else {
				break;
			}
		}
		input = TextUtils::convertToPowerPrefix(converted);
		input.append(m_symbol);
	}

protected:
	double m_min;
	double m_max;
	QString m_symbol;
	Converter m_converter;
};

#endif
