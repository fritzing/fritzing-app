#!/usr/bin/env python
"""
This module reads all Gerber and Excellon files and stores the
data for each job.

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
import re
import string
import __builtin__
import copy
import types

import aptable
import config
import makestroke
import amacro
import geometry
import util

# Parsing Gerber/Excellon files is currently very brittle. A more robust
# RS274X/Excellon parser would be a good idea and allow this program to work
# robustly with more than just Eagle CAM files.

# Reminder to self:
#
#   D01 -- move and draw line with exposure on
#   D02 -- move with exposure off
#   D03 -- flash aperture

# Patterns for Gerber RS274X file interpretation
apdef_pat = re.compile(r'^%AD(D\d+)([^*$]+)\*%$')     # Aperture definition
apmdef_pat = re.compile(r'^%AM([^*$]+)\*$')           # Aperture macro definition
comment_pat = re.compile(r'G0?4[^*]*\*')              # Comment (GerbTool comment omits the 0)
tool_pat  = re.compile(r'(D\d+)\*')                   # Aperture selection
gcode_pat = re.compile(r'G(\d{1,2})\*?')              # G-codes
drawXY_pat = re.compile(r'X([+-]?\d+)Y([+-]?\d+)D0?([123])\*')  # Drawing command
drawX_pat  = re.compile(r'X([+-]?\d+)D0?([123])\*')        # Drawing command, Y is implied
drawY_pat  = re.compile(r'Y([+-]?\d+)D0?([123])\*')        # Drawing command, X is implied
format_pat = re.compile(r'%FS(L|T)?(A|I)(N\d+)?(X\d\d)(Y\d\d)\*%')  # Format statement
layerpol_pat = re.compile(r'^%LP[CD]\*%')             # Layer polarity (D=dark, C=clear)

# Circular interpolation drawing commands (from Protel)
cdrawXY_pat = re.compile(r'X([+-]?\d+)Y([+-]?\d+)I([+-]?\d+)J([+-]?\d+)D0?([123])\*')
cdrawX_pat  = re.compile(r'X([+-]?\d+)I([+-]?\d+)J([+-]?\d+)D0?([123])\*')  # Y is implied
cdrawY_pat  = re.compile(r'Y([+-]?\d+)I([+-]?\d+)J([+-]?\d+)D0?([123])\*')  # X is implied

IgnoreList = ( \
  # These are for Eagle, and RS274X files in general
  re.compile(r'^%OFA0B0\*%$'),
  re.compile(r'^%IPPOS\*%'),
  re.compile(r'^%AMOC8\*$'),                         # Eagle's octagon defined by macro with a $1 parameter
  re.compile(r'^5,1,8,0,0,1\.08239X\$1,22\.5\*$'),   # Eagle's octagon, 22.5 degree rotation
  re.compile(r'^5,1,8,0,0,1\.08239X\$1,0\.0\*$'),    # Eagle's octagon, 0.0 degree rotation
  re.compile(r'^\*?%$'),
  re.compile(r'^M0?2\*$'),

  # These additional ones are for Orcad Layout, PCB, Protel, etc.
  re.compile(r'\*'),            # Empty statement
  re.compile(r'^%IN.*\*%'),
  re.compile(r'^%ICAS\*%'),      # Not in RS274X spec.
  re.compile(r'^%MOIN\*%'),
  re.compile(r'^%ASAXBY\*%'),
  re.compile(r'^%AD\*%'),        # GerbTool empty aperture definition
  re.compile(r'^%LN.*\*%')       # Layer name
  )

# Patterns for Excellon interpretation
xtool_pat = re.compile(r'^(T\d+)$')           # Tool selection
xydraw_pat = re.compile(r'^X([+-]?\d+)Y([+-]?\d+)$')    # Plunge command
xdraw_pat = re.compile(r'^X([+-]?\d+)$')    # Plunge command, repeat last Y value
ydraw_pat = re.compile(r'^Y([+-]?\d+)$')    # Plunge command, repeat last X value
xtdef_pat = re.compile(r'^(T\d+)(?:F\d+)?(?:S\d+)?C([0-9.]+)$') # Tool+diameter definition with optional
                                                                # feed/speed (for Protel)
xtdef2_pat = re.compile(r'^(T\d+)C([0-9.]+)(?:F\d+)?(?:S\d+)?$') # Tool+diameter definition with optional
                                                                # feed/speed at the end (for OrCAD)
xzsup_pat = re.compile(r'^INCH,([LT])Z$')      # Leading/trailing zeros INCLUDED

XIgnoreList = ( \
  re.compile(r'^%$'),
  re.compile(r'^M30$'),   # End of job
  re.compile(r'^M48$'),   # Program header to first %
  re.compile(r'^M72$')    # Inches
  )

# A Job is a single input board. It is expected to have:
#    - a board outline file in RS274X format
#    - several (at least one) Gerber files in RS274X format
#    - a drill file in Excellon format
#
# The board outline and Excellon filenames must be given separately.
# The board outline file determines the extents of the job.

class Job:
  def __init__(self, name):
    self.name = name

    # Minimum and maximum (X,Y) absolute co-ordinates encountered
    # in GERBER data only (not Excellon). Note that coordinates
    # are stored in hundred-thousandsths of an inch so 9999999 is 99.99999
    # inches.
    self.maxx = self.maxy = -9999999  # in the case all coordinates are < 0, this will prevent maxx and maxy from defaulting to 0
    self.minx = self.miny = 9999999

    # Aperture translation table relative to GAT. This dictionary
    # has as each key a layer name for the job. Each key's value
    # is itself a dictionary where each key is an aperture in the file.
    # The value is the key in the GAT. Example:
    #       apxlat['TopCopper']['D10'] = 'D12'
    #       apxlat['TopCopper']['D11'] = 'D15'
    #       apxlat['BottomCopper']['D10'] = 'D15'
    self.apxlat = {}

    # Aperture macro translation table relative to GAMT. This dictionary
    # has as each key a layer name for the job. Each key's value
    # is itself a dictionary where each key is an aperture macro name in the file.
    # The value is the key in the GAMT. Example:
    #       apxlat['TopCopper']['THD10X'] = 'M1'
    #       apxlat['BottomCopper']['AND10'] = 'M5'
    self.apmxlat = {}

    # Commands are one of:
    #     A. strings for:
    #           - aperture changes like "D12"
    #           - G-code commands like "G36"
    #           - RS-274X commands like "%LPD*%" that begin with '%'
    #     B. (X,Y,D) triples comprising X,Y integers in the range 0 through 999999
    #        and draw commands that are either D01, D02, or D03. The character
    #        D in the triple above is the integer 1, 2, or 3.
    #     C. (X,Y,I,J,D,s) 6-tuples comprising X,Y,I,J integers in the range 0 through 999999
    #        and D as with (X,Y,D) triples. The 's' integer is non-zero to indicate that
    #        the (I,J) tuple is a SIGNED offset (for multi-quadrant circular interpolation)
    #        else the tuple is unsigned.
    #
    # This variable is, as for apxlat, a dictionary keyed by layer name.
    self.commands = {}

    # This dictionary stores all GLOBAL apertures actually needed by this
    # layer, i.e., apertures specified prior to draw commands.  The dictionary
    # is indexed by layer name, and each dictionary entry is a list of aperture
    # code strings, like 'D12'. This dictionary helps us to figure out the
    # minimum number of apertures that need to be written out in the Gerber
    # header of the merged file. Once again, the list of apertures refers to
    # GLOBAL aperture codes in the GAT, not ones local to this layer.
    self.apertures = {}

    # Excellon commands are grouped by tool number in a dictionary.
    # This is to help sorting all jobs and writing out all plunge
    # commands for a single tool.
    # 
    # The key to this dictionary is the full tool name, e.g., T03
    # as a string. Each command is an (X,Y) integer tuple.
    self.xcommands = {}

    # This is a dictionary mapping LOCAL tool names (e.g., T03) to diameters
    # in inches for THIS JOB. This dictionary will be initially empty
    # for old-style Excellon files with no embedded tool sizes. The
    # main program will construct this dictionary from the global tool
    # table in this case, once all jobs have been read in.
    self.xdiam = {}

    # This is a mapping from tool name to diameter for THIS JOB
    self.ToolList = None

    # How many times to replicate this job if using auto-placement
    self.Repeat = 1

    # How many decimal digits of precision there are in the Excellon file.
    # A value greater than 0 overrides the global ExcellonDecimals setting
    # for this file, allowing jobs with different Excellon decimal settings
    # to be combined.
    self.ExcellonDecimals = 0     # 0 means global value prevails

  def width_in(self):
    "Return width in INCHES"
    return float(self.maxx-self.minx)*0.00001

  def height_in(self):
    "Return height in INCHES"
    return float(self.maxy-self.miny)*0.00001

  def jobarea(self):
    return self.width_in()*self.height_in()

  def maxdimension(self):
    return max(self.width_in(),self.height_in())

  def mincoordinates(self):
    "Return minimum X and Y coordinate"
    
    return self.minx, self.miny

  def fixcoordinates(self, x_shift, y_shift):
    "Add x_shift and y_shift to all coordinates in the job"
    
    # Shift maximum and minimum coordinates
    self.minx += x_shift
    self.maxx += x_shift
    self.miny += y_shift
    self.maxy += y_shift

    # Shift all commands
    for layer, command in self.commands.iteritems():
    
      # Loop through each command in each layer
      for index in range( len(command) ):
        c = command[index]
        
        # Shift X and Y coordinate of command
        if type(c) == types.TupleType:                      ## ensure that command is of type tuple
          command_list = list(c)                            ## convert tuple to list
          if  (type( command_list[0] ) == types.IntType) \
          and (type( command_list[1] ) == types.IntType):  ## ensure that first two elemenst are integers
            command_list[0] += x_shift
            command_list[1] += y_shift
          command[index] = tuple(command_list)              ## convert list back to tuple
          
      self.commands[layer] = command                        ## set modified command
     
    # Shift all excellon commands
    for tool, command in self.xcommands.iteritems():
    
      # Loop through each command in each layer
      for index in range( len(command) ):
        c = command[index]
        
        # Shift X and Y coordinate of command
        command_list = list(c)                              ## convert tuple to list
        if ( type( command_list[0] ) == types.IntType ) \
        and ( type( command_list[1] ) == types.IntType ):  ## ensure that first two elemenst are integers
          command_list[0] += x_shift / 10
          command_list[1] += y_shift / 10
        command[index] = tuple(command_list)                ## convert list back to tuple
        
      self.xcommands[tool] = command                        ## set modified command

  def parseGerber(self, fullname, layername, updateExtents = 0):
    """Do the dirty work. Read the Gerber file given the
       global aperture table GAT and global aperture macro table GAMT"""

    GAT = config.GAT
    GAMT = config.GAMT
    # First construct reverse GAT/GAMT, mapping definition to code
    RevGAT = config.buildRevDict(GAT)     # RevGAT[hash] = aperturename
    RevGAMT = config.buildRevDict(GAMT)   # RevGAMT[hash] = aperturemacroname

    #print 'Reading data from %s ...' % fullname

    fid = file(fullname, 'rt')
    currtool = None

    self.apxlat[layername] = {}
    self.apmxlat[layername] = {}
    self.commands[layername] = []
    self.apertures[layername] = []

    # These divisors are used to scale (X,Y) co-ordinates. We store
    # everything as integers in hundred-thousandths of an inch (i.e., M.5
    # format). If we get something in M.4 format, we must multiply by
    # 10. If we get something in M.6 format we must divide by 10, etc.
    x_div = 1.0
    y_div = 1.0

    # Drawing commands can be repeated with X or Y omitted if they are
    # the same as before. These variables store the last X/Y value as
    # integers in hundred-thousandths of an inch.
    last_x = last_y = 0

    # Last modal G-code. Some G-codes introduce "modes", such as circular interpolation
    # mode, and we want to remember what mode we're in. We're interested in:
    #    G01 -- linear interpolation, cancels all circular interpolation modes
    #    G36 -- Turn on polygon area fill
    #    G37 -- Turn off polygon area fill
    last_gmode = 1  # G01 by default, linear interpolation

    # We want to know whether to do signed (G75) or unsigned (G74) I/J offsets. These
    # modes are independent of G01/G02/G03, e.g., Protel will issue multiple G03/G01
    # codes all in G75 mode.
    #    G74 -- Single-quadrant circular interpolation (disables multi-quadrant interpolation)
    #           G02/G03 codes set clockwise/counterclockwise arcs in a single quadrant only
    #           using X/Y/I/J commands with UNSIGNED (I,J).
    #    G75 -- Multi-quadrant circular interpolation --> X/Y/I/J with signed (I,J)
    #           G02/G03 codes set clockwise/counterclockwise arcs in all 4 quadrants
    #           using X/Y/I/J commands with SIGNED (I,J).
    circ_signed = True   # Assume G75...make sure this matches canned header we write out

    # If the very first flash/draw is a shorthand command (i.e., without an Xxxxx or Yxxxx)
    # component then we don't really "see" the first point X00000Y00000. To account for this
    # we use the following Boolean flag as well as the isLastShorthand flag during parsing
    # to manually insert the point X000000Y00000 into the command stream.
    firstFlash = True

    for line in fid:
      # Get rid of CR characters (0x0D) and leading/trailing blanks
      line = string.replace(line, '\x0D', '').strip()

      # Old location of format_pat search. Now moved down into the sub-line parse loop below.

      # RS-274X statement? If so, echo it. Currently, only the "LP" statement is expected
      # (from Protel, of course). These will be distinguished from D-code and G-code
      # commands by the fact that the first character of the string is '%'.
      match = layerpol_pat.match(line)
      if match:
        self.commands[layername].append(line)
        continue
        
      # See if this is an aperture definition, and if so, map it.
      match = apdef_pat.match(line)
      if match:
        if currtool:
          raise RuntimeError, "File %s has an aperture definition that comes after drawing commands." % fullname

        A = aptable.parseAperture(line, self.apmxlat[layername])
        if not A:
          raise RuntimeError, "Unknown aperture definition in file %s" % fullname

        hash = A.hash()
        if not RevGAT.has_key(hash):
          #print line
          #print self.apmxlat
          #print RevGAT
          raise RuntimeError, 'File %s has aperture definition "%s" not in global aperture table.' % (fullname, hash)

        # This says that all draw commands with this aperture code will
        # be replaced by aperture self.apxlat[layername][code].
        self.apxlat[layername][A.code] = RevGAT[hash]
        continue

      # Ignore %AMOC8* from Eagle for now as it uses a macro parameter, which
      # is not yet supported in GerbMerge.
      if line[:7]=='%AMOC8*':
        continue

      # See if this is an aperture macro definition, and if so, map it.
      M = amacro.parseApertureMacro(line,fid)
      if M:
        if currtool:
          raise RuntimeError, "File %s has an aperture macro definition that comes after drawing commands." % fullname

        hash = M.hash()
        if not RevGAMT.has_key(hash):
          raise RuntimeError, 'File %s has aperture macro definition not in global aperture macro table:\n%s' % (fullname, hash)

        # This says that all aperture definition commands that reference this macro name
        # will be replaced by aperture macro name self.apmxlat[layername][macroname].
        self.apmxlat[layername][M.name] = RevGAMT[hash]
        continue

      # From this point on we may have more than one match on this line, e.g.:
      #   G54D11*X22400Y22300D02*X22500Y22200D01*
      sub_line = line
      while sub_line:
        # Handle "comment" G-codes first
        match = comment_pat.match(sub_line)
        if match:
          sub_line = sub_line[match.end():]
          continue

        # See if this is a format statement, and if so, map it. In version 1.3 this was moved down
        # from the line-only parse checks above (see comment) to handle OrCAD lines like
        # G74*%FSLAN2X34Y34*%
        match = format_pat.match(sub_line)   # Used to be format_pat.search
        if match:
          sub_line = sub_line[match.end():]
          for item in match.groups():
            if item is None: continue   # Optional group didn't match

            if item[0] in "LA":   # omit leading zeroes and absolute co-ordinates
              continue

            if item[0]=='T':      # omit trailing zeroes
              raise RuntimeError, "Trailing zeroes not supported in RS274X files"
            if item[0]=='I':      # incremental co-ordinates
              raise RuntimeError, "Incremental co-ordinates not supported in RS274X files"

            if item[0]=='N':      # Maximum digits for N* commands...ignore it
              continue

            if item[0]=='X':      # M.N specification for X-axis.
              fracpart = int(item[2])
              x_div = 10.0**(5-fracpart)
            if item[0]=='Y':      # M.N specification for Y-axis.
              fracpart = int(item[2])
              y_div = 10.0**(5-fracpart)
          continue

        # Parse and interpret G-codes
        match = gcode_pat.match(sub_line)
        if match:
          sub_line = sub_line[match.end():]
          gcode = int(match.group(1))

          # Determine if this is a G-Code that should be ignored because it has no effect
          # (e.g., G70 specifies "inches" which is already in effect).
          if gcode in [54, 70, 90]:
            continue

          # Determine if this is a G-Code that we have to emit because it matters.
          if gcode in [1, 2, 3, 36, 37, 74, 75]:
            self.commands[layername].append("G%02d" % gcode)

            # Determine if this is a G-code that sets a new mode
            if gcode in [1, 36, 37]:
              last_gmode = gcode

            # Remember last G74/G75 code so we know whether to do signed or unsigned I/J
            # offsets.
            if gcode==74:
              circ_signed = False
            elif gcode==75:
              circ_signed = True
              
            continue

          raise RuntimeError, "G-Code 'G%02d' is not supported" % gcode

        # See if this is a tool change (aperture change) command
        match = tool_pat.match(sub_line)
        if match:
          currtool = match.group(1)

          # Protel likes to issue random D01, D02, and D03 commands instead of aperture
          # codes. We can ignore D01 because it simply means to move to the current location
          # while drawing. Well, that's drawing a point. We can ignore D02 because it means
          # to move to the current location without drawing. Truly pointless. We do NOT want
          # to ignore D03 because it implies a flash. Protel very inefficiently issues a D02
          # move to a location without drawing, then a single-line D03 to flash. However, a D02
          # terminates a polygon in G36 mode, so keep D02's in this case.
          if currtool=='D01' or (currtool=='D02' and (last_gmode != 36)):
            sub_line = sub_line[match.end():]
            continue

          if (currtool == 'D03') or (currtool=='D02' and (last_gmode == 36)):
            self.commands[layername].append(currtool)
            sub_line = sub_line[match.end():]
            continue

          # Map it using our translation table
          if not self.apxlat[layername].has_key(currtool):
            raise RuntimeError, 'File %s has tool change command "%s" with no corresponding translation' % (fullname, currtool)

          currtool = self.apxlat[layername][currtool]

          # Add it to the list of things to write out
          self.commands[layername].append(currtool)

          # Add it to the list of all apertures needed by this layer
          self.apertures[layername].append(currtool)

          # Move on to next match, if any
          sub_line = sub_line[match.end():]
          continue

        # Is it a simple draw command?
        I = J = None  # For circular interpolation drawing commands
        match = drawXY_pat.match(sub_line)
        isLastShorthand = False    # By default assume we don't make use of last_x and last_y
        if match:
          x, y, d = map(__builtin__.int, match.groups())
        else:
          match = drawX_pat.match(sub_line)
          if match:
            x, d = map(__builtin__.int, match.groups())
            y = last_y
            isLastShorthand = True  # Indicate we're making use of last_x/last_y
          else:
            match = drawY_pat.match(sub_line)
            if match:
              y, d = map(__builtin__.int, match.groups())
              x = last_x
              isLastShorthand = True  # Indicate we're making use of last_x/last_y

        # Maybe it's a circular interpolation draw command with IJ components
        if match is None:
          match = cdrawXY_pat.match(sub_line)
          if match:
            x, y, I, J, d = map(__builtin__.int, match.groups())
          else:
            match = cdrawX_pat.match(sub_line)
            if match:
              x, I, J, d = map(__builtin__.int, match.groups())
              y = last_y
              isLastShorthand = True  # Indicate we're making use of last_x/last_y
            else:
              match = cdrawY_pat.match(sub_line)
              if match:
                y, I, J, d = map(__builtin__.int, match.groups())
                x = last_x
                isLastShorthand = True  # Indicate we're making use of last_x/last_y

        if match:
          if currtool is None:
            # It's OK if this is an exposure-off movement command (specified with D02).
            # It's also OK if we're in the middle of a G36 polygon fill as we're only defining
            # the polygon extents.
            if (d != 2) and (last_gmode != 36):
              raise RuntimeError, 'File %s has draw command %s with no aperture chosen' % (fullname, sub_line)

          # Save last_x/y BEFORE scaling to 2.5 format else subsequent single-ordinate
          # flashes (e.g., Y with no X) will be scaled twice!
          last_x = x
          last_y = y

          # Corner case: if this is the first flash/draw and we are using shorthand (i.e., missing Xxxx
          # or Yxxxxx) then prepend the point X0000Y0000 into the commands as it is actually the starting
          # point of our layer. We prepend the command X0000Y0000D02, i.e., a move to (0,0) without drawing.
          if (isLastShorthand and firstFlash):
            self.commands[layername].append((0,0,2))
            if updateExtents:
              self.minx = min(self.minx,0)
              self.maxx = max(self.maxx,0)
              self.miny = min(self.miny,0)
              self.maxy = max(self.maxy,0)

          x = int(round(x*x_div))
          y = int(round(y*y_div))
          if I is not None:
            I = int(round(I*x_div))
            J = int(round(J*y_div))
            self.commands[layername].append((x,y,I,J,d,circ_signed))
          else:
            self.commands[layername].append((x,y,d))
          firstFlash = False

          # Update dimensions...this is complicated for circular interpolation commands
          # that span more than one quadrant. For now, we ignore this problem since users
          # should be using a border layer to indicate extents.
          if updateExtents:
            if x < self.minx: self.minx = x
            if x > self.maxx: self.maxx = x
            if y < self.miny: self.miny = y
            if y > self.maxy: self.maxy = y

          # Move on to next match, if any
          sub_line = sub_line[match.end():]
          continue

        # If it's none of the above, it had better be on our ignore list.
        for pat in IgnoreList:
          match = pat.match(sub_line)
          if match:
            break
        else:
          raise RuntimeError, 'File %s has uninterpretable line:\n  %s' % (fullname, line)

        sub_line = sub_line[match.end():]
      # end while still things to match on this line
    # end of for each line in file

    fid.close()
    if 0:
      print layername
      print self.commands[layername]

  def parseExcellon(self, fullname):
    #print 'Reading data from %s ...' % fullname

    fid = file(fullname, 'rt')
    currtool = None
    suppress_leading = True     # Suppress leading zeros by default, equivalent to 'INCH,TZ'

    # We store Excellon X/Y data in ten-thousandths of an inch. If the Config
    # option ExcellonDecimals is not 4, we must adjust the values read from the
    # file by a divisor to convert to ten-thousandths.  This is only used in
    # leading-zero suppression mode. In trailing-zero suppression mode, we must
    # trailing-zero-pad all input integers to M+N digits (e.g., 6 digits for 2.4 mode)
    # specified by the 'zeropadto' variable.
    if self.ExcellonDecimals > 0:
      divisor = 10.0**(4 - self.ExcellonDecimals)
      zeropadto = 2+self.ExcellonDecimals
    else:
      divisor = 10.0**(4 - config.Config['excellondecimals'])
      zeropadto = 2+config.Config['excellondecimals']
    
    # Protel takes advantage of optional X/Y components when the previous one is the same,
    # so we have to remember them.
    last_x = last_y = 0

    # Helper function to convert X/Y strings into integers in units of ten-thousandth of an inch.
    def xln2tenthou(L, divisor=divisor, zeropadto=zeropadto):
      V = []
      for s in L:
        if not suppress_leading:
          s = s + '0'*(zeropadto-len(s))
        V.append(int(round(int(s)*divisor)))
      return tuple(V)

    for line in fid.xreadlines():
      # Get rid of CR characters
      line = string.replace(line, '\x0D', '')

      # Protel likes to embed comment lines beginning with ';'
      if line[0]==';':
        continue

      # Check for leading/trailing zeros included ("INCH,LZ" or "INCH,TZ")
      match = xzsup_pat.match(line)
      if match:
        if match.group(1)=='L':
          # LZ --> Leading zeros INCLUDED
          suppress_leading = False
        else:
          # TZ --> Trailing zeros INCLUDED
          suppress_leading = True
        continue
        
      # See if a tool is being defined. First try to match with tool name+size
      match = xtdef_pat.match(line)    # xtdef_pat and xtdef2_pat expect tool name and diameter
      if match is None:                # but xtdef_pat expects optional feed/speed between T and C
        match = xtdef2_pat.match(line) # and xtdef_2pat expects feed/speed at the end
      if match:
        currtool, diam = match.groups()
        try:
          diam = float(diam)
        except:
          raise RuntimeError, "File %s has illegal tool diameter '%s'" % (fullname, diam)

        # Canonicalize tool number because Protel (of course) sometimes specifies it
        # as T01 and sometimes as T1. We canonicalize to T01.
        currtool = 'T%02d' % int(currtool[1:])

        if self.xdiam.has_key(currtool):
          raise RuntimeError, "File %s defines tool %s more than once" % (fullname, currtool)
        self.xdiam[currtool] = diam
        continue

      # Didn't match TxxxCyyy. It could be a tool change command 'Tdd'.
      match = xtool_pat.match(line)
      if match:
        currtool = match.group(1)

        # Canonicalize tool number because Protel (of course) sometimes specifies it
        # as T01 and sometimes as T1. We canonicalize to T01.
        currtool = 'T%02d' % int(currtool[1:])

        # Diameter will be obtained from embedded tool definition, local tool list or if not found, the global tool list
        try:
          diam = self.xdiam[currtool]
        except:
          if self.ToolList:
            try:
              diam = self.ToolList[currtool]
            except:
              raise RuntimeError, "File %s uses tool code %s that is not defined in the job's tool list" % (fullname, currtool)
          else:
            try:
              diam = config.DefaultToolList[currtool]
            except:
              #print config.DefaultToolList
              raise RuntimeError, "File %s uses tool code %s that is not defined in default tool list" % (fullname, currtool)

        self.xdiam[currtool] = diam
        continue

      # Plunge command?
      match = xydraw_pat.match(line)
      if match:
        x, y = xln2tenthou(match.groups())
      else:
        match = xdraw_pat.match(line)
        if match:
          x = xln2tenthou(match.groups())[0]
          y = last_y
        else:
          match = ydraw_pat.match(line)
          if match:
            y = xln2tenthou(match.groups())[0]
            x = last_x
          
      if match:
        if currtool is None:
          raise RuntimeError, 'File %s has plunge command without previous tool selection' % fullname

        try:
          self.xcommands[currtool].append((x,y))
        except KeyError:
          self.xcommands[currtool] = [(x,y)]

        last_x = x
        last_y = y
        continue

      # It had better be an ignorable
      for pat in XIgnoreList:
        if pat.match(line):
          break
      else:
        raise RuntimeError, 'File %s has uninterpretable line:\n  %s' % (fullname, line)

  def hasLayer(self, layername):                        #### ADS START
    return self.commands.has_key(layername)

  def writeGerber(self, fid, layername, Xoff, Yoff):
    "Write out the data such that the lower-left corner of this job is at the given (X,Y) position, in inches"
    
    # Maybe we don't have this layer
    if not self.hasLayer(layername): return             #### ADS END

    # First convert given inches to 2.5 co-ordinates
    X = int(round(Xoff/0.00001))
    Y = int(round(Yoff/0.00001))

    # Now calculate displacement for each position so that we end up at specified origin
    DX = X - self.minx
    DY = Y - self.miny

    # Rock and roll. First, write out a dummy flash using code D02
    # (exposure off). This prevents an unintentional draw from the end
    # of one job to the beginning of the next when a layer is repeated
    # due to panelizing.
    fid.write('X%07dY%07dD02*\n' % (X, Y))
    for cmd in self.commands[layername]:
      if type(cmd) is types.TupleType:
        if len(cmd)==3:
          x, y, d = cmd
          fid.write('X%07dY%07dD%02d*\n' % (x+DX, y+DY, d))
        else:
          x, y, I, J, d, s = cmd
          fid.write('X%07dY%07dI%07dJ%07dD%02d*\n' % (x+DX, y+DY, I, J, d)) # I,J are relative
      else:
        # It's an aperture change, G-code, or RS274-X command that begins with '%'. If
        # it's an aperture code, the aperture has already been translated
        # to the global aperture table during the parse phase.
        if cmd[0]=='%':
          fid.write('%s\n' % cmd)  # The command already has a * in it (e.g., "%LPD*%")
        else:
          fid.write('%s*\n' % cmd)

  def findTools(self, diameter):
    "Find the tools, if any, with the given diameter in inches. There may be more than one!"
    L = []
    for tool, diam in self.xdiam.items():
      if diam==diameter:
        L.append(tool)
    return L

  def writeExcellon(self, fid, diameter, Xoff, Yoff):
    "Write out the data such that the lower-left corner of this job is at the given (X,Y) position, in inches"
    
    # First convert given inches to 2.4 co-ordinates. Note that Gerber is 2.5 (as of GerbMerge 1.2)
    # and our internal Excellon representation is 2.4 as of GerbMerge
    # version 0.91. We use X,Y to calculate DX,DY in 2.4 units (i.e., with a
    # resolution of 0.0001".
    X = int(round(Xoff/0.00001))  # First work in 2.5 format to match Gerber
    Y = int(round(Yoff/0.00001))

    # Now calculate displacement for each position so that we end up at specified origin
    DX = X - self.minx
    DY = Y - self.miny

    # Now round down to 2.4 format
    DX = int(round(DX/10.0))
    DY = int(round(DY/10.0))

    ltools = self.findTools(diameter)

    if config.Config['excellonleadingzeros']:
      fmtstr = 'X%06dY%06d\n'
    else:
      fmtstr = 'X%dY%d\n'

    # Boogie
    for ltool in ltools:
      if self.xcommands.has_key(ltool):
        for cmd in self.xcommands[ltool]:
          x, y = cmd
          fid.write(fmtstr % (x+DX, y+DY))

  def writeDrillHits(self, fid, diameter, toolNum, Xoff, Yoff):
    """Write a drill hit pattern. diameter is tool diameter in inches, while toolNum is
    an integer index into strokes.DrillStrokeList"""

    # First convert given inches to 2.5 co-ordinates
    X = int(round(Xoff/0.00001))
    Y = int(round(Yoff/0.00001))

    # Now calculate displacement for each position so that we end up at specified origin
    DX = X - self.minx
    DY = Y - self.miny

    # Do NOT round down to 2.4 format. These drill hits are in Gerber 2.5 format, not
    # Excellon plunge commands.

    ltools = self.findTools(diameter)

    for ltool in ltools:
      if self.xcommands.has_key(ltool):
        for cmd in self.xcommands[ltool]:
          x, y = cmd
          makestroke.drawDrillHit(fid, 10*x+DX, 10*y+DY, toolNum)

  def aperturesAndMacros(self, layername):
    "Return dictionaries whose keys are all necessary aperture names and macro names for this layer"

    GAT=config.GAT

    if self.apertures.has_key(layername):
      apdict = {}.fromkeys(self.apertures[layername])
      apmlist = [GAT[ap].dimx for ap in self.apertures[layername] if GAT[ap].apname=='Macro']
      apmdict = {}.fromkeys(apmlist)
      
      return apdict, apmdict
    else:
      return {}, {}

  def makeLocalApertureCode(self, layername, AP):
    "Find or create a layer-specific aperture code to represent the global aperture given"
    if AP.code not in self.apxlat[layername].values():
      lastCode = aptable.findHighestApertureCode(self.apxlat[layername].keys())
      localCode = 'D%d' % (lastCode+1)
      self.apxlat[layername][localCode] = AP.code

  def inBorders(self, x, y):
    return (x >= self.minx) and (x <= self.maxx) and (y >= self.miny) and (y <= self.maxy)

  def trimGerberLayer(self, layername):
    "Modify drawing commands that are outside job dimensions"

    newcmds = []
    lastInBorders = True
    lastx, lasty, lastd = self.minx, self.miny, 2   # (minx,miny,exposure off)
    bordersRect = (self.minx, self.miny, self.maxx, self.maxy)
    lastAperture = None

    for cmd in self.commands[layername]:
      if type(cmd) == types.TupleType:
        # It is a data command: tuple (X, Y, D), all integers, or (X, Y, I, J, D), all integers.
        if len(cmd)==3:
          x, y, d = cmd
          # I=J=None   # In case we support circular interpolation in the future
        else:
          # We don't do anything with circular interpolation for now, so just issue
          # the command and be done with it.
          # x, y, I, J, d, s = cmd
          newcmds.append(cmd)
          continue

        newInBorders = self.inBorders(x,y)

        # Flash commands are easy (for now). If they're outside borders,
        # ignore them. There's no need to consider the previous command.
        # What should we do if the flash is partially inside and partially
        # outside the border? Ideally, define a macro that constructs the
        # part of the flash that is inside the border. Practically, you've
        # got to be kidding.
        #
        # Actually, it's not that tough for rectangle apertures. We identify
        # the intersection rectangle of the aperture and the bounding box,
        # determine the new rectangular aperture required along with the
        # new flash point, add the aperture to the GAT if necessary, and
        # make the change. Spiffy.
        #
        # For circular interpolation commands, it's definitely harder since
        # we have to construct arcs that are a subset of the original arc.
        # 
        # For polygon fills, we similarly have to break up the polygon into
        # sub-polygons that are contained within the allowable extents.
        #
        # Both circular interpolation and polygon fills are a) uncommon,
        # and b) hard to handle. The current version of GerbMerge does not
        # handle these cases.
        if d==3:
          if lastAperture.isRectangle():
            apertureRect = lastAperture.rectangleAsRect(x,y)
            if geometry.isRect1InRect2(apertureRect, bordersRect):
              newcmds.append(cmd)
            else:
              newRect = geometry.intersectExtents(apertureRect, bordersRect)

              if newRect:
                newRectWidth = geometry.rectWidth(newRect)
                newRectHeight = geometry.rectHeight(newRect)
                newX, newY = geometry.rectCenter(newRect)

                # We arbitrarily remove all flashes that lead to rectangles
                # with a width or length less than 1 mil (10 Gerber units).
                # Should we make this configurable?
                if min(newRectWidth, newRectHeight) >= 10:
                  # Construct an Aperture that is a Rectangle of dimensions (newRectWidth,newRectHeight)
                  newAP = aptable.Aperture(aptable.Rectangle, 'D??', \
                            util.gerb2in(newRectWidth), util.gerb2in(newRectHeight))
                  global_code = aptable.findOrAddAperture(newAP)

                  # We need an unused local aperture code to correspond to this newly-created global one.
                  self.makeLocalApertureCode(layername, newAP)

                  # Make sure to indicate that the new aperture is one that is used by this layer
                  if global_code not in self.apertures[layername]:
                    self.apertures[layername].append(global_code)

                  # Switch to new aperture code, flash new aperture, switch back to previous aperture code
                  newcmds.append(global_code)
                  newcmds.append((newX, newY, 3))
                  newcmds.append(lastAperture.code)
                else:
                  pass    # Ignore this flash...area in common is too thin
              else:
                pass      # Ignore this flash...no area in common
          elif self.inBorders(x, y):
            # Aperture is not a rectangle and its center is somewhere within our
            # borders. Flash it and ignore part outside borders (for now).
            newcmds.append(cmd)
          else:
            pass    # Ignore this flash

        # If this is a exposure off command, then it doesn't matter what the
        # previous command is. This command just updates the (X,Y) position
        # and sets the start point for a line draw to a new location.
        elif d==2:
          if self.inBorders(x, y):
            newcmds.append(cmd)

        else:
          # This is an exposure on (draw line) command. Now things get interesting.
          # Regardless of what the last command was (draw, exposure off, flash), we
          # are planning on drawing a visible line using the current aperture from
          # the (lastx,lasty) position to the new (x,y) position. The cases are:
          #   A: (lastx,lasty) is outside borders, (x,y) is outside borders.
          #      (lastx,lasty) have already been eliminated. Just update (lastx,lasty)
          #      with new (x,y) and remove the new command too. There is one case which
          #      may be of concern, and that is when the line defined by (lastx,lasty)-(x,y)
          #      actually crosses through the job. In this case, we have to draw the
          #      partial line (x1,y1)-(x2,y2) where (x1,y1) and (x2,y2) lie on the
          #      borders. We will add 3 commands:
          #           X(x1)Y(y1)D02   # exposure off
          #           X(x2)Y(y2)D01   # exposure on
          #           X(x)Y(y)D02     # exposure off
          #
          #   B: (lastx,lasty) is outside borders, (x,y) is inside borders.
          #      We have to find the intersection of the line (lastx,lasty)-(x,y)
          #      with the borders and draw only the line segment (x1,y1)-(x,y):
          #           X(x1)Y(y1)D02   # exposure off
          #           X(x)Y(y)D01     # exposure on
          #
          #   C: (lastx,lasty) is inside borders, (x,y) is outside borders.
          #      We have to find the intersection of the line (lastx,lasty)-(x,y)
          #      with the borders and draw only the line segment (lastx,lasty)-(x1,y1):
          #      then update to the new position:
          #           X(x1)Y(y1)D01   # exposure on
          #           X(x)Y(y)D02     # exposure off
          #
          #   D: (lastx,lasty) is inside borders, (x,y) is inside borders. This is
          #      the most common and simplest case...just copy the command over:
          #           X(x)Y(y)D01     # exposure on
          #
          # All of the above are for linear interpolation. Circular interpolation
          # is ignored for now.
          if lastInBorders and newInBorders:    # Case D
            newcmds.append(cmd)

          else:
            # segmentXbox() returns a list of 0, 1, or 2 points describing the intersection
            # points of the segment (lastx,lasty)-(x,y) with the box defined
            # by lower-left corner (minx,miny) and upper-right corner (maxx,maxy).
            pointsL = geometry.segmentXbox((lastx,lasty), (x,y), (self.minx,self.miny), (self.maxx,self.maxy))

            if len(pointsL)==0:   # Case A, no intersection
              # Both points are outside the box and there is no overlap with box.
              d = 2   # Command is effectively removed since newcmds wasn't extended.
                      # Ensure "last command" is exposure off to reflect this.

            elif len(pointsL)==1:     # Cases B and C
              pt1 = pointsL[0]
              if newInBorders:      # Case B
                newcmds.append((pt1[0], pt1[1], 2))   # Go to intersection point, exposure off
                newcmds.append(cmd)                   # Go to destination point, exposure on
              else:                 # Case C
                newcmds.append((pt1[0], pt1[1], 1))   # Go to intersection point, exposure on
                newcmds.append((x, y, 2))             # Go to destination point, exposure off
                d = 2                                 # Make next 'lastd' represent exposure off

            else:                 # Case A, two points of intersection
              pt1 = pointsL[0]
              pt2 = pointsL[1]

              newcmds.append((pt1[0], pt1[1], 2))   # Go to first intersection point, exposure off
              newcmds.append((pt2[0], pt2[1], 1))   # Draw to second intersection point, exposure on
              newcmds.append((x, y, 2))             # Go to destination point, exposure off
              d = 2                                 # Make next 'lastd' represent exposure off

        lastx, lasty, lastd = x, y, d
        lastInBorders = newInBorders
      else:
        # It's a string indicating an aperture change, G-code, or RS-274X
        # command (e.g., "D13", "G75", "%LPD*%")
        newcmds.append(cmd)
        if cmd[0]=='D' and int(cmd[1:])>=10:  # Don't interpret D01, D02, D03
          lastAperture = config.GAT[cmd]

    self.commands[layername] = newcmds

  def trimGerber(self):
    for layername in self.commands.keys():
      self.trimGerberLayer(layername)

  def trimExcellon(self):
    "Remove plunge commands that are outside job dimensions"
    keys = self.xcommands.keys()
    for toolname in keys:
      # Remember Excellon is 2.4 format while Gerber data is 2.5 format
      validList = [(x,y) for x,y in self.xcommands[toolname] if self.inBorders(10*x,10*y)]

      if validList:
        self.xcommands[toolname] = validList
      else:
        del self.xcommands[toolname]
        del self.xdiam[toolname]

# This class encapsulates a Job object, providing absolute
# positioning information.
class JobLayout:
  def __init__(self, job):
    self.job = job
    self.x = None
    self.y = None

  def canonicalize(self):       # Must return a JobLayout object as a list
    return [self]

  def writeGerber(self, fid, layername):
    assert self.x is not None
    self.job.writeGerber(fid, layername, self.x, self.y)

  def aperturesAndMacros(self, layername):
    return self.job.aperturesAndMacros(layername)

  def writeExcellon(self, fid, diameter):
    assert self.x is not None
    self.job.writeExcellon(fid, diameter, self.x, self.y)

  def writeDrillHits(self, fid, diameter, toolNum):
    assert self.x is not None
    self.job.writeDrillHits(fid, diameter, toolNum, self.x, self.y)

  def writeCutLines(self, fid, drawing_code, X1, Y1, X2, Y2):
    """Draw a board outline using the given aperture code"""
    def notEdge(x, X):
      return round(abs(1000*(x-X)))

    assert self.x and self.y

    radius = config.GAT[drawing_code].dimx/2.0
    
    # Start at lower-left, proceed clockwise
    x = self.x - radius
    y = self.y - radius

    left = notEdge(self.x, X1)
    right = notEdge(self.x+self.width_in(), X2)
    bot = notEdge(self.y, Y1)
    top = notEdge(self.y+self.height_in(), Y2)

    BL = ((x), (y))
    TL = ((x), (y+self.height_in()+2*radius))
    TR = ((x+self.width_in()+2*radius), (y+self.height_in()+2*radius))
    BR = ((x+self.width_in()+2*radius), (y))

    if not left:
      BL = (BL[0]+2*radius, BL[1])
      TL = (TL[0]+2*radius, TL[1])

    if not top:
      TL = (TL[0], TL[1]-2*radius)
      TR = (TR[0], TR[1]-2*radius)

    if not right:
      TR = (TR[0]-2*radius, TR[1])
      BR = (BR[0]-2*radius, BR[1])

    if not bot:
      BL = (BL[0], BL[1]+2*radius)
      BR = (BR[0], BR[1]+2*radius)

    BL = (util.in2gerb(BL[0]), util.in2gerb(BL[1]))
    TL = (util.in2gerb(TL[0]), util.in2gerb(TL[1]))
    TR = (util.in2gerb(TR[0]), util.in2gerb(TR[1]))
    BR = (util.in2gerb(BR[0]), util.in2gerb(BR[1]))

    # The "if 1 or ..." construct draws all four sides of the job. By
    # removing the 1 from the expression, only the sides that do not
    # correspond to panel edges are drawn. The former is probably better
    # since panels tend to have a little slop from the cutting operation
    # and it's easier to just cut it smaller when there's a cut line.
    # The way it is now with "if 1 or....", much of this function is
    # unnecessary. Heck, we could even just use the boardoutline layer
    # directly.
    if 1 or left:
      fid.write('X%07dY%07dD02*\n' % BL)
      fid.write('X%07dY%07dD01*\n' % TL)

    if 1 or top:
      if not left: fid.write('X%07dY%07dD02*\n' % TL)
      fid.write('X%07dY%07dD01*\n' % TR)

    if 1 or right:
      if not top: fid.write('X%07dY%07dD02*\n' % TR)
      fid.write('X%07dY%07dD01*\n' % BR)

    if 1 or bot:
      if not right: fid.write('X%07dY%07dD02*\n' % BR)
      fid.write('X%07dY%07dD01*\n' % BL)

  def setPosition(self, x, y):
    self.x=x
    self.y=y

  def width_in(self):
    return self.job.width_in()

  def height_in(self):
    return self.job.height_in()

  def drillhits(self, diameter):
    tools = self.job.findTools(diameter)
    total = 0
    for tool in tools:
      try:
        total += len(self.job.xcommands[tool])
      except:
        pass

    return total

  def jobarea(self):
    return self.job.jobarea()

def rotateJob(job, degrees = 90, firstpass = True):
  """Create a new job from an existing one, rotating by specified degrees in 90 degree passes"""
  GAT = config.GAT
  GAMT = config.GAMT
  ##print "rotating job:", job.name, degrees, firstpass
  if firstpass:
    if degrees == 270:
        J = Job(job.name+'*rotated270')
    elif degrees == 180:
        J = Job(job.name+'*rotated180')
    else:
        J = Job(job.name+'*rotated90')
  else:
    J = Job(job.name)

  # Keep the origin (lower-left) in the same place
  J.maxx = job.minx + job.maxy-job.miny
  J.maxy = job.miny + job.maxx-job.minx
  J.minx = job.minx
  J.miny = job.miny

  RevGAT = config.buildRevDict(GAT)   # RevGAT[hash] = aperturename
  RevGAMT = config.buildRevDict(GAMT) # RevGAMT[hash] = aperturemacroname

  # Keep list of tool diameters and default tool list
  J.xdiam = job.xdiam
  J.ToolList = job.ToolList
  J.Repeat = job.Repeat

  # D-code translation table is the same, except we have to rotate
  # those apertures which have an orientation: rectangles, ovals, and macros.

  ToolChangeReplace = {}
  for layername in job.apxlat.keys():
    J.apxlat[layername] = {}

    for ap in job.apxlat[layername].keys():
      code = job.apxlat[layername][ap]
      A = GAT[code]

      if A.apname in ('Circle', 'Octagon'):
        # This aperture is fine. Copy it over.
        J.apxlat[layername][ap] = code
        continue

      # Must rotate the aperture
      APR = A.rotated(RevGAMT)

      # Does it already exist in the GAT?
      hash = APR.hash()
      try:
        # Yup...add it to apxlat
        newcode = RevGAT[hash]
      except KeyError:
        # Must add new aperture to GAT
        newcode = aptable.addToApertureTable(APR)

        # Rebuild RevGAT
        #RevGAT = config.buildRevDict(GAT)
        RevGAT[hash] = newcode

      J.apxlat[layername][ap] = newcode

      # Must also replace all tool change commands from
      # old code to new command.
      ToolChangeReplace[code] = newcode

  # Now we copy commands, rotating X,Y positions.
  # Rotations will occur counterclockwise about the
  # point (minx,miny). Then, we shift to the right
  # by the height so that the lower-left point of
  # the rotated job continues to be (minx,miny).
  #
  # We also have to take aperture change commands and
  # replace them with the new aperture code if we have
  # a rotation.
  offset = job.maxy-job.miny
  for layername in job.commands.keys():
    J.commands[layername] = []
    J.apertures[layername] = []

    for cmd in job.commands[layername]:
      # Is it a drawing command?
      if type(cmd) is types.TupleType:
        if len(cmd)==3:
          x, y, d = map(__builtin__.int, cmd)
          II=JJ=None
        else:
          x, y, II, JJ, d, signed = map(__builtin__.int, cmd)   # J is already used as Job object
      else:
        # No, must be a string indicating aperture change, G-code, or RS274-X command.
        if cmd[0] in ('G', '%'):
          # G-codes and RS274-X commands are just copied verbatim and not affected by rotation
          J.commands[layername].append(cmd)
          continue

        # It's a D-code. See if we need to replace aperture changes with a rotated aperture.
        # But only for D-codes >= 10.
        if int(cmd[1:]) < 10:
          J.commands[layername].append(cmd)
          continue

        try:
          newcmd = ToolChangeReplace[cmd]
          J.commands[layername].append(newcmd)
          J.apertures[layername].append(newcmd)
        except KeyError:
          J.commands[layername].append(cmd)
          J.apertures[layername].append(cmd)
        continue

      # (X,Y) --> (-Y,X) effects a 90-degree counterclockwise shift
      # Adding 'offset' to -Y maintains the lower-left origin of (minx,miny).
      newx = -(y - job.miny) + job.minx + offset
      newy = (x-job.minx) + job.miny

      # For circular interpolation commands, (I,J) components are always relative
      # so we do not worry about offsets, just reverse their sense, i.e., I becomes J
      # and J becomes I. For 360-degree circular interpolation, I/J are signed and we
      # must map (I,J) --> (-J,I).
      if II is not None:
        if signed:
          J.commands[layername].append((newx, newy, -JJ, II, d, signed))
        else:
          J.commands[layername].append((newx, newy, JJ, II, d, signed))
      else:
        J.commands[layername].append((newx,newy,d))

    if 0:
      print job.minx, job.miny, offset
      print layername
      print J.commands[layername]

  # Finally, rotate drills. Offset is in hundred-thousandths (2.5) while Excellon
  # data is in 2.4 format.
  for tool in job.xcommands.keys():
    J.xcommands[tool] = []

    for x,y in job.xcommands[tool]:
      newx = -(10*y - job.miny) + job.minx + offset
      newy =  (10*x - job.minx) + job.miny

      newx = int(round(newx/10.0))
      newy = int(round(newy/10.0))

      J.xcommands[tool].append((newx,newy))

  # Rotate some more if required
  degrees -= 90
  if degrees > 0:
    return rotateJob(J, degrees, False)
  else:
    ##print "rotated:", J.name
    return J
