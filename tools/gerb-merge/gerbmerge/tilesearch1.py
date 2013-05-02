#!/usr/bin/env python
"""Search for an optimum tiling using brute force exhaustive search
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

import config
import tiling

_StartTime = 0.0           # Start time of tiling
_CkpointTime = 0.0         # Next time to print stats
_Placements = 0L           # Number of placements attempted
_PossiblePermutations = 0L # Number of different ways of ordering jobs
_Permutations = 0L         # Number of different job orderings already computed
_TBestTiling = None        # Best tiling so far
_TBestScore  = float(sys.maxint) # Smallest area so far
_PrintStats = 1            # Print statistics every 3 seconds

def printTilingStats():
  global _CkpointTime
  _CkpointTime = time.time() + 3

  if _TBestTiling:
    area = _TBestTiling.area()
    utilization = _TBestTiling.usedArea() / area * 100.0
  else:
    area = 999999.0
    utilization = 0.0

  percent = 100.0*_Permutations/_PossiblePermutations

  print "\r  %5.2f%% complete / %ld/%ld Perm/Place / Smallest area: %.1f sq. in. / Best utilization: %.1f%%" % \
        (percent, _Permutations, _Placements, area, utilization),

  sys.stdout.flush()

def bestTiling():
  return _TBestTiling

def _tile_search1(Jobs, TSoFar, firstAddPoint, cfg=config.Config):
  """This recursive function does the following with an existing tiling TSoFar:
     
     * For each 4-tuple (Xdim,Ydim,job,rjob) in Jobs, the non-rotated 'job' is selected
     
     * For the non-rotated job, the list of valid add-points is found

     * For each valid add-point, the job is placed at this point in a new,
       cloned tiling.

     * The function then calls its recursively with the remaining list of
       jobs.

     * The rotated job is then selected and the list of valid add-points is
       found. Again, for each valid add-point the job is placed there in
       a new, cloned tiling.

     * Once again, the function calls itself recursively with the remaining
       list of jobs.

     * The best tiling encountered from all recursive calls is returned.

     If TSoFar is None it means this combination of jobs is not tileable.

     The side-effect of this function is to set _TBestTiling and _TBestScore
     to the best tiling encountered so far. _TBestTiling could be None if
     no valid tilings have been found so far.
  """
  global _StartTime, _CkpointTime, _Placements, _TBestTiling, _TBestScore, _Permutations, _PrintStats

  if not TSoFar:
    return (None, float(sys.maxint))

  if not Jobs:
    # Update the best tiling and score. If the new tiling matches
    # the best score so far, compare on number of corners, trying to
    # minimize them.
    score = TSoFar.area()

    if score < _TBestScore:
      _TBestTiling,_TBestScore = TSoFar,score
    elif score == _TBestScore:
      if TSoFar.corners() < _TBestTiling.corners():
        _TBestTiling,_TBestScore = TSoFar,score

    _Placements += 1
    if firstAddPoint:
      _Permutations += 1
    return

  xspacing = cfg['xspacing']
  yspacing = cfg['yspacing']

  minInletSize = tiling.minDimension(Jobs)
  TSoFar.removeInlets(minInletSize)

  for job_ix in range(len(Jobs)):
    # Pop off the next job and construct remaining_jobs, a sub-list
    # of Jobs with the job we've just popped off excluded.
    Xdim,Ydim,job,rjob = Jobs[job_ix]
    remaining_jobs = Jobs[:job_ix]+Jobs[job_ix+1:]

    if 0:
      print "Level %d (%s)" % (level, job.name)
      TSoFar.joblist()
      for J in remaining_jobs:
          print J[2].name, ", ",
      print
      print '-'*75

    # Construct add-points for the non-rotated and rotated job.
    # As an optimization, do not construct add-points for the rotated
    # job if the job is a square (duh).
    addpoints1 = TSoFar.validAddPoints(Xdim+xspacing,Ydim+yspacing)     # unrotated job
    if Xdim != Ydim:
      addpoints2 = TSoFar.validAddPoints(Ydim+xspacing,Xdim+yspacing)   # rotated job
    else:
      addpoints2 = []

    # Recursively construct tilings for the non-rotated job and
    # update the best-tiling-so-far as we do so.
    if addpoints1:
      for ix in addpoints1:
        # Clone the tiling we're starting with and add the job at this
        # add-point.
        T = TSoFar.clone()
        T.addJob(ix, Xdim+xspacing, Ydim+yspacing, job)

        # Recursive call with the remaining jobs and this new tiling. The
        # point behind the last parameter is simply so that _Permutations is
        # only updated once for each permutation, not once per add-point.
        # A permutation is some ordering of jobs (N! choices) and some
        # ordering of non-rotated and rotated within that ordering (2**N
        # possibilities per ordering).
        _tile_search1(remaining_jobs, T, firstAddPoint and ix==addpoints1[0])
    elif firstAddPoint:
      # Premature prune due to not being able to put this job anywhere. We
      # have pruned off 2^M permutations where M is the length of the remaining
      # jobs.
      _Permutations += 2L**len(remaining_jobs)

    if addpoints2:
      for ix in addpoints2:
        # Clone the tiling we're starting with and add the job at this
        # add-point. Remember that the job is rotated so swap X and Y
        # dimensions.
        T = TSoFar.clone()
        T.addJob(ix, Ydim+xspacing, Xdim+yspacing, rjob)

        # Recursive call with the remaining jobs and this new tiling.
        _tile_search1(remaining_jobs, T, firstAddPoint and ix==addpoints2[0])
    elif firstAddPoint:
      # Premature prune due to not being able to put this job anywhere. We
      # have pruned off 2^M permutations where M is the length of the remaining
      # jobs.
      _Permutations += 2L**len(remaining_jobs)

    # If we've been at this for 3 seconds, print some status information
    if _PrintStats and time.time() > _CkpointTime:
      printTilingStats()

  # end for each job in job list

def factorial(N):
  if (N <= 1): return 1L

  prod = long(N)
  while (N > 2):
      N -= 1
      prod *= N

  return prod

def initialize(printStats=1):
  global _StartTime, _CkpointTime, _Placements, _TBestTiling, _TBestScore, _Permutations, _PossiblePermutations, _PrintStats

  _PrintStats = printStats
  _Placements = 0L
  _Permutations = 0L
  _TBestTiling = None
  _TBestScore = float(sys.maxint)

def tile_search1(Jobs, X, Y):
  """Wrapper around _tile_search1 to handle keyboard interrupt, etc."""
  global _StartTime, _CkpointTime, _Placements, _TBestTiling, _TBestScore, _Permutations, _PossiblePermutations

  initialize()

  _StartTime = time.time()
  _CkpointTime = _StartTime + 3
  # There are (2**N)*(N!) possible permutations where N is the number of jobs.
  # This is assuming all jobs are unique and each job has a rotation (i.e., is not
  # square). Practically, these assumptions make no difference because the software
  # currently doesn't optimize for cases of repeated jobs.
  _PossiblePermutations = (2L**len(Jobs))*factorial(len(Jobs))
  #print "Possible permutations:", _PossiblePermutations

  print '='*70
  print "Starting placement using exhaustive search."
  print "There are %ld possible permutations..." % _PossiblePermutations,
  if _PossiblePermutations < 1e4:
    print "this'll take no time at all."
  elif _PossiblePermutations < 1e5:
    print "surf the web for a few minutes."
  elif _PossiblePermutations < 1e6:
    print "take a long lunch."
  elif _PossiblePermutations < 1e7:
    print "come back tomorrow."
  else:
    print "don't hold your breath."
  print "Press Ctrl-C to stop and use the best placement so far."
  print "Estimated maximum possible utilization is %.1f%%." % (tiling.maxUtilization(Jobs)*100)

  try:
    _tile_search1(Jobs, tiling.Tiling(X,Y), 1)
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
