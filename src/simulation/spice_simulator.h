/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef SPICE_SIMULATOR_H
#define SPICE_SIMULATOR_H

#include <QtCore>

#include "sim_types.h"

#include <string>
#include <vector>
#include <complex>
#include <memory>


class SPICE_REPORTER;
class SPICE_SIMULATOR;

typedef std::complex<double> COMPLEX;

class SPICE_SIMULATOR
{
public:
    SPICE_SIMULATOR() : m_reporter( NULL ) {}
    virtual ~SPICE_SIMULATOR() {}

    ///> Creates a simulator instance of particular type (currently only ngspice is handled)
    static std::shared_ptr<SPICE_SIMULATOR> CreateInstance( const std::string& aName );

    ///> Initializes the simulator
    virtual void Init() = 0;

    /*
     * @brief Loads a netlist for the simulation.
     * @return True in case of success, false otherwise.
     */
    virtual bool LoadNetlist( const std::string& aNetlist ) = 0;

    /**
     * @brief Executes the simulation with currently loaded netlist.
     * @return True in case of success, false otherwise.
     */
    virtual bool Run() = 0;

    /**
     * @brief Halts the simulation.
     * @return True in case of success, false otherwise.
     */
    virtual bool Stop() = 0;

    /**
     * @brief Checks if simulation is running at the moment.
     * @return True if simulation is currently executed.
     */
    virtual bool IsRunning() = 0;

    /**
     * @brief Executes a Spice command as if it was typed into console.
     * @param aCmd is the command to be issued.
     */
    virtual bool Command( const std::string& aCmd ) = 0;

    ///> Returns X axis name for a given simulation type
    virtual std::string GetXAxis( SIM_TYPE aType ) const = 0;

    ///> Sets a SPICE_REPORTER object to receive the simulation log.
    virtual void SetReporter( SPICE_REPORTER* aReporter )
    {
        m_reporter = aReporter;
    }

    /**
     * @brief Returns a list with all vectors generated in current simulation.
     * @param none
     * @return List of vector names. ?May not match to the net name elements.
     */
    virtual std::vector<std::string> AllPlots() = 0;

    /**
     * @brief Returns a requested vector with complex values. If the vector is real, then
     * the imaginary part is set to 0 in all values.
     * @param aName is the vector named in Spice convention (e.g. V(3), I(R1)).
     * @param aMaxLen is max count of returned values.
     * if -1 (default) all available values are returned.
     * @return Requested vector. It might be empty if there is no vector with requested name.
     */
    virtual std::vector<COMPLEX> GetPlot( const std::string& aName, int aMaxLen = -1 ) = 0;

    /**
     * @brief Returns a requested vector with real values. If the vector is complex, then
     * the real part is returned.
     * @param aName is the vector named in Spice convention (e.g. V(3), I(R1)).
     * @param aMaxLen is max count of returned values.
     * if -1 (default) all available values are returned.
     * @return Requested vector. It might be empty if there is no vector with requested name.
     */
    virtual std::vector<double> GetRealPlot( const std::string& aName, int aMaxLen = -1 ) = 0;

    /**
     * @brief Returns a requested vector with imaginary values. If the vector is complex, then
     * the imaginary part is returned. If the vector is reql, then only zeroes are returned.
     * @param aName is the vector named in Spice convention (e.g. V(3), I(R1)).
     * @param aMaxLen is max count of returned values.
     * if -1 (default) all available values are returned.
     * @return Requested vector. It might be empty if there is no vector with requested name.
     */
    virtual std::vector<double> GetImagPlot( const std::string& aName, int aMaxLen = -1 ) = 0;

    /**
     * @brief Returns a requested vector with magnitude values.
     * @param aName is the vector named in Spice convention (e.g. V(3), I(R1)).
     * @param aMaxLen is max count of returned values.
     * if -1 (default) all available values are returned.
     * @return Requested vector. It might be empty if there is no vector with requested name.
     */
    virtual std::vector<double> GetMagPlot( const std::string& aName, int aMaxLen = -1 ) = 0;

	/**
	 * @brief Returns a requested data point with magnitude values.
	 * @param aName is the vector named in Spice convention (e.g. V(3), I(R1)).
	 * @return Requested a data pointvector. It might be empty if there is no vector with requested name.
	 */
	virtual double GetDataPoint( const std::string& aName) = 0;

    /**
     * @brief Returns a requested vector with phase values.
     * @param aName is the vector named in Spice convention (e.g. V(3), I(R1)).
     * @param aMaxLen is max count of returned values.
     * if -1 (default) all available values are returned.
     * @return Requested vector. It might be empty if there is no vector with requested name.
     */
    virtual std::vector<double> GetPhasePlot( const std::string& aName, int aMaxLen = -1 ) = 0;

    /**
     * @brief Returns current SPICE netlist used by the simulator.
     * @return The netlist.
     */
    virtual const std::string GetNetlist() const = 0;

    /**
     * @brief Returns a string with simulation name based on enum.
     * @param aType is the enum describing simulation type
     * @param aShortName if true  - return is in format "TRAN", "OP".
     *                   if false - return is in format "Transient", "Operating Point".
     * @return String with requested name as described above.
     */
	static QString TypeToName( SIM_TYPE aType, bool aShortName );

	virtual bool ErrorFound() = 0;

protected:
    ///> Reporter object to receive simulation log
    SPICE_REPORTER* m_reporter;
};

#endif /* SPICE_SIMULATOR_H */
