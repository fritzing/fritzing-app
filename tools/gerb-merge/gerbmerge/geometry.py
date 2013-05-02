#!/usr/bin/env python
"""
General geometry support routines.

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

# Ensure all list elements are unique
def uniqueify(L):
  return {}.fromkeys(L).keys()

# This function rounds an (X,Y) point to integer co-ordinates
def roundPoint(pt):
  return (int(round(pt[0])),int(round(pt[1])))

# Returns True if the segment defined by endpoints p1 and p2 is vertical
def isSegmentVertical(p1, p2):
  return p1[0]==p2[0]

# Returns True if the segment defined by endpoints p1 and p2 is horizontal
def isSegmentHorizontal(p1, p2):
  return p1[1]==p2[1]

# Returns slope of a non-vertical line segment
def segmentSlope(p1, p2):
  return float(p2[1]-p1[1])/(p2[0]-p1[0])

# Determine if the (X,Y) 'point' is on the line segment defined by endpoints p1
# and p2, both (X,Y) tuples. It's assumed that the point is on the line defined
# by the segment, but just may be beyond the endpoints. NOTE: No testing is
# performed to see if the point is actually on the line defined by the segment!
# This is assumed!
def isPointOnSegment(point, p1, p2):
  if isSegmentVertical(p1,p2):
    # Treat vertical lines by comparing Y-ordinates
    return (point[1]-p2[1])*(point[1]-p1[1]) <= 0
  else:
    # Treat other lines, including horizontal lines, by comparing X-ordinates
    return (point[0]-p2[0])*(point[0]-p1[0]) <= 0

# Returns (X,Y) point where the line segment defined by (X,Y) endpoints p1 and
# p2 intersects the line segment defined by endpoints q1 and q2. Only a single
# intersection point is allowed, so no coincident lines.  If there is no point
# of intersection, None is returned.
def segmentXsegment1pt(p1, p2, q1, q2):
  A,B = p1
  C,D = p2
  P,Q = q1
  R,S = q2
  
  # We have to consider special cases of one or other line segments being vertical
  if isSegmentVertical(p1,p2):
    if isSegmentVertical(q1,q2): return None

    x = A
    y = segmentSlope(q1,q2)*(A-P) + Q
  elif isSegmentVertical(q1,q2):
    x = P
    y = segmentSlope(p1,p2)*(P-A) + B
  else:
    m1 = segmentSlope(p1,p2)
    m2 = segmentSlope(q1,q2)

    if m1==m2: return None

    x = (A*m1 - B - P*m2 + Q) / (m1-m2)
    y = m1*(x-A) + B

  # Candidate point identified. Check to make sure it's on both line segments.
  if isPointOnSegment((x,y), p1, p2) and isPointOnSegment((x,y), q1, q2):
    return roundPoint((x,y))
  else:
    return None

# Returns True if the given (X,Y) 'point' is strictly within the rectangle
# defined by (LLX,LLY,URX,URY) co-ordinates (LL=lower left, UR=upper right).
def isPointStrictlyInRectangle(point, rect):
  x,y = point
  llx,lly,urx,ury = rect
  return (llx < x < urx) and (lly < y < ury)

# This function takes two points which define the extents of a rectangle.  The
# return value is a 5-tuple (ll, ul, ur, lr, rect) which comprises 4 points
# (lower-left, upper-left, upper-right, lower-right) and a rect object (minx,
# miny, maxx, maxy). If called with a single argument, it is expected to be
# a 4-tuple (x1,y1,x2,y2).
def canonicalizeExtents(pt1, pt2=None):
  # First canonicalize lower-left and upper-right points
  if pt2 is None:
    maxx = max(pt1[0], pt1[2])
    minx = min(pt1[0], pt1[2])
    maxy = max(pt1[1], pt1[3])
    miny = min(pt1[1], pt1[3])
  else:
    maxx = max(pt1[0], pt2[0])
    minx = min(pt1[0], pt2[0])
    maxy = max(pt1[1], pt2[1])
    miny = min(pt1[1], pt2[1])

  # Construct the four corners
  llpt = (minx,miny)
  urpt = (maxx,maxy)
  ulpt = (minx,maxy)
  lrpt = (maxx,miny)

  # Construct a rect object for use by various functions
  rect = (minx, miny, maxx, maxy)

  return (llpt, ulpt, urpt, lrpt, rect)

# This function returns a list of intersection points of the line segment
# pt1-->pt2 and the box defined by corners llpt and urpt. These corners are
# canonicalized internally so they need not necessarily be lower-left and
# upper-right points.
#
# The return value may be a list of 0, 1, or 2 points.  If the list has 2
# points, then the segment intersects the box in two points since both points
# are outside the box. If the list has 1 point, then the segment has one point
# inside the box and another point outside. If the list is empty, the segment
# has both points outside the box and there is no intersection, or has both
# points inside the box.
#
# Note that segments collinear with box edges produce no points of
# intersection.
def segmentXbox(pt1, pt2, llpt, urpt):
  # First canonicalize lower-left and upper-right points
  llpt, ulpt, urpt, lrpt, rect = canonicalizeExtents(llpt, urpt)

  # Determine whether one point is inside the rectangle and the other is not.
  # Note the XOR operator '^'
  oneInOneOut = isPointStrictlyInRectangle(pt1,rect) ^ isPointStrictlyInRectangle(pt2,rect)

  # Find all intersections of the segment with the 4 sides of the box,
  # one side at a time. L will be the list of definitely-true intersections,
  # while corners is a list of potential intersections. An intersection
  # is potential if a) it is a corner, and b) there is another intersection
  # of the line with the box somewhere else. This is how we handle
  # corner intersections, which are sometimes legal (when one segment endpoint
  # is inside the box and the other isn't, or when the segment intersects the
  # box in two places) and sometimes not (when the segment is "tangent" to
  # the box at the corner and the corner is the signle point of intersection).
  L = []
  corners = []

  # Do not allow intersection if segment is collinear with box sides.  For
  # example, a horizontal line collinear with the box top side should not
  # return an intersection with the upper-left or upper-right corner.
  # Similarly, a point of intersection that is a corner should only be
  # allowed if one segment point is inside the box and the other is not,
  # otherwise it means the segment is "tangent" to the box at that corner.
  # There is a case, however, in which a corner is a point of intersection
  # with both segment points outside the box, and that is if there are two
  # points of intersection, i.e., the segment goes completely through the box.

  def checkIntersection(corner1, corner2):
    # Check intersection with side of box
    pt = segmentXsegment1pt(pt1, pt2, corner1, corner2)
    if pt in (corner1,corner2):
      # Only allow this corner intersection point if line is not
      # horizontal/vertical and one point is inside rectangle while other is
      # not, or the segment intersects the box in two places. Since oneInOneOut
      # calls isPointStrictlyInRectangle(), which automatically excludes points
      # on the box itself, horizontal/vertical lines collinear with box sides
      # will always lead to oneInOneOut==False (since both will be "out of
      # box").
      if oneInOneOut:
        L.append(pt)
      else:
        corners.append(pt)    # Potentially a point of intersection...we'll have to wait and
                              # see if there is one more point of intersection somewhere else.
    else:
      # Not a corner intersection, so it's valid
      if pt is not None: L.append(pt)

  # Check intersection with left side of box
  checkIntersection(llpt, ulpt)

  # Check intersection with top side of box
  checkIntersection(ulpt, urpt)

  # Check intersection with right side of box
  checkIntersection(urpt, lrpt)

  # Check intersection with bottom side of box
  checkIntersection(llpt, lrpt)

  # Ensure all points are unique. We may get a double hit at the corners
  # of the box.
  L = uniqueify(L)
  corners = uniqueify(corners)

  # If the total number of intersections len(L)+len(corners) is 2, the corner
  # is valid. If there is only a single corner, it's a tangent and invalid.
  # However, if both corners are on the same side of the box, it's not valid.
  numPts = len(L)+len(corners)
  assert numPts <= 2
  if numPts == 2:
    if len(corners)==2 and (isSegmentHorizontal(corners[0], corners[1]) or isSegmentVertical(corners[0],corners[1])):
      return []
    else:
      L += corners
      L.sort()      # Just for stability in assertion checking
      return L
  else:
    L.sort()
    return L      # Correct if numPts==1, since it will be empty or contain a single valid intersection
                  # Correct if numPts==0, since it will be empty

# This function determines if two rectangles defined by 4-tuples
# (minx, miny, maxx, maxy) have any rectangle in common. If so, it is
# returned as a 4-tuple, else None is returned. This function assumes
# the rectangles are canonical so that minx<maxx, miny<maxy. If the
# optional allowLines parameter is True, rectangles that overlap on
# a line are considered overlapping, otherwise they must overlap with
# a rectangle of at least width 1.
def areExtentsOverlapping(E1, E2, allowLines=False):
  minX,minY,maxX,maxY = E1
  minU,minV,maxU,maxV = E2

  if allowLines:
    if (minU > maxX) or (maxU < minX) or (minV > maxY) or (maxV < minY):
      return False
    else:
      return True
  else:
    if (minU >= maxX) or (maxU <= minX) or (minV >= maxY) or (maxV <= minY):
      return False
    else:
      return True

# Compute the intersection of two rectangles defined by 4-tuples E1 and E2,
# which are not necessarily canonicalized.
def intersectExtents(E1, E2):
  ll1, ul1, ur1, lr1, rect1 = canonicalizeExtents(E1)
  ll2, ul2, ur2, lr2, rect2 = canonicalizeExtents(E2)

  if not areExtentsOverlapping(rect1, rect2):
    return None

  xll = max(rect1[0], rect2[0])    # Maximum of minx values
  yll = max(rect1[1], rect2[1])    # Maximum of miny values
  xur = min(rect1[2], rect2[2])    # Minimum of maxx values
  yur = min(rect1[3], rect2[3])    # Minimum of maxy values
  return (xll, yll, xur, yur)

# This function returns True if rectangle E1 is wholly contained within
# rectangle E2. Both E1 and E2 are 4-tuples (minx,miny,maxx,maxy), not
# necessarily canonicalized. This function is like a slightly faster
# version of "intersectExtents(E1, E2)==E1".
def isRect1InRect2(E1, E2):
  ll1, ul1, ur1, lr1, rect1 = canonicalizeExtents(E1)
  ll2, ul2, ur2, lr2, rect2 = canonicalizeExtents(E2)
  
  return (ll1[0] >= ll2[0]) and (ll1[1] >= ll2[1]) \
     and (ur1[0] <= ur2[0]) and (ur1[1] <= ur2[1])

# Return width of rectangle, which may be 0 if bottom-left and upper-right X
# positions are the same. The rectangle is a 4-tuple (minx,miny,maxx,maxy).
def rectWidth(rect):
  return abs(rect[2]-rect[0])

# Return height of rectangle, which may be 0 if bottom-left and upper-right Y
# positions are the same. The rectangle is a 4-tuple (minx,miny,maxx,maxy).
def rectHeight(rect):
  return abs(rect[3]-rect[1])

# Return center (X,Y) co-ordinates of rectangle.
def rectCenter(rect):
  dx = rectWidth(rect)
  dy = rectHeight(rect)

  if dx & 1:    # Odd width: center is (left+right)/2 + 1/2
    X = (rect[0] + rect[2] + 1)/2
  else:         # Even width: center is (left+right)/2
    X = (rect[0] + rect[2])/2

  if dy & 1:
    Y = (rect[1] + rect[3] + 1)/2
  else:
    Y = (rect[1] + rect[3])/2

  return (X,Y)

if __name__=="__main__":
  llpt = (1000,1000)
  urpt = (5000,5000)

  # A segment that cuts across the box and intersects in corners
  assert segmentXbox((0,0), (6000,6000), llpt, urpt) == [(1000,1000), (5000,5000)]  # Two valid corners
  assert segmentXbox((0,6000), (6000,0), llpt, urpt) == [(1000,5000), (5000,1000)]  # Two valid corners
  assert segmentXbox((500,500), (2500, 2500), llpt, urpt) == [(1000,1000)]          # One valid corner
  assert segmentXbox((2500,2500), (5500, 5500), llpt, urpt) == [(5000,5000)]        # One valid corner

  # Segments collinear with box sides
  assert segmentXbox((1000,0),    (1000,6000), llpt, urpt) == []    # Box side contained in segment
  assert segmentXbox((1000,0),    (1000,3000), llpt, urpt) == []    # Box side partially overlaps segment
  assert segmentXbox((1000,2000), (1000,4000), llpt, urpt) == []    # Segment contained in box side

  # Segments fully contained within box
  assert segmentXbox((1500,2000), (2000,2500), llpt, urpt) == []

  # Segments with points on box sides
  assert segmentXbox((2500,1000), (2700,1200), llpt, urpt) == [(2500,1000)]   # One point on box side
  assert segmentXbox((2500,1000), (2700,5000), llpt, urpt) == [(2500,1000), (2700,5000)]   # Two points on box sides

  # Segment intersects box at one point
  assert segmentXbox((3500,5500), (3000, 2500), llpt, urpt) == [(3417, 5000)]  # First point outside
  assert segmentXbox((3500,1500), (3000, 6500), llpt, urpt) == [(3150, 5000)]  # Second point outside

  # Segment intersects box at two points, not corners
  assert segmentXbox((500,3000), (1500,500), llpt, urpt) == [(1000,1750), (1300,1000)]
  assert segmentXbox((2500,300), (5500,3500), llpt, urpt) == [(3156,1000), (5000,2967)]
  assert segmentXbox((5200,1200), (2000,6000), llpt, urpt) == [(2667,5000), (5000, 1500)]
  assert segmentXbox((3200,5200), (-10, 1200), llpt, urpt) == [(1000, 2459), (3040, 5000)]

  assert segmentXbox((500,2000), (5500, 2000), llpt, urpt) == [(1000,2000), (5000, 2000)]
  assert segmentXbox((5200,1250), (-200, 4800), llpt, urpt) == [(1000, 4011), (5000, 1381)]

  assert segmentXbox((1300,200), (1300, 5200), llpt, urpt) == [(1300, 1000), (1300, 5000)]
  assert segmentXbox((1200,200), (1300, 5200), llpt, urpt) == [(1216, 1000), (1296, 5000)]

  assert intersectExtents( (100,100,500,500), (500,500,900,900) ) == None
  assert intersectExtents( (100,100,500,500), (400,400,900,900) ) == (400,400,500,500)
  assert intersectExtents( (100,100,500,500), (200,0,600,300) ) == (200,100,500,300)
  assert intersectExtents( (100,100,500,500), (200,0,300,600) ) == (200,100,300,500)

  assert intersectExtents( (100,100,500,500), (0,600,50,550) ) == None
  assert intersectExtents( (100,100,500,500), (0,600,600,-10) ) == (100,100,500,500)
  assert intersectExtents( (100,100,500,500), (0,600,600,200) ) == (100,200,500,500)
  assert intersectExtents( (100,100,500,500), (0,600,300,300) ) == (100,300,300,500)

  assert isRect1InRect2( (100,100,500,500), (0,600,50,550) )  == False
  assert isRect1InRect2( (100,100,500,500), (0,600,600,-10) ) == True
  assert isRect1InRect2( (100,100,500,500), (0,600,600,200) ) == False
  assert isRect1InRect2( (100,100,500,500), (0,600,300,300) ) == False
  assert isRect1InRect2( (100,100,500,500), (0,0,500,500) )   == True

  print 'All tests pass'
