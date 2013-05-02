#!/usr/bin/env python
"""A placement is a final arrangement of jobs at given (X,Y) positions.
This class is intended to "un-pack" an arragement of jobs constructed
manually through Layout/Panel/JobLayout/etc. (i.e., a layout.def file)
or automatically through a Tiling. From either source, the result is
simply a list of jobs.
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
import re

import parselayout
import jobs

class Placement:
  def __init__(self):
    self.jobs = []    # A list of JobLayout objects

  def addFromLayout(self, Layout):
    # Layout is a recursive list of JobLayout items. At the end
    # of each tree there is a JobLayout object which has a 'job'
    # member, which is what we're looking for. Fortunately, the
    # canonicalize() function flattens the tree.
    #
    # Positions of jobs have already been set (we're assuming)
    # prior to calling this function.
    self.jobs = self.jobs + parselayout.canonicalizePanel(Layout)

  def addFromTiling(self, T, OriginX, OriginY):
    # T is a Tiling. Calling its canonicalize() method will construct
    # a list of JobLayout objects and set the (X,Y) position of each
    # object.
    self.jobs = self.jobs + T.canonicalize(OriginX,OriginY)

  def extents(self):
    """Return the maximum X and Y value over all jobs"""
    maxX = 0.0
    maxY = 0.0

    for job in self.jobs:
      maxX = max(maxX, job.x+job.width_in())
      maxY = max(maxY, job.y+job.height_in())

    return (maxX,maxY)

  def write(self, fname):
    """Write placement to a file"""
    fid = file(fname, 'wt')
    for job in self.jobs:
      fid.write('%s %.3f %.3f\n' % (job.job.name, job.x, job.y))
    fid.close()

  def addFromFile(self, fname, Jobs):
    """Read placement from a file, placed against jobs in Jobs list"""
    pat = re.compile(r'\s*(\S+)\s+(\S+)\s+(\S+)')
    comment = re.compile(r'\s*(?:#.+)?$')
   
    try:
      fid = file(fname, 'rt')
    except:
      print 'Unable to open placement file: "%s"' % fname
      sys.exit(1)

    lines = fid.readlines()
    fid.close()

    for line in lines:
      if comment.match(line): continue

      match = pat.match(line)
      if not match:
        print 'Cannot interpret placement line in placement file:\n  %s' % line
        sys.exit(1)

      jobname, X, Y = match.groups()
      try:
        X = float(X)
        Y = float(Y)
      except:
        print 'Illegal (X,Y) co-ordinates in placement file:\n  %s' % line
        sys.exit(1)

      rotated = 0
      if len(jobname) > 8:        
          if jobname[-8:] == '*rotated':
            rotated = 90
            jobname = jobname[:-8]
          elif jobname[-10:] == '*rotated90':
            rotated = 90
            jobname = jobname[:-10]
          elif jobname[-11:] == '*rotated180':
            rotated = 180
            jobname = jobname[:-11]
          elif jobname[-11:] == '*rotated270':
            rotated = 270
            jobname = jobname[:-11]

      addjob = parselayout.findJob(jobname, rotated, Jobs)
      addjob.setPosition(X,Y)
      self.jobs.append(addjob)

