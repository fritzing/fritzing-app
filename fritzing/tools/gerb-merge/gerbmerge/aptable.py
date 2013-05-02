#!/usr/bin/env python
"""
Manage apertures, read aperture table, etc.

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

import config
import amacro
import util

# Recognized apertures and re pattern that matches its definition Thermals and
# annuli are generated using macros (see the eagle.def file) but only on inner
# layers. Octagons are also generated as macros (%AMOC8) but we handle these
# specially as the Eagle macro uses a replaceable macro parameter ($1) and
# GerbMerge doesn't handle these yet...only fixed macros (no parameters) are
# currently supported.
Apertures = (
   ('Rectangle', re.compile(r'^%AD(D\d+)R,([^X]+)X([^*]+)\*%$'), '%%AD%sR,%.5fX%.5f*%%\n'),
   ('Circle',    re.compile(r'^%AD(D\d+)C,([^*]+)\*%$'),         '%%AD%sC,%.5f*%%\n'),
   ('Oval',      re.compile(r'^%AD(D\d+)O,([^X]+)X([^*]+)\*%$'), '%%AD%sO,%.5fX%.5f*%%\n'),
   ('Octagon',   re.compile(r'^%AD(D\d+)OC8,([^*]+)\*%$'),       '%%AD%sOC8,%.5f*%%\n'),     # Specific to Eagle
   ('Macro',     re.compile(r'^%AD(D\d+)([^*]+)\*%$'),           '%%AD%s%s*%%\n')
  )

# This loop defines names in this module like 'Rectangle',
# which are element 0 of the Apertures list above. So code
# will be like:
#       import aptable
#       A = aptable.Aperture(aptable.Rectangle, ......)

for ap in Apertures:
  globals()[ap[0]] = ap

class Aperture:
  def __init__(self, aptype, code, dimx, dimy=None):
    assert aptype in Apertures
    self.apname, self.pat, self.format = aptype
    self.code = code
    self.dimx = dimx      # Macro name for Macro apertures
    self.dimy = dimy      # None for Macro apertures

    if self.apname in ('Circle', 'Octagon', 'Macro'):
      assert (dimy is None)
        
  def isRectangle(self):
    return self.apname == 'Rectangle'

  def rectangleAsRect(self, X, Y):
    """Return a 4-tuple (minx,miny,maxx,maxy) describing the area covered by
    this Rectangle aperture when flashed at center co-ordinates (X,Y)"""
    dx = util.in2gerb(self.dimx)
    dy = util.in2gerb(self.dimy)

    if dx & 1:    # Odd-sized: X extents are (dx+1)/2 on the left and (dx-1)/2 on the right
      xm = (dx+1)/2
      xp = xm-1
    else:         # Even-sized: X extents are X-dx/2 and X+dx/2
      xm = xp = dx/2

    if dy & 1:    # Odd-sized: Y extents are (dy+1)/2 below and (dy-1)/2 above
      ym = (dy+1)/2
      yp = ym-1
    else:         # Even-sized: Y extents are Y-dy/2 and Y+dy/2
      ym = yp = dy/2

    return (X-xm, Y-ym, X+xp, Y+yp)
    
  def getAdjusted(self, minimum):
    """
      Adjust aperture properties to conform to minimum feature dimensions
      Return new aperture if required, else return False
    """
    dimx = dimy = None
   
    # Check for X and Y dimensions less than minimum
    if (self.dimx != None) and (self.dimx < minimum):
        dimx = minimum
    if (self.dimy != None) and (self.dimx < minimum):
        dimy = minimum   
    
    # Return new aperture if needed
    if (dimx != None) or (dimy != None):
      if dimx==None: dimx=self.dimx
      if dimy==None: dimy=self.dimy
      return Aperture( (self.apname, self.pat, self.format), self.code, dimx, dimy )
    else:
      return False ## no new aperture needs to be created

  def rotate(self, RevGAMT):
    if self.apname in ('Macro',):
      # Construct a rotated macro, see if it's in the GAMT, and set self.dimx
      # to its name if so. If not, add the rotated macro to the GAMT and set
      # self.dimx to the new name. Recall that GAMT maps name to macro
      # (e.g., GAMT['M9'] = ApertureMacro(...)) while RevGAMT maps hash to
      # macro name (e.g., RevGAMT[hash] = 'M9')
      AMR = config.GAMT[self.dimx].rotated()
      hash = AMR.hash()
      try:
        self.dimx = RevGAMT[hash]
      except KeyError:
        AMR = amacro.addToApertureMacroTable(AMR)   # adds to GAMT and modifies name to global name
        self.dimx = RevGAMT[hash] = AMR.name

    elif self.dimy is not None:       # Rectangles and Ovals have a dimy setting and need to be rotated
      t = self.dimx
      self.dimx = self.dimy
      self.dimy = t

  def rotated(self, RevGAMT):
    # deepcopy doesn't work on re patterns for some reason so we copy ourselves manually
    APR = Aperture((self.apname, self.pat, self.format), self.code, self.dimx, self.dimy)
    APR.rotate(RevGAMT)
    return APR

  def dump(self, fid=sys.stdout):
    fid.write(str(self))

  def __str__(self):
    return '%s: %s' % (self.code, self.hash())
    #if 0:
    #  if self.dimy:
    #    return ('%s: %s (%.4f x %.4f)' % (self.code, self.apname, self.dimx, self.dimy))
    #  else:
    #    if self.apname in ('Macro'):
    #      return ('%s: %s (%s)' % (self.code, self.apname, self.dimx))
    #    else:
    #      return ('%s: %s (%.4f)' % (self.code, self.apname, self.dimx))

  def hash(self):
    if self.dimy:
      return ('%s (%.5f x %.5f)' % (self.apname, self.dimx, self.dimy))
    else:
      if self.apname in ('Macro',):
        return ('%s (%s)' % (self.apname, self.dimx))
      else:
        return ('%s (%.5f)' % (self.apname, self.dimx))

  def writeDef(self, fid):
    if self.dimy:
      fid.write(self.format % (self.code, self.dimx, self.dimy))
    else:
      fid.write(self.format % (self.code, self.dimx))

# Parse the aperture definition in line 's'. macroNames is an aperture macro dictionary
# that translates macro names local to this file to global names in the GAMT. We make
# the translation right away so that the return value from this function is an aperture
# definition with a global macro name, e.g., 'ADD10M5'
def parseAperture(s, knownMacroNames):
  for ap in Apertures:
    match = ap[1].match(s)
    if match:
      dimy = None
      if ap[0] in ('Circle', 'Octagon', 'Macro'):
        code, dimx = match.groups()
      else:
        code, dimx, dimy = match.groups()

      if ap[0] in ('Macro',):
        if knownMacroNames.has_key(dimx):
          dimx = knownMacroNames[dimx]    # dimx is now GLOBAL, permanent macro name (e.g., 'M2')
        else:
          raise RuntimeError, 'Aperture Macro name "%s" not defined' % dimx
      else:
        try:
          dimx = float(dimx)
          if dimy:
            dimy = float(dimy)
        except:
          raise RuntimeError, "Illegal floating point aperture size"

      return Aperture(ap, code, dimx, dimy)

  return None

# This function returns a dictionary where each key is an
# aperture code string (e.g., "D11") and the value is the
# Aperture object that represents it. For example:
#
#    %ADD12R,0.0630X0.0630*%
#
# from a Gerber file would result in the dictionary entry:
#
#    "D12": Aperture(ap, 'D10', 0.063, 0.063)
#
# The input fileList is a list of pathnames which will be read to construct the
# aperture table for a job.  All the files in the given list will be so
# examined, and a global aperture table will be constructed as a dictionary.
# Same goes for the global aperture macro table.

tool_pat  = re.compile(r'^(?:G54)?D\d+\*$')

def constructApertureTable(fileList):
  # First we construct a dictionary where each key is the
  # string representation of the aperture. Then we go back and assign
  # numbers. For aperture macros, we construct their final version
  # (i.e., 'M1', 'M2', etc.) right away, as they are parsed. Thus,
  # we translate from 'THX10N' or whatever to 'M2' right away.
  GAT = config.GAT      # Global Aperture Table
  GAT.clear()
  GAMT = config.GAMT    # Global Aperture Macro Table
  GAMT.clear()
  RevGAMT = {}          # Dictionary keyed by aperture macro hash and returning macro name

  AT = {}               # Aperture Table for this file
  for fname in fileList:
    #print 'Reading apertures from %s ...' % fname

    knownMacroNames = {}

    fid = file(fname,'rt')
    for line in fid:
      # Get rid of CR
      line = line.replace('\x0D', '')

      if tool_pat.match(line):
        break # When tools start, no more apertures are being defined

      # If this is an aperture macro definition, add its string
      # representation to the dictionary. It might already exist.
      # Ignore %AMOC8* from Eagle for now as it uses a macro parameter.
      if line[:7]=='%AMOC8*':
        continue

      # parseApertureMacro() sucks up all macro lines up to terminating '%'
      AM = amacro.parseApertureMacro(line, fid)
      if AM:
        # Has this macro definition already been defined (perhaps by another name
        # in another layer)?
        try:
          # If this macro has already been encountered anywhere in any job,
          # RevGAMT will map the macro hash to the global macro name. Then,
          # make the local association knownMacroNames[localMacroName] = globalMacroName.
          knownMacroNames[AM.name] = RevGAMT[AM.hash()]
        except KeyError:
          # No, so define the global macro and do the translation. Note that
          # addToApertureMacroTable() MODIFIES AM.name to the new M-name.
          localMacroName = AM.name
          AM = amacro.addToApertureMacroTable(AM)
          knownMacroNames[localMacroName] = AM.name
          RevGAMT[AM.hash()] = AM.name
      else:
        A = parseAperture(line, knownMacroNames)

        # If this is an aperture definition, add the string representation
        # to the dictionary. It might already exist.
        if A:
          AT[A.hash()] = A

    fid.close()

  # Now, go through and assign sequential codes to all apertures
  code = 10
  for val in AT.values():
    key = 'D%d' % code
    GAT[key] = val
    val.code = key
    code += 1

  if 0:
    keylist = config.GAT.keys()
    keylist.sort()
    print 'Apertures'
    print '========='
    for key in keylist:
      print '%s' % config.GAT[key]
    sys.exit(0)

def findHighestApertureCode(keys):
  "Find the highest integer value in a list of aperture codes: ['D10', 'D23', 'D35', ...]"

  # Must sort keys by integer value, not string since 99 comes before 100
  # as an integer but not a string.
  keys = [int(K[1:]) for K in keys]
  keys.sort()

  return keys[-1]

def addToApertureTable(AP):
  GAT = config.GAT

  lastCode = findHighestApertureCode(GAT.keys())
  code = 'D%d' % (lastCode+1)
  GAT[code] = AP
  AP.code = code

  return code
  
def findInApertureTable(AP):
  """Return 'D10', for example in response to query for an object
     of type Aperture()"""
  hash = AP.hash()
  for key, val in config.GAT.items():
    if hash==val.hash():
      return key

  return None

def findOrAddAperture(AP):
  """If the aperture exists in the GAT, modify the AP.code field to reflect the global code
  and return the code. Otherwise, create a new aperture in the GAT and return the new code
  for it."""
  code = findInApertureTable(AP)
  if code:
    AP.code = code
    return code
  else:
    return addToApertureTable(AP)

if __name__=="__main__":
  constructApertureTable(sys.argv[1:])

  keylist = config.GAMT.keys()
  keylist.sort()
  print 'Aperture Macros'
  print '==============='
  for key in keylist:
    print '%s' % config.GAMT[key]

  keylist = config.GAT.keys()
  keylist.sort()
  print 'Apertures'
  print '========='
  for key in keylist:
    print '%s' % config.GAT[key]
