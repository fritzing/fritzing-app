#!/usr/bin/env python
import sys
import glob
import os

from distutils.core import setup, Extension
import distutils.sysconfig

from gerbmerge.gerbmerge import VERSION_MAJOR, VERSION_MINOR

if sys.version_info < (2,4,0):
  print '*'*73
  print 'GerbMerge version %d.%d requires Python 2.4 or higher' % (VERSION_MAJOR, VERSION_MINOR)
  print '*'*73
  sys.exit(1)

if 0:
  for key,val in distutils.sysconfig.get_config_vars().items():
    print key
    print '***********************'
    print '  ', val
    print
    print

  sys.exit(0)

SampleFiles = glob.glob('testdata/*')
DocFiles = glob.glob('doc/*')
AuxFiles = ['README', 'COPYING']

if sys.platform == 'win32' or ('bdist_wininst' in sys.argv):
  #DestLib = distutils.sysconfig.get_config_var('prefix')
  #DestDir = os.path.join(DestLib, 'gerbmerge')
  #BinDir = DestLib
  DestLib = '.'
  DestDir = os.path.join(DestLib, 'gerbmerge')
  BinFiles = ['misc/gerbmerge.bat']
  BinDir = '.'
else:
  DestLib = distutils.sysconfig.get_config_var('LIBDEST')
  DestDir = os.path.join(DestLib, 'gerbmerge')
  BinFiles = ['misc/gerbmerge']
  BinDir = distutils.sysconfig.get_config_var('BINDIR')  

  # Create top-level invocation program
  fid = file('misc/gerbmerge', 'wt')
  fid.write( \
  r"""#!/bin/sh
python %s/site-packages/gerbmerge/gerbmerge.py $*
  """ % DestLib)
  fid.close()

dist=setup (name = "gerbmerge",
       license = "GPL",
       version = "%d.%d" % (VERSION_MAJOR, VERSION_MINOR),
      long_description=\
r"""GerbMerge is a program that combines several Gerber
(i.e., RS274-X) and Excellon files into a single set
of files. This program is useful for combining multiple
printed circuit board layout files into a single job.

To run the program, invoke the Python interpreter on the
gerbmerge.py file. On Windows, if you installed GerbMerge in
C:/Python24, for example, open a command window (DOS box)
and type:
    C:/Python24/gerbmerge.bat

For more details on installation or running GerbMerge, see the
URL below.
""",
       description = "Merge multiple Gerber/Excellon files",
       author = "Andrew Sterian",
       author_email = "steriana@claymore.engineer.gvsu.edu",
       url = "http://claymore.engineer.gvsu.edu/~steriana/Python",
       packages = ['gerbmerge'],
       platforms = ['all'],
       data_files = [ (DestDir, AuxFiles), 
                      (os.path.join(DestDir,'testdata'), SampleFiles),
                      (os.path.join(DestDir,'doc'), DocFiles),
                      (BinDir, BinFiles) ]
)

do_fix_perms = 0
if sys.platform != "win32":
  for cmd in dist.commands:
   if cmd[:7]=='install':
    do_fix_perms = 1
    break

if do_fix_perms:
  # Ensure package files and misc/help files are world readable-searchable.
  # Shouldn't Distutils do this for us?
  print 'Setting permissions on installed files...',
  try:
    def fixperms(arg, dirname, names):
      os.chmod(dirname, 0755)
      for name in names:
        fullname = os.path.join(dirname, name)
        if os.access(fullname, os.X_OK):
          os.chmod(fullname, 0755)
        else:
          os.chmod(fullname, 0644)

    os.path.walk(DestDir, fixperms, 1)
    os.path.walk(os.path.join(DestLib, 'site-packages/gerbmerge'), fixperms, 1)

    os.chmod(os.path.join(BinDir, 'gerbmerge'), 0755)
    print 'done'
  except:
    print 'FAILED'
    print
    print '*** Please verify that the installed files have correct permissions. On'
    print "*** systems without permission flags, you don't need to"
    print '*** worry about it.' 

if cmd[:7]=='install':
  print
  print '******** Installation Complete ******** '
  print
  print 'Sample files and documentation have been installed in:'
  print '   ', DestDir
  print
  print 'A shortcut to starting the program has been installed as:'
  print '   ', os.path.join(BinDir, 'gerbmerge')
  print
