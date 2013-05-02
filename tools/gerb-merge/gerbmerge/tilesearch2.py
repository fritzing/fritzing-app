#!/usr/bin/env python
"""Tile search using random placement and evaluation. Works surprisingly well.
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
import time
import random

import config
import tiling
import tilesearch1

_StartTime = 0.0           # Start time of tiling
_CkpointTime = 0.0         # Next time to print stats
_Placements = 0L           # Number of placements attempted
_TBestTiling = None        # Best tiling so far
_TBestScore  = float(sys.maxint) # Smallest area so far

def printTilingStats():
  global _CkpointTime
  _CkpointTime = time.time() + 3

  if _TBestTiling:
    area = _TBestTiling.area()
    utilization = _TBestTiling.usedArea() / area * 100.0
  else:
    area = 999999.0
    utilization = 0.0

  print "\r  %ld placements / Smallest area: %.1f sq. in. / Best utilization: %.1f%%" % \
        (_Placements, area, utilization),

  sys.stdout.flush()

def _tile_search2(Jobs, X, Y, cfg=config.Config):
  global _CkpointTime, _Placements, _TBestTiling, _TBestScore

  r = random.Random()
  N = len(Jobs)

  # M is the number of jobs that will be placed randomly.
  # N-M is the number of jobs that will be searched exhaustively.
  M = N - config.RandomSearchExhaustiveJobs
  M = max(M,0)

  xspacing = cfg['xspacing']
  yspacing = cfg['yspacing']
  
  # Must escape with Ctrl-C
  while 1:
    T = tiling.Tiling(X,Y)
    joborder = r.sample(range(N), N)

    minInletSize = tiling.minDimension(Jobs)

    for ix in joborder[:M]:
      Xdim,Ydim,job,rjob = Jobs[ix]
      
      T.removeInlets(minInletSize)

      if r.choice([0,1]):
        addpoints = T.validAddPoints(Xdim+xspacing,Ydim+yspacing)
        if not addpoints:
          break

        pt = r.choice(addpoints)
        T.addJob(pt, Xdim+xspacing, Ydim+yspacing, job)
      else:
        addpoints = T.validAddPoints(Ydim+xspacing,Xdim+yspacing)
        if not addpoints:
          break

        pt = r.choice(addpoints)
        T.addJob(pt, Ydim+xspacing, Xdim+yspacing, rjob)
    else:
      # Do exhaustive search on remaining jobs
      if N-M:
        remainingJobs = []
        for ix in joborder[M:]:
          remainingJobs.append(Jobs[ix])

        tilesearch1.initialize(0)
        tilesearch1._tile_search1(remainingJobs, T, 1)
        T = tilesearch1.bestTiling()

      if T:
        score = T.area()

        if score < _TBestScore:
          _TBestTiling,_TBestScore = T,score
        elif score == _TBestScore:
          if T.corners() < _TBestTiling.corners():
            _TBestTiling,_TBestScore = T,score

    _Placements += 1
      
    # If we've been at this for 3 seconds, print some status information
    if time.time() > _CkpointTime:
      printTilingStats()

  # end while 1

def tile_search2(Jobs, X, Y):
  """Wrapper around _tile_search2 to handle keyboard interrupt, etc."""
  global _StartTime, _CkpointTime, _Placements, _TBestTiling, _TBestScore

  _StartTime = time.time()
  _CkpointTime = _StartTime + 3
  _Placements = 0L
  _TBestTiling = None
  _TBestScore = float(sys.maxint)

  print '='*70
  print "Starting random placement trials. You must press Ctrl-C to"
  print "stop the process and use the best placement so far."
  print "Estimated maximum possible utilization is %.1f%%." % (tiling.maxUtilization(Jobs)*100)

  try:
    _tile_search2(Jobs, X, Y)
    printTilingStats()
    print
  except KeyboardInterrupt:
    printTilingStats()
    print
    print "Interrupted."

  computeTime = time.time() - _StartTime
  print "Computed %ld placements in %d seconds / %.1f placements/second" % (_Placements, computeTime, _Placements/computeTime)
  print '='*70

  return _TBestTiling
