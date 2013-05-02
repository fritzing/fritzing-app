#!/usr/bin/env python
"""
Parse the job layout specification file.

Requires:
  
  - mxTextTools (from mxBase distribution 2.0.4 or higher)
              http://www.egenix.com/files/python/eGenix-mx-Extensions.html

  - SimpleParse 2.0 or higher
              http://simpleparse.sourceforge.net
              

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
import string

from simpleparse.parser import Parser

import config
import jobs

declaration = r'''
file         := (commentline/nullline/rowspec)+
rowspec      := ts, 'Row', ws, '{'!, ts, comment?, '\n', rowjob+, ts, '}'!, ts, comment?, '\n'
rowjob       := jobspec/colspec/commentline/nullline
colspec      := ts, 'Col', ws, '{'!, ts, comment?, '\n', coljob+, ts, '}'!, ts, comment?, '\n'
coljob       := jobspec/rowspec/commentline/nullline

jobspec      := ts, (paneljobspec/basicjobspec), ts, comment?, '\n'
basicjobspec := id, (rotation)?
paneljobspec := 'Not yet implemented'
#paneljobspec := int, [xX], int, ws, basicjobspec
comment      := ([#;]/'//'), -'\n'*
commentline  := ts, comment, '\n'
nullline     := ts, '\n'
rotation     := ws, 'Rotate', ('90'/'180'/'270')?
ws := [ \t]+
ts := [ \t]*
id := [a-zA-Z0-9], [a-zA-Z0-9_-]*
int := [0-9]+
'''

class Panel:                 # Meant to be subclassed as either a Row() or Col()
  def __init__(self):
    self.x = None
    self.y = None
    self.jobs = []           # List (left-to-right or bottom-to-top) of JobLayout() or Row()/Col() objects

  def canonicalize(self):    # Return plain list of JobLayout objects at the roots of all trees
    L = []
    for job in self.jobs:
      L = L + job.canonicalize()
    return L

  def addjob(self, job):     # Either a JobLayout class or Panel (sub)class
    assert isinstance(job, Panel) or isinstance(job, jobs.JobLayout)
    self.jobs.append(job)

  def addwidths(self):
    "Return width in inches"
    width = 0.0
    for job in self.jobs:
      width += job.width_in() + config.Config['xspacing']
    width -= config.Config['xspacing']
    return width

  def maxwidths(self):
    "Return maximum width in inches of any one subpanel"
    width = 0.0
    for job in self.jobs:
      width = max(width,job.width_in())
    return width

  def addheights(self):
    "Return height in inches"
    height = 0.0
    for job in self.jobs:
      height += job.height_in() + config.Config['yspacing']
    height -= config.Config['yspacing']
    return height

  def maxheights(self):
    "Return maximum height in inches of any one subpanel"
    height = 0.0
    for job in self.jobs:
      height = max(height,job.height_in())
    return height

  def writeGerber(self, fid, layername):
    for job in self.jobs:
      job.writeGerber(fid, layername)
    
  def writeExcellon(self, fid, tool):
    for job in self.jobs:
      job.writeExcellon(fid, tool)

  def writeDrillHits(self, fid, tool, toolNum):
    for job in self.jobs:
      job.writeDrillHits(fid, tool, toolNum)

  def writeCutLines(self, fid, drawing_code, X1, Y1, X2, Y2):
    for job in self.jobs:
      job.writeCutLines(fid, drawing_code, X1, Y1, X2, Y2)
    
  def drillhits(self, tool):
    hits = 0
    for job in self.jobs:
      hits += job.drillhits(tool)

    return hits

  def jobarea(self):
    area = 0.0
    for job in self.jobs:
      area += job.jobarea()

    return area

class Row(Panel):
  def __init__(self):
    Panel.__init__(self)
    self.LR = 1   # Horizontal arrangement

  def width_in(self):
    return self.addwidths()

  def height_in(self):
    return self.maxheights()

  def setPosition(self, x, y):   # In inches
    self.x = x
    self.y = y
    for job in self.jobs:
      job.setPosition(x,y)
      x += job.width_in() + config.Config['xspacing']

class Col(Panel):
  def __init__(self):
    Panel.__init__(self)
    self.LR = 0   # Vertical arrangement

  def width_in(self):
    return self.maxwidths()

  def height_in(self):
    return self.addheights()

  def setPosition(self, x, y):   # In inches
    self.x = x
    self.y = y
    for job in self.jobs:
      job.setPosition(x,y)
      y += job.height_in() + config.Config['yspacing']

def canonicalizePanel(panel):
  L = []
  for job in panel:
    L = L + job.canonicalize()
  return L
  
def findJob(jobname, rotated, Jobs=config.Jobs):
  """
    Find a job in config.Jobs, possibly rotating it
    If job not in config.Jobs add it for future reference
    Return found job
  """
                                                                                    
  if rotated == 90:
    fullname = jobname + '*rotated90'
  elif rotated == 180:
    fullname = jobname + '*rotated180'
  elif rotated == 270:
    fullname = jobname + '*rotated270'
  else:
    fullname = jobname
                         
  try:
    for existingjob in Jobs.keys():
      if existingjob.lower() == fullname.lower(): ## job names are case insensitive
        job = Jobs[existingjob]                 
        return jobs.JobLayout(job)
  except:
    pass

  # Perhaps we just don't have a rotated job yet
  if rotated:
    try:      
      for existingjob in Jobs.keys():
        if existingjob.lower() == jobname.lower(): ## job names are case insensitive
          job = Jobs[existingjob]
    except:
      raise RuntimeError, "Job name '%s' not found" % jobname
  else:
    raise RuntimeError, "Job name '%s' not found" % jobname

  # Make a rotated job
  job = jobs.rotateJob(job, rotated)
  Jobs[fullname] = job

  return jobs.JobLayout(job)

def parseJobSpec(spec, data):
  for jobspec in spec:
    if jobspec[0] in ('ts','comment'): continue

    assert jobspec[0] in ('paneljobspec','basicjobspec')
    if jobspec[0] == 'basicjobspec':
      namefield = jobspec[3][0]
      jobname = data[namefield[1]:namefield[2]]

      if len(jobspec[3]) > 1:
        rotationfield = jobspec[3][1]
        rotation = data[ rotationfield[1] + 1: rotationfield[2] ]
                
        if (rotation == "Rotate") or (rotation == "Rotate90"):
            rotated = 90
        elif rotation == "Rotate180":
            rotated = 180
        elif rotation == "Rotate270":
            rotated = 270
        else:
            raise RuntimeError, "Unsupported rotation: %s" % rotation

      else:
        rotated = 0

      return findJob(jobname, rotated)
    else:
      raise RuntimeError, "Matrix panels not yet supported"

def parseColSpec(spec, data):
  jobs = Col()

  for coljob in spec:
    if coljob[0] in ('ts','ws','comment'): continue

    assert coljob[0] == 'coljob'
    job = coljob[3][0]
    if job[0] in ('commentline','nullline'): continue
    
    assert job[0] in ('jobspec','rowspec')
    if job[0] == 'jobspec':
      jobs.addjob(parseJobSpec(job[3],data))
    else:
      jobs.addjob(parseRowSpec(job[3],data))

  return jobs

def parseRowSpec(spec, data):
  jobs = Row()

  for rowjob in spec:
    if rowjob[0] in ('ts','ws','comment'): continue

    assert rowjob[0] == 'rowjob'
    job = rowjob[3][0]
    if job[0] in ('commentline','nullline'): continue
    
    assert job[0] in ('jobspec','colspec')
    if job[0] == 'jobspec':
      jobs.addjob(parseJobSpec(job[3],data))
    else:
      jobs.addjob(parseColSpec(job[3],data))

  return jobs

def parseLayoutFile(fname):
  """config.Jobs is a dictionary of ('jobname', Job Object).

     The return value is a nested array. The primary dimension
     of the array is one row:

         [ Row1, Row2, Row3 ]

     Each row element consists of a list of jobs or columns (i.e.,
     JobLayout or Col objects).

     Each column consists of a list of either jobs or rows.
     These are recursive, so it can look like:

        [ 
          Row([JobLayout(), Col([ Row([JobLayout(), JobLayout()]),
                                     JobLayout()       ]),         JobLayout() ]),   # That was row 0
          Row([JobLayout(), JobLayout()])                                            # That was row 1
        ]

     This is a panel with two rows. In the first row there is
     a job, a column, and another job, from left to right. In the 
     second row there are two jobs, from left to right.

     The column in the first row has two jobs side by side, then
     another one above them.
  """

  try:
    fid = file(fname, 'rt')
  except Exception, detail:
    raise RuntimeError, "Unable to open layout file: %s\n  %s" % (fname, str(detail))

  data = fid.read()
  fid.close()
  parser = Parser(declaration, "file")

  # Replace all CR's in data with nothing, to convert DOS line endings
  # to unix format (all LF's).
  data = string.replace(data, '\x0D', '')

  tree = parser.parse(data)

  # Last element of tree is number of characters parsed
  if not tree[0]:
    raise RuntimeError, "Layout file cannot be parsed"

  if tree[2] != len(data):
    raise RuntimeError, "Parse error at character %d in layout file" % tree[2]

  Rows = []
  for rowspec in tree[1]:
    if rowspec[0] in ('nullline', 'commentline'): continue
    assert rowspec[0]=='rowspec'

    Rows.append(parseRowSpec(rowspec[3], data))

  return Rows

if __name__=="__main__":
  fid = file(sys.argv[1])
  testdata = fid.read()
  fid.close()

  parser = Parser(declaration, "file")
  import pprint
  pprint.pprint(parser.parse(testdata))
