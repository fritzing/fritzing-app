#!/usr/bin/env python
"""
Parse the GerbMerge configuration file.

--------------------------------------------------------------------

This program is licensed under the GNU General Public License (GPL).
See http://www.fsf.org for details of the license.

Andrew Sterian
Padnos College of Engineering and Computing
Grand Valley State University
<steriana@claymore.engineer.gvsu.edu>
<http://claymore.engineer.gvsu.edu/~steriana>
"""

import sys
import ConfigParser
import re
import string

import jobs
import aptable

# Configuration dictionary. Specify floats as strings. Ints can be specified
# as ints or strings.
Config = {
   'xspacing': '0.125',              # Spacing in horizontal direction
   'yspacing': '0.125',              # Spacing in vertical direction
   'panelwidth': '12.6',             # X-Dimension maximum panel size (Olimex)
   'panelheight': '7.8',             # Y-Dimension maximum panel size (Olimex)
   'cropmarklayers': None,           # e.g., *toplayer,*bottomlayer
   'cropmarkwidth': '0.01',          # Width (inches) of crop lines
   'cutlinelayers': None,            # as for cropmarklayers
   'cutlinewidth': '0.01',           # Width (inches) of cut lines
   'minimumfeaturesize': 0,          # Minimum dimension for selected layers
   'toollist': None,                 # Name of file containing default tool list
   'drillclustertolerance': '.002',  # Tolerance for clustering drill sizes
   'allowmissinglayers': 0,          # Set to 1 to allow multiple jobs to have non-matching layers
   'fabricationdrawingfile': None,   # Name of file to which to write fabrication drawing, or None
   'fabricationdrawingtext': None,   # Name of file containing text to write to fab drawing
   'excellondecimals': 4,            # Number of digits after the decimal point in input Excellon files
   'excellonleadingzeros': 0,        # Generate leading zeros in merged Excellon output file
   'outlinelayerfile': None,         # Name of file to which to write simple box outline, or None
   'scoringfile': None,              # Name of file to which to write scoring data, or None
   'leftmargin': 0,                  # Inches of extra room to leave on left side of panel for tooling
   'topmargin': 0,                   # Inches of extra room to leave on top side of panel for tooling
   'rightmargin': 0,                 # Inches of extra room to leave on right side of panel for tooling
   'bottommargin': 0,                # Inches of extra room to leave on bottom side of panel for tooling
   }

# This dictionary is indexed by lowercase layer name and has as values a file
# name to use for the output.
MergeOutputFiles = {
  'boardoutline': 'merged.boardoutline.ger',
  'drills':       'merged.drills.xln',
  'placement':    'merged.placement.txt',
  'toollist':     'merged.toollist.drl'
  }

# The global aperture table, indexed by aperture code (e.g., 'D10')
GAT = {}

# The global aperture macro table, indexed by macro name (e.g., 'M3', 'M4R' for rotated macros)
GAMT = {}

# The list of all jobs loaded, indexed by job name (e.g., 'PowerBoard')
Jobs = {}

# The set of all Gerber layer names encountered in all jobs. Doesn't
# include drills.
LayerList = {'boardoutline': 1}

# The tool list as read in from the DefaultToolList file in the configuration
# file. This is a dictionary indexed by tool name (e.g., 'T03') and
# a floating point number as the value, the drill diameter in inches.
DefaultToolList = {}

# The GlobalToolMap dictionary maps tool name to diameter in inches. It
# is initially empty and is constructed after all files are read in. It
# only contains actual tools used in jobs.
GlobalToolMap = {}

# The GlobalToolRMap dictionary is a reverse dictionary of ToolMap, i.e., it maps
# diameter to tool name.
GlobalToolRMap = {}

##############################################################################

# This configuration option determines whether trimGerber() is called
TrimGerber = 1

# This configuration option determines whether trimExcellon() is called
TrimExcellon = 1

# This configuration option determines the minimum size of feature dimensions for
# each layer. It is a dictionary indexed by layer name (e.g. '*topsilkscreen') and 
# has a floating point number as the value (in inches).
MinimumFeatureDimension = {}

# Construct the reverse-GAT/GAMT translation table, keyed by aperture/aperture macro
# hash string. The value is the aperture code (e.g., 'D10') or macro name (e.g., 'M5').
def buildRevDict(D):
  RevD = {}
  for key,val in D.items():
    RevD[val.hash()] = key
  return RevD

def parseStringList(L):
  """Parse something like '*toplayer, *bottomlayer' into a list of names
     without quotes, spaces, etc."""

  if 0:
    if L[0]=="'":
      if L[-1] != "'":
        raise RuntimeError, "Illegal configuration string '%s'" % L
      L = L[1:-1]

    elif L[0]=='"':
      if L[-1] != '"':
        raise RuntimeError, "Illegal configuration string '%s'" % L
      L = L[1:-1]

  # This pattern matches quotes at the beginning and end...quotes must match
  quotepat = re.compile(r'^([' "'" '"' r']?)([^\1]*)\1$')
  delimitpat = re.compile(r'[ \t]*[,;][ \t]*')

  match = quotepat.match(L)
  if match:
    L = match.group(2)

  return delimitpat.split(L)

# Parse an Excellon tool list file of the form
#
#   T01 0.035in
#   T02 0.042in
def parseToolList(fname):  
  TL = {}

  try:
    fid = file(fname, 'rt')
  except Exception, detail:
    raise RuntimeError, "Unable to open tool list file '%s':\n  %s" % (fname, str(detail))

  pat_in  = re.compile(r'\s*(T\d+)\s+([0-9.]+)\s*in\s*')
  pat_mm  = re.compile(r'\s*(T\d+)\s+([0-9.]+)\s*mm\s*')
  pat_mil = re.compile(r'\s*(T\d+)\s+([0-9.]+)\s*(?:mil)?')
  for line in fid.xreadlines():
    line = string.strip(line)
    if (not line) or (line[0] in ('#', ';')): continue

    mm = 0
    mil = 0
    match = pat_in.match(line)
    if not match:
      mm = 1
      match = pat_mm.match(line)
      if not match:
        mil = 1
        match = pat_mil.match(line)
        if not match:
          continue
          #raise RuntimeError, "Illegal tool list specification:\n  %s" % line

    tool, size = match.groups()

    try:
      size = float(size)
    except:
      raise RuntimeError, "Tool size in file '%s' is not a valid floating-point number:\n  %s" % (fname,line)

    if mil:
      size = size*0.001  # Convert mil to inches
    elif mm:    
      size = size/25.4   # Convert mm to inches

    # Canonicalize tool so that T1 becomes T01
    tool = 'T%02d' % int(tool[1:])

    if TL.has_key(tool):
      raise RuntimeError, "Tool '%s' defined more than once in tool list file '%s'" % (tool,fname)

    TL[tool]=size
  fid.close()

  return TL

# This function parses the job configuration file and does
# everything needed to:
#
#   * parse global options and store them in the Config dictionary
#     as natural types (i.e., ints, floats, lists)
#
#   * Read Gerber/Excellon data and populate the Jobs dictionary
#
#   * Read Gerber/Excellon data and populate the global aperture
#     table, GAT, and the global aperture macro table, GAMT
#
#   * read the tool list file and populate the DefaultToolList dictionary
def parseConfigFile(fname, Config=Config, Jobs=Jobs):
  global DefaultToolList

  CP = ConfigParser.ConfigParser()
  CP.readfp(file(fname,'rt'))

  # First parse global options
  if CP.has_section('Options'):
    for opt in CP.options('Options'):
      # Is it one we expect
      if Config.has_key(opt):
        # Yup...override it
        Config[opt] = CP.get('Options', opt)

      elif CP.defaults().has_key(opt):
        pass   # Ignore DEFAULTS section keys

      elif opt in ('fabricationdrawing', 'outlinelayer'):
        print '*'*73
        print '\nThe FabricationDrawing and OutlineLayer configuration options have been'
        print 'renamed as of GerbMerge version 1.0. Please consult the documentation for'
        print 'a description of the new options, then modify your configuration file.\n'
        print '*'*73
        sys.exit(1)
      else:
        raise RuntimeError, "Unknown option '%s' in [Options] section of configuration file" % opt
  else:
    raise RuntimeError, "Missing [Options] section in configuration file"

  # Ensure we got a tool list
  if not Config.has_key('toollist'):
    raise RuntimeError, "INTERNAL ERROR: Missing tool list assignment in [Options] section"

  # Make integers integers, floats floats
  for key,val in Config.items():
    try:
      val = int(val)
      Config[key]=val
    except:
      try:
        val = float(val)
        Config[key]=val
      except:
        pass

  # Process lists of strings
  if Config['cutlinelayers']:
    Config['cutlinelayers'] = parseStringList(Config['cutlinelayers'])
  if Config['cropmarklayers']:
    Config['cropmarklayers'] = parseStringList(Config['cropmarklayers'])
    
  # Process list of minimum feature dimensions
  if Config['minimumfeaturesize']:
    temp = Config['minimumfeaturesize'].split(",")
    try:
      for index in range(0, len(temp), 2):
        MinimumFeatureDimension[ temp[index] ] = float( temp[index + 1] )
    except:
      raise RuntimeError, "Illegal configuration string:" + Config['minimumfeaturesize']

  # Process MergeOutputFiles section to set output file names
  if CP.has_section('MergeOutputFiles'):
    for opt in CP.options('MergeOutputFiles'):
      # Each option is a layer name and the output file for this name
      if opt[0]=='*' or opt in ('boardoutline', 'drills', 'placement', 'toollist'):
        MergeOutputFiles[opt] = CP.get('MergeOutputFiles', opt)

  # Now, we go through all jobs and collect Gerber layers
  # so we can construct the Global Aperture Table.
  apfiles = []

  for jobname in CP.sections():
    if jobname=='Options': continue
    if jobname=='MergeOutputFiles': continue

    # Ensure all jobs have a board outline and drills
    if not CP.has_option(jobname, 'boardoutline'):
      raise RuntimeError, "Job '%s' does not have a board outline specified" % jobname

    if not CP.has_option(jobname, 'drills'):
      raise RuntimeError, "Job '%s' does not have a drills layer specified" % jobname

    for layername in CP.options(jobname):
      if layername[0]=='*' or layername=='boardoutline':
        fname = CP.get(jobname, layername)
        apfiles.append(fname)

        if layername[0]=='*':
          LayerList[layername]=1

  # Now construct global aperture tables, GAT and GAMT. This step actually
  # reads in the jobs for aperture data but doesn't store Gerber
  # data yet.
  aptable.constructApertureTable(apfiles)
  del apfiles

  if 0:
    keylist = GAMT.keys()
    keylist.sort()
    for key in keylist:
      print '%s' % GAMT[key]
    sys.exit(0)

  # Parse the tool list
  if Config['toollist']:
    DefaultToolList = parseToolList(Config['toollist'])

  # Now get jobs. Each job implies layer names, and we
  # expect consistency in layer names from one job to the
  # next. Two reserved layer names, however, are
  # BoardOutline and Drills.

  Jobs.clear()

  do_abort = 0
  errstr = 'ERROR'
  if Config['allowmissinglayers']:
    errstr = 'WARNING'

  for jobname in CP.sections():
    if jobname=='Options': continue
    if jobname=='MergeOutputFiles': continue

    print 'Reading data from', jobname, '...'

    J = jobs.Job(jobname)

    # Parse the job settings, like tool list, first, since we are not
    # guaranteed to have ConfigParser return the layers in the same order that
    # the user wrote them, and we may get Gerber files before we get a tool
    # list! Same thing goes for ExcellonDecimals. We need to know what this is
    # before parsing any Excellon files.
    for layername in CP.options(jobname):
      fname = CP.get(jobname, layername)

      if layername == 'toollist':
        J.ToolList = parseToolList(fname)
      elif layername=='excellondecimals':
        try:
          J.ExcellonDecimals = int(fname)
        except:
          raise RuntimeError, "Excellon decimals '%s' in config file is not a valid integer" % fname
      elif layername=='repeat':
        try:
          J.Repeat = int(fname)
        except:
          raise RuntimeError, "Repeat count '%s' in config file is not a valid integer" % fname

    for layername in CP.options(jobname):
      fname = CP.get(jobname, layername)

      if layername=='boardoutline':
        J.parseGerber(fname, layername, updateExtents=1)
      elif layername[0]=='*':
        J.parseGerber(fname, layername, updateExtents=0)
      elif layername=='drills':
        J.parseExcellon(fname)

    # Emit warnings if some layers are missing
    LL = LayerList.copy()
    for layername in J.apxlat.keys():
      assert LL.has_key(layername)
      del LL[layername]

    if LL:
      if errstr=='ERROR':
        do_abort=1

      print '%s: Job %s is missing the following layers:' % (errstr, jobname)
      for layername in LL.keys():
        print '  %s' % layername

    # Store the job in the global Jobs dictionary, keyed by job name
    Jobs[jobname] = J

  if do_abort:
    raise RuntimeError, 'Exiting since jobs are missing layers. Set AllowMissingLayers=1\nto override.'

if __name__=="__main__":
  CP = parseConfigFile(sys.argv[1])
  print Config
  sys.exit(0)

  if 0:
    for key, val in CP.defaults().items():
      print '%s: %s' % (key,val)

    for section in CP.sections():
      print '[%s]' % section
      for opt in CP.options(section):
        print '  %s=%s' % (opt, CP.get(section, opt))
