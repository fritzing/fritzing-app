/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016-2018 CERN
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

//#include <config.h>     // Needed for MSW compilation

#include "ngspice.h"
//#include "spice_reporter.h"

//#include <common.h>
#include <iostream>
#include <stdexcept>

#include <algorithm>

using namespace std;

//static const wxChar* const traceNgspice = wxT( "KICAD_NGSPICE" );

//FIXME:
#define NGSPICE_DLL_FILE ""
#define __WINDOWS__

NGSPICE::NGSPICE()
        : m_ngSpice_Init( nullptr ),
          m_ngSpice_Circ( nullptr ),
          m_ngSpice_Command( nullptr ),
          m_ngGet_Vec_Info( nullptr ),
          m_ngSpice_CurPlot( nullptr ),
          m_ngSpice_AllPlots( nullptr ),
          m_ngSpice_AllVecs( nullptr ),
          m_ngSpice_Running( nullptr ),
          m_error( false )
{
    init_dll();
	m_bgThreadIsRunning = true;
}


NGSPICE::~NGSPICE()
{
}


void NGSPICE::Init()
{
    Command( "reset" );
}


vector<string> NGSPICE::AllPlots()
{
	LOCALE_IO c_locale; // ngspice works correctly only with C locale
    char*     currentPlot = m_ngSpice_CurPlot();
    char**    allPlots    = m_ngSpice_AllVecs( currentPlot );
    int       noOfPlots   = 0;

    if( allPlots != nullptr )
        for( char** plot = allPlots; *plot != nullptr; plot++ )
            noOfPlots++;

    vector<string> retVal( noOfPlots );
    for( int i = 0; i < noOfPlots; i++, allPlots++ )
    {
        string vec = *allPlots;
        retVal.at( i )  = vec;
    }

    return retVal;
}


vector<COMPLEX> NGSPICE::GetPlot( const string& aName, int aMaxLen )
{
	LOCALE_IO c_locale;       // ngspice works correctly only with C locale
    vector<COMPLEX> data;
    vector_info* vi = m_ngGet_Vec_Info( (char*) aName.c_str() );

    if( vi )
    {
        int length = aMaxLen < 0 ? vi->v_length : std::min( aMaxLen, vi->v_length );
        data.reserve( length );

        if( vi->v_realdata )
        {
            for( int i = 0; i < length; i++ )
                data.emplace_back( vi->v_realdata[i], 0.0 );
        }
        else if( vi->v_compdata )
        {
            for( int i = 0; i < length; i++ )
                data.emplace_back( vi->v_compdata[i].cx_real, vi->v_compdata[i].cx_imag );
        }
    }

    return data;
}


vector<double> NGSPICE::GetRealPlot( const string& aName, int aMaxLen )
{
	LOCALE_IO c_locale;       // ngspice works correctly only with C locale
    vector<double> data;
    vector_info* vi = m_ngGet_Vec_Info( (char*) aName.c_str() );

    if( vi )
    {
        int length = aMaxLen < 0 ? vi->v_length : std::min( aMaxLen, vi->v_length );
        data.reserve( length );

        if( vi->v_realdata )
        {
            for( int i = 0; i < length; i++ )
            {
                data.push_back( vi->v_realdata[i] );
            }
        }
        else if( vi->v_compdata )
        {
            for( int i = 0; i < length; i++ )
            {
                assert( vi->v_compdata[i].cx_imag == 0.0 );
                data.push_back( vi->v_compdata[i].cx_real );
            }
        }
    }

    return data;
}


vector<double> NGSPICE::GetImagPlot( const string& aName, int aMaxLen )
{
	LOCALE_IO c_locale;       // ngspice works correctly only with C locale
    vector<double> data;
    vector_info* vi = m_ngGet_Vec_Info( (char*) aName.c_str() );

    if( vi )
    {
        int length = aMaxLen < 0 ? vi->v_length : std::min( aMaxLen, vi->v_length );
        data.reserve( length );

        if( vi->v_compdata )
        {
            for( int i = 0; i < length; i++ )
            {
                data.push_back( vi->v_compdata[i].cx_imag );
            }
        }
    }

    return data;
}


vector<double> NGSPICE::GetMagPlot( const string& aName, int aMaxLen )
{
	LOCALE_IO c_locale;       // ngspice works correctly only with C locale
    vector<double> data;
    vector_info* vi = m_ngGet_Vec_Info( (char*) aName.c_str() );

    if( vi )
    {
        int length = aMaxLen < 0 ? vi->v_length : std::min( aMaxLen, vi->v_length );
        data.reserve( length );

        if( vi->v_realdata )
        {
            for( int i = 0; i < length; i++ )
                data.push_back( vi->v_realdata[i] );
        }
        else if( vi->v_compdata )
        {
            for( int i = 0; i < length; i++ )
                data.push_back( hypot( vi->v_compdata[i].cx_real, vi->v_compdata[i].cx_imag ) );
        }
    }

    return data;
}

double NGSPICE::GetDataPoint( const string& aName)
{
	LOCALE_IO c_locale;       // ngspice works correctly only with C locale
	double data;
	vector_info* vi = m_ngGet_Vec_Info( (char*) aName.c_str() );

	if( vi )
	{

		if( vi->v_realdata )
		{
			return  vi->v_realdata[0];
		}
		else if( vi->v_compdata )
		{
			return hypot( vi->v_compdata[0].cx_real, vi->v_compdata[0].cx_imag );
		}
	}

	return data;
}


vector<double> NGSPICE::GetPhasePlot( const string& aName, int aMaxLen )
{
	LOCALE_IO c_locale;       // ngspice works correctly only with C locale
    vector<double> data;
    vector_info* vi = m_ngGet_Vec_Info( (char*) aName.c_str() );

    if( vi )
    {
        int length = aMaxLen < 0 ? vi->v_length : std::min( aMaxLen, vi->v_length );
        data.reserve( length );

        if( vi->v_realdata )
        {
            for( int i = 0; i < length; i++ )
                data.push_back( 0.0 );      // well, that's life
        }
        else if( vi->v_compdata )
        {
            for( int i = 0; i < length; i++ )
                data.push_back( atan2( vi->v_compdata[i].cx_imag, vi->v_compdata[i].cx_real ) );
        }
    }

    return data;
}


bool NGSPICE::LoadNetlist( const string& aNetlist )
{
	LOCALE_IO c_locale;       // ngspice works correctly only with C locale
    vector<char*> lines;
    stringstream ss( aNetlist );

    m_netlist = "";

    while( !ss.eof() )
    {
        char line[1024];
        ss.getline( line, sizeof( line ) );
		lines.push_back( _strdup( line ) );
        m_netlist += std::string( line ) + std::string( "\n" );
    }
    lines.push_back( nullptr ); // sentinel, as requested in ngSpice_Circ description
	std::cout << "NETLIST_NGSPICE: " <<lines.size()<<std::endl;
	int result = m_ngSpice_Circ( lines.data() );
	std::cout << "Result of ngSpice_Circ is : " << result <<std::endl;

    for( auto line : lines )
        free( line );

    return true;
}


bool NGSPICE::Run()
{
	LOCALE_IO c_locale;               // ngspice works correctly only with C locale
    return Command( "bg_run" );     // bg_* commands execute in a separate thread
}


bool NGSPICE::Stop()
{
	LOCALE_IO c_locale;               // ngspice works correctly only with C locale
    return Command( "bg_halt" );    // bg_* commands execute in a separate thread
}


bool NGSPICE::IsRunning()
{
	return m_bgThreadIsRunning;
}


bool NGSPICE::Command( const string& aCmd )
{
	LOCALE_IO c_locale;               // ngspice works correctly only with C locale
    validate();
    m_ngSpice_Command( (char*) aCmd.c_str() );
    return true;
}


string NGSPICE::GetXAxis( SIM_TYPE aType ) const
{
    switch( aType )
    {
        case ST_AC:
        case ST_NOISE:
            return string( "frequency" );

        case ST_DC:
            return string( "v-sweep" );

        case ST_TRANSIENT:
            return string( "time" );

        default:
            break;
    }

    return string( "" );
}


void NGSPICE::init_dll()
{
	if( m_initialized ){
        return;
	}

	LOCALE_IO c_locale;               // ngspice works correctly only with C locale
	//const wxStandardPaths& stdPaths = wxStandardPaths::Get();

	if( m_dll.isLoaded() )      // enable force reload
		m_dll.unload();

// Extra effort to find libngspice
//    wxFileName dllFile( "", NGSPICE_DLL_FILE );
#if defined(__WINDOWS__)
    const vector<string> dllPaths = { "", "/mingw64/bin", "/mingw32/bin" };
#elif defined(__WXMAC__)
    const vector<string> dllPaths = {
        GetOSXKicadUserDataDir().ToStdString() + "/PlugIns/ngspice",
        GetOSXKicadMachineDataDir().ToStdString() + "/PlugIns/ngspice",
        // when running kicad.app
        stdPaths.GetPluginsDir().ToStdString() + "/sim",
        // when running eeschema.app
        wxFileName( stdPaths.GetExecutablePath() ).GetPath().ToStdString() + "/../../../../../Contents/PlugIns/sim"
    };
#else   // Unix systems
    const vector<string> dllPaths = { "/usr/local/lib" };
#endif

#if defined(__WINDOWS__) || (__WXMAC__)
    for( const auto& path : dllPaths )
    {
		//dllFile.SetPath( path );
		//wxLogTrace( traceNgspice, "libngspice search path: %s", dllFile.GetFullPath() );
		m_dll.load();

		if( m_dll.isLoaded() )
        {
			//wxLogTrace( traceNgspice, "libngspice path found in: %s", dllFile.GetFullPath() );
            break;
        }
    }

//	if( !m_dll.isLoaded() ) // try also the system libraries
//		m_dll.load( wxDynamicLibrary::CanonicalizeName( "ngspice" ) );

#else   // here: __LINUX__
    // First, try the system libraries
    m_dll.Load( NGSPICE_DLL_FILE, wxDL_VERBATIM | wxDL_QUIET | wxDL_NOW );

    // If failed, try some other paths:
    if( !m_dll.IsLoaded() )
    {
        for( const auto& path : dllPaths )
        {
            dllFile.SetPath( path );
            wxLogTrace( traceNgspice, "libngspice search path: %s", dllFile.GetFullPath() );
            m_dll.Load( dllFile.GetFullPath(), wxDL_VERBATIM | wxDL_QUIET | wxDL_NOW );

            if( m_dll.IsLoaded() )
            {
                wxLogTrace( traceNgspice, "libngspice path found in: %s", dllFile.GetFullPath() );
                break;
            }
        }
    }
#endif

	if( !m_dll.isLoaded() )
		throw std::runtime_error( "Missing ngspice shared library: " + m_dll.errorString().toStdString());

    m_error = false;

    // Obtain function pointers
	m_ngSpice_Init = (ngSpice_Init) m_dll.resolve( "ngSpice_Init" );
	m_ngSpice_Circ = (ngSpice_Circ) m_dll.resolve( "ngSpice_Circ" );
	m_ngSpice_Command = (ngSpice_Command) m_dll.resolve( "ngSpice_Command" );
	m_ngGet_Vec_Info = (ngGet_Vec_Info) m_dll.resolve( "ngGet_Vec_Info" );
	m_ngSpice_CurPlot  = (ngSpice_CurPlot) m_dll.resolve( "ngSpice_CurPlot" );
	m_ngSpice_AllPlots = (ngSpice_AllPlots) m_dll.resolve( "ngSpice_AllPlots" );
	m_ngSpice_AllVecs = (ngSpice_AllVecs) m_dll.resolve( "ngSpice_AllVecs" );
	m_ngSpice_Running = (ngSpice_Running) m_dll.resolve( "ngSpice_running" ); // it is not a typo

    m_ngSpice_Init( &cbSendChar, &cbSendStat, &cbControlledExit, NULL, NULL, &cbBGThreadRunning, this );

    // Load a custom spinit file, to fix the problem with loading .cm files
    // Switch to the executable directory, so the relative paths are correct
//	QString cwd( wxGetCwd() );
//    wxFileName exeDir( stdPaths.GetExecutablePath() );
//    wxSetWorkingDirectory( exeDir.GetPath() );

//    // Find *.cm files
//    string cmPath = findCmPath();

//    // __CMPATH is used in custom spinit file to point to the codemodels directory
//    if( !cmPath.empty() )
//        Command( "set __CMPATH=\"" + cmPath + "\"" );

//    // Possible relative locations for spinit file
//    const vector<string> spiceinitPaths =
//    {
//        ".",
//#ifdef __WXMAC__
//        stdPaths.GetPluginsDir().ToStdString() + "/sim/ngspice/scripts",
//        wxFileName( stdPaths.GetExecutablePath() ).GetPath().ToStdString() + "/../../../../../Contents/PlugIns/sim/ngspice/scripts"
//#endif /* __WXMAC__ */
//        "../share/kicad",
//        "../share",
//        "../../share/kicad",
//        "../../share"
//    };

//    bool foundSpiceinit = false;

//    for( const auto& path : spiceinitPaths )
//    {
//        wxLogTrace( traceNgspice, "ngspice init script search path: %s", path );

//        if( loadSpinit( path + "/spiceinit" ) )
//        {
//            wxLogTrace( traceNgspice, "ngspice path found in: %s", path );
//            foundSpiceinit = true;
//            break;
//        }
//    }

//    // Last chance to load codemodel files, we have not found
//    // spiceinit file, but we know the path to *.cm files
//    if( !foundSpiceinit && !cmPath.empty() )
//        loadCodemodels( cmPath );

//    // Restore the working directory
//    wxSetWorkingDirectory( cwd );

    // Workarounds to avoid hang ups on certain errors
    // These commands have to be called, no matter what is in the spinit file
    Command( "unset interactive" );
    Command( "set noaskquit" );
    Command( "set nomoremode" );

    m_initialized = true;
}


//bool NGSPICE::loadSpinit( const string& aFileName )
//{
//    if( !wxFileName::FileExists( aFileName ) )
//        return false;

//    wxTextFile file;

//    if( !file.Open( aFileName ) )
//        return false;

//    for( auto cmd = file.GetFirstLine(); !file.Eof(); cmd = file.GetNextLine() )
//        Command( cmd.ToStdString() );

//    return true;
//}


//string NGSPICE::findCmPath() const
//{
//    const vector<string> cmPaths =
//    {
//#ifdef __WXMAC__
//        "/Applications/ngspice/lib/ngspice",
//        "Contents/Frameworks",
//        wxStandardPaths::Get().GetPluginsDir().ToStdString() + "/sim/ngspice",
//        wxFileName( wxStandardPaths::Get().GetExecutablePath() ).GetPath().ToStdString() + "/../../../../../Contents/PlugIns/sim/ngspice",
//        "../Plugins/sim/ngspice",
//#endif /* __WXMAC__ */
//        "../lib/ngspice",
//        "../../lib/ngspice",
//        "lib/ngspice",
//        "ngspice"
//    };

//    for( const auto& path : cmPaths )
//    {
//        wxLogTrace( traceNgspice, "ngspice code models search path: %s", path );

//        if( wxFileName::FileExists( path + "/spice2poly.cm" ) )
//        {
//            wxLogTrace( traceNgspice, "ngspice code models found in: %s", path );
//            return path;
//        }
//    }

//    return string();
//}


//bool NGSPICE::loadCodemodels( const string& aPath )
//{
//    wxArrayString cmFiles;
//    size_t count = wxDir::GetAllFiles( aPath, &cmFiles );

//    for( const auto& cm : cmFiles )
//        Command( "codemodel " + cm.ToStdString() );

//    return count != 0;
//}


int NGSPICE::cbSendChar( char* what, int id, void* user )
{
    NGSPICE* sim = reinterpret_cast<NGSPICE*>( user );

    if( sim->m_reporter )
    {
        // strip stdout/stderr from the line
//        if( ( strncasecmp( what, "stdout ", 7 ) == 0 )
//                || ( strncasecmp( what, "stderr ", 7 ) == 0 ) )
//            what += 7;

//        sim->m_reporter->Report( what );

    }
	std::cout << "Simulator internal message: " << what << endl;
	QString msg(what);
	if (msg.contains("Fatal error")) {
		sim->m_error = true;
	}
    return 0;
}


int NGSPICE::cbSendStat( char* what, int id, void* user )
{
/*    NGSPICE* sim = reinterpret_cast<NGSPICE*>( user );
    if( sim->m_consoleReporter )
        sim->m_consoleReporter->Report( what );*/
	std::cout << what << endl;
    return 0;
}


int NGSPICE::cbBGThreadRunning( bool is_running, int id, void* user )
{
    NGSPICE* sim = reinterpret_cast<NGSPICE*>( user );
	sim->m_bgThreadIsRunning = !is_running;
	std::cout << "Simulator running: " << is_running << endl;
//    if( sim->m_reporter )
//        // I know the test below seems like an error, but well, it works somehow..
//        sim->m_reporter->OnSimStateChange( sim, is_running ? SIM_IDLE : SIM_RUNNING );

    return 0;
}


int NGSPICE::cbControlledExit( int status, bool immediate, bool exit_upon_quit, int id, void* user )
{
    // Something went wrong, reload the dll
    NGSPICE* sim = reinterpret_cast<NGSPICE*>( user );
    sim->m_error = true;

    return 0;
}


void NGSPICE::validate()
{
    if( m_error )
    {
        m_initialized = false;
        init_dll();
    }
}


const std::string NGSPICE::GetNetlist() const
{
    return m_netlist;
}

bool NGSPICE::ErrorFound() {
	return m_error;
}

bool NGSPICE::m_initialized = false;

std::atomic<unsigned int> LOCALE_IO::m_c_count( 0 );
LOCALE_IO::LOCALE_IO() : m_wxLocale( nullptr )
{
	// use thread safe, atomic operation
	if( m_c_count++ == 0 )
	{
#if USE_WXLOCALE
		m_wxLocale = new wxLocale( "C", "C", "C", false );
#else
		// Store the user locale name, to restore this locale later, in dtor
		m_user_locale = setlocale( LC_NUMERIC, nullptr );

		// Switch the locale to C locale, to read/write files with fp numbers
		setlocale( LC_NUMERIC, "C" );
#endif
	}
}


LOCALE_IO::~LOCALE_IO()
{
	// use thread safe, atomic operation
	if( --m_c_count == 0 )
	{
		// revert to the user locale
#if USE_WXLOCALE
		delete m_wxLocale;      // Deleting m_wxLocale restored previous locale
		m_wxLocale = nullptr;
#else
		setlocale( LC_NUMERIC, m_user_locale.c_str() );
#endif
	}
}
