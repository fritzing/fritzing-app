#!/usr/bin/env python
"""A tiling is an arrangement of jobs, where each job may
be a copy of another and may be rotated. A tiling consists
of two things:

  - a list of where each job is located (the lower-left of
    each job is the origin)

  - a list of points that begins at (0,Ymax) and ends at
    (Xmax,0). These points describe the outside boundary
    of the tiling.
--------------------------------------------------------------------

This program is licensed under the GNU General Public License (GPL).
See http://www.fsf.org for details of the license.

Andrew Sterian
Padnos School of Engineering
Grand Valley State University
<steriana@claymore.engineer.gvsu.edu>
<http://claymore.engineer.gvsu.edu/~steriana>
"""

import sys
import math

import config
import jobs

# Helper functions to determine if points are right-of, left-of, above, and
# below each other. These definitions assume that points are on a line that
# is vertical or horizontal.
def left_of(p1,p2):
  return p1[0]<p2[0] and p1[1]==p2[1]

def right_of(p1,p2):
  return p1[0]>p2[0] and p1[1]==p2[1]

def above(p1,p2):
  return p1[1]>p2[1] and p1[0]==p2[0]

def below(p1,p2):
  return p1[1]<p2[1] and p1[0]==p2[0]

class Tiling:
  def __init__(self, Xmax, Ymax):
    # Make maximum dimensions bigger by inter-job spacing so that
    # we allow jobs (which are seated at the lower left of their cells)
    # to just fit on the panel, and not disqualify them because their
    # spacing area slightly exceeds the panel edge.
    self.xmax = Xmax + config.Config['xspacing']
    self.ymax = Ymax + config.Config['yspacing']

    self.points = [(0,Ymax), (0,0), (Xmax,0)]    # List of (X,Y) co-ordinates
    self.jobs = []   # List of 3-tuples: ((Xbl,Ybl),(Xtr,Ytr),Job) where
                     # (Xbl,Ybl) is bottom left, (Xtr,Ytr) is top-right of the cell.
                     # The actual job has dimensions (Xtr-Xbl-Config['xspacing'],Ytr-Ybl-Config['yspacing'])
                     # and is located at the lower-left of the cell.

  def canonicalize(self, OriginX, OriginY):
    """Return a list of JobLayout objects, after setting each job's (X,Y) origin"""
    L = []
    for job in self.jobs:
      J = jobs.JobLayout(job[2])
      J.setPosition(job[0][0]+OriginX, job[0][1]+OriginY)
      L.append(J)

    return L

  def corners(self):
    return len(self.points)-2

  def clone(self):
    T = Tiling(self.xmax-config.Config['xspacing'], self.ymax-config.Config['yspacing'])
    T.points = self.points[:]
    T.jobs = self.jobs[:]
    return T

  def dump(self, fid=sys.stdout):
    fid.write("Points:\n  ")
    count = 0
    for XY in self.points:
      fid.write("%s " % str(XY))
      count += 1
      if count==8:
        fid.write("\n  ")
        count=0
    if count:
      fid.write("\n")

    fid.write("Jobs:\n")
    for bl,tr,Job in self.jobs:
      fid.write("  %s: %s\n" % (str(Job), str(bl)))
      
  def joblist(self, fid=sys.stdout):
    for bl,tr,Job in self.jobs:
      fid.write("%s@(%.1f,%.1f) " % (Job.name,bl[0],bl[1]))
    fid.write('\n')

  def isOverlap(self, ix, X, Y, cfg=config.Config):
    """Determines if a new job with actual dimensions X-by-Y located at self.points[ix]
       overlaps any existing job or exceeds the boundaries of the panel.
       
       If it's an L-point, the new job will have position

         p_bl=(self.points[ix][0],self.points[ix][1])

       and top-right co-ordinate:
        
         p_tr=(self.points[ix][0]+X,self.points[ix][1]+Y)

       If it's a mirror-L point, the new job will have position

         p_bl=(self.points[ix][0]-X,self.points[ix][1])

       and top-right co-ordinate:

         p_tr=(self.points[ix][0],self.points[ix][1]+Y)

       For a test job defined by t_bl and t_tr, the given job overlaps
       if:
          p.left_edge<t.right_edge and p.right_edge>t.left_edge
                                   and
          p.bottom_edge<t.top_edge and p.top_edge>t.bottom_edge
    """
    if self.isL(ix):
      p_bl = self.points[ix]
      p_tr = (p_bl[0]+X, p_bl[1]+Y)
      if p_tr[0]>self.xmax or p_tr[1]>self.ymax:
        return 1
    else:
      p_bl = (self.points[ix][0]-X,self.points[ix][1])
      p_tr = (self.points[ix][0],self.points[ix][1]+Y)
      if p_bl[0]<0 or p_tr[1]>self.ymax:
        return 1

    for t_bl,t_tr,Job in self.jobs:
      if p_bl[0]<t_tr[0] and p_tr[0]>t_bl[0] \
                         and                 \
         p_bl[1]<t_tr[1] and p_tr[1]>t_bl[1]:
        return 1         

    return 0

  def isL(self, ix):
    """True if self.points[ix] represents an L-shaped corner where there
       is free space above and to the right, like this:

               +------+  _____ this point is an L-shaped corner
               |      | /
               |      +______+
               |             |
               |             |
               .             .
               .             .
               .             .
    """
    pts = self.points
    # This is an L-point if:
    #   Previous point X co-ordinates are the same, and
    #   previous point Y co-ordinate is higher, and
    #   next point Y co-ordinate is the same, and
    #   next point X co-ordinate is to the right
    return pts[ix-1][0]==pts[ix][0]      \
       and pts[ix-1][1]>pts[ix][1]       \
       and pts[ix+1][1]==pts[ix][1]      \
       and pts[ix+1][0]>pts[ix][0]        

  def isMirrorL(self, ix):
    """True if self.points[ix] represents a mirrored L-shaped corner where there
       is free space above and to the left, like this:

                      +------+
mirrored-L corner __  |      |
                    \ |      |
               +______+      |
               |             |
               |             |
               .             .
               .             .
               .             .
    """
    pts = self.points
    # This is a mirrored L-point if:
    #   Previous point Y co-ordinates are the same, and
    #   previous point X co-ordinate is lower, and
    #   next point X co-ordinate is the same, and
    #   next point X co-ordinate is higher
    return pts[ix-1][1]==pts[ix][1]       \
       and pts[ix-1][0]<pts[ix][0]        \
       and pts[ix+1][0]==pts[ix][0]       \
       and pts[ix+1][1]>pts[ix][1]        

  def validAddPoints(self, X, Y):
    """Return a list of all valid indices into self.points at which we can add
    the job with dimensions X-by-Y). Only points which are either L-points or
    mirrored-L-points and which would support the given job with no overlaps
    are returned.
    """
    return [ix for ix in range(1,len(self.points)-1) if (self.isL(ix) or self.isMirrorL(ix)) and not self.isOverlap(ix,X,Y)]

  def mergePoints(self, ix):
    """Inspect points self.points[ix] and self.points[ix+1] as well
       as self.points[ix+3] and self.points[ix+4]. If they are the same, delete
       both points, thus merging lines formed when the corners of two jobs coincide.
    """

    # Do farther-on points first so we can delete things right from the list
    if self.points[ix+3]==self.points[ix+4]:
      del self.points[ix+3:ix+5]

    if self.points[ix]==self.points[ix+1]:
      del self.points[ix:ix+2]

  # Experimental
  def removeInlets(self, minSize):
    """Find sequences of 3 points that define an "inlet", either a left/right-going gap:

                   ...---------------+                          +--------.....
                                     |                          |
                                     |                          |
                        +------------+                          +------------+
                        |                                                    |
                        +----------------------+        +--------------------+
                                               |        |
                                               .        .
                                               .        .
                                               .        .
       or a down-going gap:                                             +-------.....
                                                                        |
                  ...-----------+                    ...-------------+  |
                                |                                    |  |
                                |                                    |  |
                                |                                    |  |
                                |                                    |  |
                                |  +-----....                        |  |
                                |  |                                 |  |
                                |  |                                 |  |
                                +--+                                 +--+
       
       that are too small for any job to fit in (as defined by minSize). These inlets
       can be deleted to form corners where new jobs can be placed.
    """
    pt = self.points
    done = 0

    while not done:
      # Repeat this loop each time there is a change
      for ix in range(0, len(pt)-3):

        # Check for horizontal left-going inlet
        if right_of(pt[ix],pt[ix+1]) and above(pt[ix+1],pt[ix+2]) and left_of(pt[ix+2],pt[ix+3]):
          # Make sure minSize requirement is met
          if pt[ix][1]-pt[ix+3][1] < minSize:
            # Get rid of middle two points, extend Y-value of highest point down to lowest point
            pt[ix] = (pt[ix][0],pt[ix+3][1])
            del pt[ix+1:ix+3]
            break

        # Check for horizontal right-going inlet
        if left_of(pt[ix],pt[ix+1]) and below(pt[ix+1],pt[ix+2]) and right_of(pt[ix+2],pt[ix+3]):
          # Make sure minSize requirement is met
          if pt[ix+3][1]-pt[ix][1] < minSize:
            # Get rid of middle two points, exten Y-value of highest point down to lowest point
            pt[ix+3] = (pt[ix+3][0], pt[ix][1])
            del pt[ix+1:ix+3]
            break

        # Check for vertical inlets
        if above(pt[ix],pt[ix+1]) and left_of(pt[ix+1],pt[ix+2]) and below(pt[ix+2],pt[ix+3]):
          # Make sure minSize requirement is met
          if pt[ix+3][0]-pt[ix][0] < minSize:
            # Is right side lower or higher?
            if pt[ix+3][1]>=pt[ix][1]:   # higher?
              pt[ix] = (pt[ix+3][0], pt[ix][1]) # Move first point to the right
            else:                        # lower?
              pt[ix+3] = (pt[ix][0], pt[ix+3][1]) # Move last point to the left
            del pt[ix+1:ix+3]
            break

      else:
        done = 1

  def addLJob(self, ix, X, Y, Job, cfg=config.Config):
    """Add a job to the tiling at L-point self.points[ix] with actual dimensions X-by-Y.
    The job is added with its lower-left corner at the point. The existing point
    is removed from the tiling and new points are added at the top-left, top-right
    and bottom-right of the new job, with extra space added for inter-job spacing.
    """
    x,y = self.points[ix]
    x_tr = x+X
    y_tr = y+Y
    self.points[ix:ix+1] = [(x,y_tr), (x_tr,y_tr), (x_tr,y)]
    self.jobs.append( ((x,y),(x_tr,y_tr),Job) )
        
    self.mergePoints(ix-1)

  def addMirrorLJob(self, ix, X, Y, Job, cfg=config.Config):
    """Add a job to the tiling at mirror-L-point self.points[ix] with dimensions X-by-Y.
    The job is added with its lower-right corner at the point. The existing point
    is removed from the tiling and new points are added at the bottom-left, top-left
    and top-right of the new job, with extra space added for inter-job spacing.
    """
    x_tr,y = self.points[ix]
    x    = x_tr-X
    y_tr = y+Y
    self.points[ix:ix+1] = [(x,y), (x,y_tr), (x_tr,y_tr)]
    self.jobs.append( ((x,y),(x_tr,y_tr),Job) )
        
    self.mergePoints(ix-1)

  def addJob(self, ix, X, Y, Job):
    """Add a job to the tiling at point self.points[ix] and with dimensions X-by-Y.
    If the given point is an L-point, the job will be added with its lower-left
    corner at the point. If the given point is a mirrored-L point, the job will
    be added with its lower-right corner at the point.
    """
    if self.isL(ix):
      self.addLJob(ix, X, Y, Job)
    else:
      self.addMirrorLJob(ix, X, Y, Job)

  def bounds(self):
    """Return 2-tuple ((minX, minY), (maxX, maxY)) of rectangular region defined by all jobs"""
    minX = minY = float(sys.maxint)
    maxX = maxY = 0.0

    for bl,tr,job in self.jobs:
      minX = min(minX,bl[0])
      maxX = max(maxX,tr[0])
      minY = min(minY,bl[1])
      maxY = max(maxY,tr[1])

    return ( (minX,minY), (maxX-config.Config['xspacing'], maxY-config.Config['yspacing']) )

  def area(self):
    """Return area of rectangular region defined by all jobs."""
    bl,tr = self.bounds()

    DX = tr[0]-bl[0]
    DY = tr[1]-bl[1]
    return DX*DY

  def usedArea(self):
    """Return total area of just jobs, not spaces in-between."""
    area = 0.0
    for job in self.jobs:
      area += job[2].jobarea()

    return area

# Function to estimate the maximum possible utilization given a list of jobs.
# Jobs list is 4-tuple (Xdim,Ydim,job,rjob).
def maxUtilization(Jobs):
  xspacing = config.Config['xspacing']
  yspacing = config.Config['yspacing']

  usedArea = totalArea = 0.0
  for Xdim,Ydim,job,rjob in Jobs:
    usedArea += job.jobarea()
    totalArea += job.jobarea()
    totalArea += job.width_in()*xspacing + job.height_in()*yspacing + xspacing*yspacing
    
  # Reduce total area by strip of unused spacing around top and side. Assume
  # final result will be approximately square.
  sq_side = math.sqrt(totalArea)
  totalArea -= sq_side*xspacing + sq_side*yspacing + xspacing*yspacing

  return usedArea/totalArea

# Utility function to compute the minimum dimension along any axis of all jobs.
# Used to remove inlets.
def minDimension(Jobs):
  M = float(sys.maxint)
  for Xdim,Ydim,job,rjob in Jobs:
    M = min(M,Xdim)
    M = min(M,Ydim)
  return M

# vim: expandtab ts=2 sw=2
