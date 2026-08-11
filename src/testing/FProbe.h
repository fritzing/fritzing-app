/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2022 Fritzing GmbH

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

#ifndef FPROBE_H
#define FPROBE_H

#include <QString>
#include <QVariant>

class FProbe {
	public:
		FProbe(std::string name);
		~FProbe();

	protected:
		friend class FTesting;  // Only FTesting may use the probes.
		virtual QVariant read() = 0;
		virtual void write(QVariant) = 0;
		// Optional verb for probes that take parameters and return a result.
		virtual QVariant call(QVariant params) { Q_UNUSED(params); return QVariant(); }
		virtual std::string name();

		std::string m_name;
};

#endif
