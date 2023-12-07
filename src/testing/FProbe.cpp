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

#include"FProbe.h"
#include"FTesting.h"

FProbe::FProbe(std::string name) :
	m_name(name)
{
	std::shared_ptr<FTesting> fTesting = FTesting::getInstance();
	fTesting->addProbe(this);
}

FProbe::~FProbe() {
	std::shared_ptr<FTesting> fTesting = FTesting::getInstance();
	fTesting->removeProbe(m_name);
};

std::string FProbe::name() {
	return m_name;
}
