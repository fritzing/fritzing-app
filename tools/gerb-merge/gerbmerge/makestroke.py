#!/usr/bin/env python
"""Support for writing characters and graphics to Gerber files
--------------------------------------------------------------------

This program is licensed under the GNU General Public License (GPL).
See http://www.fsf.org for details of the license.

Andrew Sterian
Padnos College of Engineering and Computing
Grand Valley State University
<steriana@claymore.engineer.gvsu.edu>
<http://claymore.engineer.gvsu.edu/~steriana>
"""

import math

import strokes

# Define percentage of cell height and width to determine
# intercharacter spacing
SpacingX = 1.20
SpacingY = 1.20

# Arrow dimensions
BarLength       = 1500 # Length of dimension line
ArrowWidth      = 750  # How broad the arrow is
ArrowLength     = 750  # How far back from dimension line it is
ArrowStemLength = 1250 # How long the arrow stem extends from center point

#################################################################

# Arrow directions
FacingLeft=0    # 0 degrees
FacingDown=1    # 90 degrees counterclockwise
FacingRight=2   # 180 degrees
FacingUp=3    # 270 degrees

SpacingDX = 10*int(round(strokes.MaxWidth*SpacingX))
SpacingDY = 10*int(round(strokes.MaxHeight*SpacingY))
  
RotatedGlyphs={}

# Default arrow glyph is at 0 degrees rotation, facing left
ArrowGlyph = [ [(0,-BarLength/2), (0, BarLength/2)],
               [(ArrowLength,ArrowWidth/2), (0,0), (ArrowLength,-ArrowWidth/2)],
               [(0,0), (ArrowStemLength,0)]
             ]

def rotateGlyph(glyph, degrees, glyphName):
  """Rotate a glyph counterclockwise by given number of degrees. The glyph
  is a list of lists, where each sub-list is a connected path."""
  try:
    return RotatedGlyphs["%.1f_%s" % (degrees, glyphName)]
  except KeyError:
    pass # Not cached yet

  rad = degrees/180.0*math.pi
  cosx = math.cos(rad)
  sinx = math.sin(rad)

  newglyph = []
  for path in glyph:
    newpath = []
    for X,Y in path:
      x = int(round(X*cosx - Y*sinx))
      y = int(round(X*sinx + Y*cosx))
      newpath.append((x,y))
    newglyph.append(newpath)

  RotatedGlyphs["%.1f_%s" % (degrees, glyphName)] = newglyph
  return newglyph

def writeFlash(fid, X, Y, D):
  fid.write("X%07dY%07dD%02d*\n" % (X,Y,D))

def drawPolyline(fid, L, offX, offY, scale=1):
  for ix in range(len(L)):
    X,Y = L[ix]
    X *= scale
    Y *= scale
    if ix==0:
      writeFlash(fid, X+offX, Y+offY, 2)
    else:
      writeFlash(fid, X+offX, Y+offY, 1)
    
def writeGlyph(fid, glyph, X, Y, degrees, glyphName=None):
  if not glyphName:
    glyphName = str(glyph)

  for path in rotateGlyph(glyph, degrees, glyphName):
    drawPolyline(fid, path, X, Y, 10)

def writeChar(fid, c, X, Y, degrees):  
  if c==' ': return

  try:
    glyph = strokes.StrokeMap[c]
  except:
    raise RuntimeError, 'No glyph for character %s' % hex(ord(c))

  writeGlyph(fid, glyph, X, Y, degrees, c)

def writeString(fid, s, X, Y, degrees):
  posX = X
  posY = Y
  rad = degrees/180.0*math.pi
  dX = int(round(math.cos(rad)*SpacingDX))
  dY = int(round(math.sin(rad)*SpacingDX))

  if 0:
    if dX < 0:
      # Always print text left to right
      dX = -dX
      s = list(s)
      s.reverse()
      s = string.join(s, '')

  for char in s:
    writeChar(fid, char, posX, posY, degrees)
    posX += dX
    posY += dY

def drawLine(fid, X1, Y1, X2, Y2):
  drawPolyline(fid, [(X1,Y1), (X2,Y2)], 0, 0)

def boundingBox(s, X1, Y1):
  "Return (X1,Y1),(X2,Y2) for given string"
  if not s:
    return (X1, Y1), (X1, Y1)
    
  X2 = X1 + (len(s)-1)*SpacingDX + 10*strokes.MaxWidth
  Y2 = Y1 + 10*strokes.MaxHeight  # Not including descenders
  return (X1, Y1), (X2, Y2)

def drawDimensionArrow(fid, X, Y, facing):
  writeGlyph(fid, ArrowGlyph, X, Y, facing*90, "Arrow")
  
def drawDrillHit(fid, X, Y, toolNum):
  writeGlyph(fid, strokes.DrillStrokeList[toolNum], X, Y, 0, "Drill%02d" % toolNum)

if __name__=="__main__":
  import string
  s = string.digits+string.letters+string.punctuation
  #s = "The quick brown fox jumped over the lazy dog!"

  fid = file('test.ger','wt')
  fid.write("""G75*
G70*
%OFA0B0*%
%FSAX24Y24*%
%IPPOS*%
%LPD*%
%AMOC8*
5,1,8,0,0,1.08239X$1,22.5*
*%
%ADD10C,0.0100*%
D10*
""")

  writeString(fid, s, 0, 0, 0)
  drawDimensionArrow(fid, 0, 5000, FacingLeft)
  drawDimensionArrow(fid, 5000, 5000, FacingRight)
  drawDimensionArrow(fid, 0, 10000, FacingUp)
  drawDimensionArrow(fid, 5000, 10000, FacingDown)

  for diam in range(0,strokes.MaxNumDrillTools):
    writeGlyph(fid, strokes.DrillStrokeList[diam], diam*1250, 15000, 0, "%02d" % diam)

  fid.write("M02*\n")
  fid.close()
