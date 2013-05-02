#!/usr/bin/env python
"""
Define and manage aperture macros (%AM command). Currently,
only macros without replaceable parameters (e.g., $1, $2, etc.)
are supported.

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
import copy

import config

_macro_pat = re.compile(r'^%AM([^*]+)\*$')

# This list stores the expected types of parameters for each primitive type
# (e.g., outline, line, circle, polygon, etc.). None is used for undefined
# primitives. Each entry corresponds to the defined primitive code, and
# comprises a tuple of conversion functions (i.e., built-in int() and float()
# functions) that apply to all parameters AFTER the primitive code. For example,
# code 1 (circle) may be instantiated as:
#                         1,1,0.025,0.0,0.0
# (the parameters are code, exposure type, diameter, X center, Y center).
# After the integer code, we expect an int for exposure type, then floats
# for the remaining three parameters. Thus, the entry for code 1 is
# (int, float, float, float).
PrimitiveParmTypes = (
   None,                                            # Code 0  -- undefined
  (int, float, float, float),                       # Code 1  -- circle
  (int, float, float, float, float, float, float),  # Code 2  -- line (vector)
   None,                                            # Code 3  -- end-of-file for .DES files
  (int, int, float, float, float, float, float),    # Code 4  -- outline...takes any number of additional floats
  (int, int, float, float, float, float),           # Code 5  -- regular polygon
  (float, float, float, float, float, int, float, float, float),  # Code 6 -- moire
  (float, float, float, float, float, float),       # Code 7  -- thermal
   None,                                            # Code 8  -- undefined
   None,                                            # Code 9  -- undefined
   None,                                            # Code 10 -- undefined
   None,                                            # Code 11 -- undefined
   None,                                            # Code 12 -- undefined
   None,                                            # Code 13 -- undefined
   None,                                            # Code 14 -- undefined
   None,                                            # Code 15 -- undefined
   None,                                            # Code 16 -- undefined
   None,                                            # Code 17 -- undefined
   None,                                            # Code 18 -- undefined
   None,                                            # Code 19 -- undefined
  (int, float, float, float, float, float),         # Code 20 -- line (vector)...alias for code 2
  (int, float, float, float, float, float),         # Code 21 -- line (center)
  (int, float, float, float, float, float)          # Code 22 -- line (lower-left)
)  

def rotatexy(x,y):
  # Rotate point (x,y) counterclockwise 90 degrees about the origin
  return (-y,x)

def rotatexypair(L, ix):
  # Rotate list items L[ix],L[ix+1] by 90 degrees
  L[ix],L[ix+1] = rotatexy(L[ix],L[ix+1])

def swapxypair(L, ix):
  # Swap two list elements
  L[ix],L[ix+1] = L[ix+1],L[ix]

def rotatetheta(th):
  # Increase angle th in degrees by +90 degrees (counterclockwise).
  # Handle modulo 360 issues
  th += 90
  if th >= 360:
    th -= 360
  return th

def rotatethelem(L, ix):
  # Increase angle th by +90 degrees for a list element
  L[ix] = rotatetheta(L[ix])

class ApertureMacroPrimitive:
  def __init__(self, code=-1, fields=None):
    self.code = code
    self.parms = []
    if fields is not None:
      self.setFromFields(code, fields)

  def setFromFields(self, code, fields):
    # code is an integer describing the primitive type, and fields is
    # a list of STRINGS for each parameter
    self.code = code

    # valids will be one of the PrimitiveParmTypes tuples above. Some are
    # None to indicate illegal codes. We also set valids to None to indicate
    # the macro primitive code is outside the range of known types.
    try:
      valids = PrimitiveParmTypes[code]
    except:
      valids = None

    if valids is None:
      raise RuntimeError, 'Undefined aperture macro primitive code %d' % code

    # We expect exactly the number of fields required, except for macro
    # type 4 which is an outline and has a variable number of points.
    # For outlines, the second parameter indicates the number of points,
    # each of which has an (X,Y) co-ordinate. Thus, we expect an Outline
    # specification to have 1+1+2*N+1=3+2N fields:
    #   - first field is exposure
    #   - second field is number of points
    #   - 2*N fields for X,Y points
    #   - last field is rotation
    if self.code==4:
      if len(fields) < 2:
        raise RuntimeError, 'Outline macro primitive has way too few fields'

      try:
        N = int(fields[1])
      except:
        raise RuntimeError, 'Outline macro primitive has non-integer number of points'

      if len(fields) != (3+2*N):
        raise RuntimeError, 'Outline macro primitive has %d fields...expecting %d fields' % (len(fields), 3+2*N)
    else:
      if len(fields) != len(valids):
        raise RuntimeError, 'Macro primitive has %d fields...expecting %d fields' % (len(fields), len(valids))

    # Convert each parameter on the input line to an entry in the self.parms
    # list, using either int() or float() conversion.
    for parmix in range(len(fields)):
      try:
        converter = valids[parmix]
      except:
        converter = float     # To handle variable number of points in Outline type

      try:
        self.parms.append(converter(fields[parmix]))
      except:
        raise RuntimeError, 'Aperture macro primitive parameter %d has incorrect type' % (parmix+1)

  def setFromLine(self, line):      
    # Account for DOS line endings and get rid of line ending and '*' at the end
    line = line.replace('\x0D', '')
    line = line.rstrip()
    line = line.rstrip('*')

    fields = line.split(',')

    try:
      try:
        code = int(fields[0])
      except:
        raise RuntimeError, 'Illegal aperture macro primitive code "%s"' % fields[0]
      self.setFromFields(code, fields[1:])
    except:
      print '='*20
      print "==> ", line
      print '='*20
      raise

  def rotate(self):
    if self.code == 1:      # Circle: nothing to do
      pass
    elif self.code in (2,20): # Line (vector): fields (2,3) and (4,5) must be rotated, no need to
                              # rotate field 6
      rotatexypair(self.parms, 2)
      rotatexypair(self.parms, 4)
    elif self.code == 21:     # Line (center): fields (3,4) must be rotated, and field 5 incremented by +90
      rotatexypair(self.parms, 3)
      rotatethelem(self.parms, 5)
    elif self.code == 22:     # Line (lower-left): fields (3,4) must be rotated, and field 5 incremented by +90
      rotatexypair(self.parms, 3)
      rotatethelem(self.parms, 5)
    elif self.code == 4:      # Outline: fields (2,3), (4,5), etc. must be rotated, the last field need not be incremented
      ix = 2
      for pts in range(self.parms[1]):    # parms[1] is the number of points
        rotatexypair(self.parms, ix)
        ix += 2
      #rotatethelem(self.parms, ix)
    elif self.code == 5:      # Polygon: fields (2,3) must be rotated, and field 5 incremented by +90
      rotatexypair(self.parms, 2)
      rotatethelem(self.parms, 5)
    elif self.code == 6:      # Moire: fields (0,1) must be rotated, and field 8 incremented by +90
      rotatexypair(self.parms, 0)
      rotatethelem(self.parms, 8)
    elif self.code == 7:      # Thermal: fields (0,1) must be rotated, and field 5 incremented by +90
      rotatexypair(self.parms, 0)
      rotatethelem(self.parms, 5)

  def __str__(self):
    # Construct a string with ints as ints and floats as floats
    s = '%d' % self.code
    for parmix in range(len(self.parms)):
      valids = PrimitiveParmTypes[self.code]

      format = ',%f'
      try:
        if valids[parmix] is int:
          format = ',%d'
      except:
        pass    # '%f' is OK for Outline extra points
        
      s += format % self.parms[parmix]

    return s

  def writeDef(self, fid):
    fid.write('%s*\n' % str(self))

class ApertureMacro:
  def __init__(self, name):
    self.name = name
    self.prim = []
        
  def add(self, prim):
    self.prim.append(prim)

  def rotate(self):
    for prim in self.prim:
      prim.rotate()

  def rotated(self):
    # Return copy of ourselves, rotated. Replace 'R' as the first letter of the
    # macro name.  We don't append because we like to be able to count the
    # number of aperture macros by stripping off the leading character.
    M = copy.deepcopy(self)
    M.rotate()
    M.name = 'R'+M.name[1:]
    return M

  def dump(self, fid=sys.stdout):
    fid.write(str(self))

  def __str__(self):
    s = '%s:\n' % self.name
    s += self.hash()
    return s

  def hash(self):
    s = ''
    for prim in self.prim:
      s += '  '+str(prim)+'\n'
    return s

  def writeDef(self, fid):
    fid.write('%%AM%s*\n' % self.name)
    for prim in self.prim:
      prim.writeDef(fid)
    fid.write('%\n')

def parseApertureMacro(s, fid):
  match = _macro_pat.match(s)
  if match:
    name = match.group(1)

    M = ApertureMacro(name)

    for line in fid:
      if line[0]=='%':
        return M

      P = ApertureMacroPrimitive()
      P.setFromLine(line)

      M.add(P)
    else:
      raise RuntimeError, "Premature end-of-file while parsing aperture macro"
  else:
    return None

# This function adds the new aperture macro AM to the global aperture macro
# table. The return value is the modified macro (name modified to be its global
# name).  macro.
def addToApertureMacroTable(AM):
  GAMT = config.GAMT

  # Must sort keys by integer value, not string since 99 comes before 100
  # as an integer but not a string.
  keys = map(int, map(lambda K: K[1:], GAMT.keys()))
  keys.sort()

  if len(keys):
    lastCode = keys[-1]
  else:
    lastCode = 0

  mcode = 'M%d' % (lastCode+1)
  AM.name = mcode
  GAMT[mcode] = AM

  return AM

if __name__=="__main__":
  # Create a funky aperture macro with all the fixins, and make sure
  # it rotates properly.
  M = ApertureMacro('TEST')

  # X and Y axes
  M.add(ApertureMacroPrimitive(2, ('1', '0.0025', '0.0', '-0.1', '0.0', '0.1', '0.0')))
  M.add(ApertureMacroPrimitive(2, ('1', '0.0025', '0.0', '-0.1', '0.0', '0.1', '90.0')))

  # A circle in the top-right quadrant, touching the axes
  M.add(ApertureMacroPrimitive(1, ('1', '0.02', '0.01', '0.01')))
  # A line of slope -1 centered on the above circle, of thickness 5mil, length 0.05
  M.add(ApertureMacroPrimitive(2, ('1', '0.005', '0.0', '0.02', '0.02', '0.0', '0.0')))
  # A narrow vertical rectangle centered on the circle of width 2.5mil
  M.add(ApertureMacroPrimitive(21, ('1', '0.0025', '0.03', '0.01', '0.01', '0.0')))
  # A 45-degree line in the third quadrant, not quite touching the origin
  M.add(ApertureMacroPrimitive(22, ('1', '0.02', '0.01', '-0.03', '-0.03', '45')))
  # A right triangle in the second quadrant
  M.add(ApertureMacroPrimitive(4, ('1', '4', '-0.03', '0.01', '-0.03', '0.03', '-0.01', '0.01', '-0.03', '0.01', '0.0')))
  # A pentagon in the fourth quadrant, rotated by 15 degrees
  M.add(ApertureMacroPrimitive(5, ('1', '5', '0.03', '-0.03', '0.02', '15')))
  # A moire in the first quadrant, beyond the circle, with 2 annuli
  M.add(ApertureMacroPrimitive(6, ('0.07', '0.07', '0.04', '0.005', '0.01', '2', '0.005', '0.04', '0.0')))
  # A thermal in the second quadrant, beyond the right triangle
  M.add(ApertureMacroPrimitive(7, ('-0.07', '0.07', '0.03', '0.02', '0.005', '15')))

  MR = M.rotated()

  # Generate the Gerber so we can view it
  fid = file('amacro.ger', 'wt')
  print >> fid, \
"""G75*
G70*
%OFA0B0*%
%FSLAX24Y24*%
%IPPOS*%
%LPD*%"""
  M.writeDef(fid)
  MR.writeDef(fid)
  print >> fid, \
"""%ADD10TEST*%
%ADD11TESTR*%
D10*
X010000Y010000D03*
D11*
X015000Y010000D03*
M02*"""
  fid.close()

  print M
  print MR
