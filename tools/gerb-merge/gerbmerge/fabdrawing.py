#!/usr/bin/env python
"""This file handles the writing of the fabrication drawing Gerber file

--------------------------------------------------------------------

This program is licensed under the GNU General Public License (GPL).
See http://www.fsf.org for details of the license.

Andrew Sterian
Padnos College of Engineering and Computing
Grand Valley State University
<steriana@claymore.engineer.gvsu.edu>
<http://claymore.engineer.gvsu.edu/~steriana>
"""

import string

import config
import makestroke
import util

def writeDrillHits(fid, Place, Tools):  
  toolNumber = -1 

  for tool in Tools:
    toolNumber += 1

    try:
      size = config.GlobalToolMap[tool]
    except:
      raise RuntimeError, "INTERNAL ERROR: Tool code %s not found in global tool list" % tool

    #for row in Layout:
    #  row.writeDrillHits(fid, size, toolNumber)
    for job in Place.jobs:
      job.writeDrillHits(fid, size, toolNumber)

def writeBoundingBox(fid, OriginX, OriginY, MaxXExtent, MaxYExtent):
  x = util.in2gerb(OriginX)
  y = util.in2gerb(OriginY)
  X = util.in2gerb(MaxXExtent)
  Y = util.in2gerb(MaxYExtent)

  makestroke.drawPolyline(fid, [(x,y), (X,y), (X,Y), (x,Y), (x,y)], 0, 0)

def writeDrillLegend(fid, Tools, OriginY, MaxXExtent):
  # This is the spacing from the right edge of the board to where the
  # drill legend is to be drawn, in inches. Remember we have to allow
  # for dimension arrows, too.
  dimspace = 0.5  # inches

  # This is the spacing from the drill hit glyph to the drill size
  # in inches.
  glyphspace = 0.1 # inches

  # Convert to Gerber 2.5 units
  dimspace = util.in2gerb(dimspace)
  glyphspace = util.in2gerb(glyphspace)

  # Construct a list of tuples (toolSize, toolNumber) where toolNumber
  # is the position of the tool in Tools and toolSize is in inches.
  L = []
  toolNumber = -1
  for tool in Tools:
    toolNumber += 1
    L.append((config.GlobalToolMap[tool], toolNumber))

  # Now sort the list from smallest to largest
  L.sort()

  # And reverse to go from largest to smallest, so we can write the legend
  # from the bottom up
  L.reverse()

  # For each tool, draw a drill hit marker then the size of the tool
  # in inches.
  posY = util.in2gerb(OriginY)
  posX = util.in2gerb(MaxXExtent) + dimspace
  maxX = 0
  for size,toolNum in L:
    # Determine string to write and midpoint of string
    s = '%.3f"' % size
    ll, ur = makestroke.boundingBox(s, posX+glyphspace, posY) # Returns lower-left point, upper-right point
    midpoint = (ur[1]+ll[1])/2

    # Keep track of maximum extent of legend
    maxX = max(maxX, ur[0])

    makestroke.drawDrillHit(fid, posX, midpoint, toolNum)
    makestroke.writeString(fid, s, posX+glyphspace, posY, 0)

    posY += int(round((ur[1]-ll[1])*1.5))

  # Return value is lower-left of user text area, without any padding.
  return maxX, util.in2gerb(OriginY)

def writeDimensionArrow(fid, OriginX, OriginY, MaxXExtent, MaxYExtent):
  x = util.in2gerb(OriginX)
  y = util.in2gerb(OriginY)
  X = util.in2gerb(MaxXExtent)
  Y = util.in2gerb(MaxYExtent)

  # This constant is how far away from the board the centerline of the dimension
  # arrows should be, in inches.
  dimspace = 0.2

  # Convert it to Gerber (0.00001" or 2.5) units
  dimspace = util.in2gerb(dimspace)

  # Draw an arrow above the board, on the left side and right side
  makestroke.drawDimensionArrow(fid, x, Y+dimspace, makestroke.FacingLeft)
  makestroke.drawDimensionArrow(fid, X, Y+dimspace, makestroke.FacingRight)

  # Draw arrows to the right of the board, at top and bottom
  makestroke.drawDimensionArrow(fid, X+dimspace, Y, makestroke.FacingUp)
  makestroke.drawDimensionArrow(fid, X+dimspace, y, makestroke.FacingDown)

  # Now draw the text. First, horizontal text above the board.
  s = '%.3f"' % (MaxXExtent - OriginX)
  ll, ur = makestroke.boundingBox(s, 0, 0)
  s_width = ur[0]-ll[0]   # Width in 2.5 units
  s_height = ur[1]-ll[1]  # Height in 2.5 units

  # Compute the position in 2.5 units where we should draw this. It should be
  # centered horizontally and also vertically about the dimension arrow centerline.
  posX = x + (x+X)/2
  posX -= s_width/2
  posY = Y + dimspace - s_height/2
  makestroke.writeString(fid, s, posX, posY, 0)

  # Finally, draw the extending lines from the text to the arrows.
  posY = Y + dimspace
  posX1 = posX - util.in2gerb(0.1) # 1000
  posX2 = posX + s_width + util.in2gerb(0.1) # 1000
  makestroke.drawLine(fid, x, posY, posX1, posY)
  makestroke.drawLine(fid, posX2, posY, X, posY)

  # Now do the vertical text
  s = '%.3f"' % (MaxYExtent - OriginY)
  ll, ur = makestroke.boundingBox(s, 0, 0)
  s_width = ur[0]-ll[0]
  s_height = ur[1]-ll[1]

  # As above, figure out where to draw this. Rotation will be -90 degrees
  # so new origin will be top-left of bounding box after rotation.
  posX = X + dimspace - s_height/2
  posY = y + (y+Y)/2
  posY += s_width/2
  makestroke.writeString(fid, s, posX, posY, -90)

  # Draw extending lines
  posX = X + dimspace
  posY1 = posY + util.in2gerb(0.1) # 1000
  posY2 = posY - s_width - util.in2gerb(0.1) # 1000
  makestroke.drawLine(fid, posX, Y, posX, posY1)
  makestroke.drawLine(fid, posX, posY2, posX, y)

def writeUserText(fid, X, Y):
  fname = config.Config['fabricationdrawingtext']
  if not fname: return

  try:
    tfile = file(fname, 'rt')
  except Exception, detail:
    raise RuntimeError, "Could not open fabrication drawing text file '%s':\n  %s" % (fname,str(detail))

  lines = tfile.readlines()
  tfile.close()
  lines.reverse()  # We're going to print from bottom up

  # Offset X position to give some clearance from drill legend
  X += util.in2gerb(0.2) # 2000

  for line in lines:
    # Get rid of CR
    line = string.replace(line, '\x0D', '')

    # Chop off \n
    #if line[-1] in string.whitespace:
    #  line = line[:-1]

    # Strip off trailing whitespace
    line = string.rstrip(line)

    # Blank lines still need height, so must have at least one character
    if not line:
      line = ' '

    ll, ur = makestroke.boundingBox(line, X, Y)
    makestroke.writeString(fid, line, X, Y, 0)

    Y += int(round((ur[1]-ll[1])*1.5))

# Main entry point. Gerber file has already been opened, header written
# out, 1mil tool selected.
def writeFabDrawing(fid, Place, Tools, OriginX, OriginY, MaxXExtent, MaxYExtent):

  # Write out all the drill hits
  writeDrillHits(fid, Place, Tools)

  # Draw a bounding box for the project
  writeBoundingBox(fid, OriginX, OriginY, MaxXExtent, MaxYExtent)

  # Write out the drill hit legend off to the side. This function returns
  # (X,Y) lower-left origin where user text is to begin, in Gerber units
  # and without any padding.
  X,Y = writeDrillLegend(fid, Tools, OriginY, MaxXExtent)

  # Write out the dimensioning arrows
  writeDimensionArrow(fid, OriginX, OriginY, MaxXExtent, MaxYExtent)

  # Finally, write out user text
  writeUserText(fid, X, Y)
