#!/usr/bin/env python
# encoding: utf-8
"""
/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-09 Fachhochschule Potsdam - http://fh-potsdam.de

Fritzing is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Fritzing is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Fritzing.  If not, see <http://www.gnu.org/licenses/>.

********************************************************************

$Revision: 1595 $:
$Author: dirk@fritzing.org $:
$Date: 2008-11-21 11:04:36 +0100 (Fri, 21 Nov 2008) $

********************************************************************/

generator.py
version 1.2

CHANGES:
Now working with the new Fritzing file format.


This script generates the image svg files and a Fritzing part (.fzp) file for IC Packages.

Run it in the commandline with this:
	python generator.py [-f] [-n numberOfPins] [--narrow|--medium|--wide|--extrawide] TITLE ["text on chip"]
	
To get the full help about these options and arguments use
	python generator.py --help
	

"""

import sys, os, re, string
import getopt, ConfigParser, uuid
from datetime import date
from distutils.file_util import copy_file
from Cheetah.Template import Template

ARGUMENT_DEBUG = False
sL = {}
DEFAULT_N_PINS = 6
DEFAULT_NAME_OF_PIN = "Please Name This Pin"
SVG_BREADBOARD_TEMPLATE_FILE = 'dilpackage_svg.tmpl'
SVG_ICON_TEMPLATE_FILE = 'dilpackage_icon_svg.tmpl'
SVG_SCHEMATIC_TEMPLATE_FILE = 'dilpackage_schematic_svg.tmpl'
PARTFILE_TEMPLATE_FILE = 'dilpackage_fzp.tmpl'
FOOTPRINTS_DIRECTORY = "pcb"
availablePackages = {'narrow': [4, 6, 8, 14, 16, 18, 20, 22, 24, 28], 
					'medium': [8, 14, 16, 18, 20, 22, 24, 28], 
					'wide': [28, 32, 36, 40, 42, 44, 48], 
					'extra': [64]
					}
letterAppendixForType = {'narrow': 'N', 'medium': 'M', 'wide': 'W', 'extra': 'X'}

help_message = """
Fritzing DIL Package IC generator
version 1.2

This script creates Fritzing DIL-Package IC parts. It generates the svg images
and the fritzing part .fzp file for those parts in a named folder.

Usage:
python generator.py [-f] [-n numberOfPins] [--narrow|--medium|--wide|--extrawide] TITLE ["text on chip"]

Arguments:
-f
	-f, --force
	the flag to overwrite or not.
	If a folder with the name TITLE did already exist and the -f flag is
	not used the script will exit. Default overwrite is false.

-n numberOfPins
	-n #, --numberOfPins=#
	specifies the number of pins, where # must be an even integer, minimal 6.
	e.g. -n 14 (or --numberOfPins=14)
	This Creates a DIL package with 14 pins.
	Default number of pins is 6.

--narrow, --medium, --wide, --extrawide
 	argument indicating is whether to create a wide or narrow package.
	Has to be '--narrow', '--medium', '--wide' or '--extrawide'.
	Narrow means bridging 3 pins.     (300 mil)
	Medium means bridging 4 pins.     (400 mil)
	Wide means bridging 6 pins.       (600 mil)
	Extra wide means bridging 9 pins. (900 mil)
	Default is --narrow.

TITLE
	the title of the IC Package (e.g. 555Timer)
	A folder named TITLE will be created and all images and xml will be created therein.
	This argument must be given.
	
"text on chip"
	the text which is printed on the chip
	If more than one word is used, it needs to be in quotes. (")
	This argument is optional


For more information on how to create parts and how to use them in Fritzing,
please read the online documentation:
http://fritzing.org/learning/parts
"""

class Usage(Exception):
	def __init__(self, msg):
		self.msg = msg

def makeUUID():
	"creates an 8 character hex UUID"
	return str(uuid.uuid1())

def makeDate():
	"creates a date formatted as YYYY-MM-DD"
	return date.today().isoformat()

def getUserName():
	"gets the full user name if available"
	try:
		import pwd
		return pwd.getpwuid(os.getuid())[4]
	except:
		return os.getlogin()

def getTemplatefile(fileName):
	thisScriptPath = sys.argv[0]
	thisScriptFolder = os.path.split(thisScriptPath)[0]
	expectedTemplate = os.path.join(thisScriptFolder, fileName)
	if os.path.exists(expectedTemplate):
		return expectedTemplate
	else:
		print >> sys.stderr, "Script Package Incomplete: Couldn't find the svg template file: %s" % (fileName)
		sys.exit(2)

def getConfigFile(fileName):
	thisScriptPath = sys.argv[0]
	thisScriptFolder = os.path.split(thisScriptPath)[0]
	expectedTemplate = os.path.join(thisScriptFolder, fileName)
	if os.path.exists(expectedTemplate):
		return expectedTemplate
	else:
		print >> sys.stderr, "Script Package Incomplete: Couldn't load the config file: %s" % (fileName)
		sys.exit(2)

def getFootprintFile(fileName):
	thisScriptPath = sys.argv[0]
	thisScriptFolder = os.path.split(thisScriptPath)[0]
	expectedFootprint = os.path.join(thisScriptFolder, fileName)
	if os.path.exists(expectedFootprint):
		return expectedFootprint
	else:
		print >> sys.stderr, "Script Package Incomplete: Couldn't find the footprint file: %s" % (fileName)
		sys.exit(2)

def writeOutFileInSubDirectoryWithData(outputDir,  subDirectory, fileName, data):
	outputDir = os.path.join(outputDir, subDirectory)
	if not os.path.exists(outputDir):
		os.makedirs(outputDir)
	outfile = open(os.path.join(outputDir, fileName), "w")
	outfile.write(str(data))
	outfile.close()


def main(argv=None):
	if argv is None:
		argv = sys.argv
	try:
		try:
			opts, args = getopt.getopt(argv[1:], "hfn:", ["help", "force", "numberOfPins=", "narrow", "medium", "wide", "extrawide"])
		except getopt.error, msg:
			raise Usage(msg)

		# Default values
		forceFlag = False
		numberOfPins = 0 # 0 is not the default, but this is so we can check the validity later
		wideOrNarrow = 'narrow'
		textOnChip = ''

		# option processing
		for option, value in opts:
			if option in ("-h", "--help"):
				raise Usage(help_message)
			if option in ("-f", "--force"):
				forceFlag = True
			if option in ("-n", "--numberOfPins"):
				numberOfPins = value
			if option in ("--narrow", "--medium", "--wide", "--extrawide"):
				wideOrNarrow = option[2:]
				
		# argument processing
		if args:
			packageName = args.pop(0)
		else:
			# print >> sys.stderr, "No TITLE specified"
			raise Usage("Error: No TITLE specified")
		if args:
			# Get the text on the chip
			textOnChip = args.pop(0)
		if args:
			# More arguments than we are able to handle
			print >> sys.stderr, "\nWarning: Superfluous arguments are discarded: %s" % (args)
		
		# Check validity of number of pins
		pinsPattern = re.compile(r'\d+')
		if numberOfPins and pinsPattern.match(numberOfPins):
			numberOfPins = int(numberOfPins)
			if numberOfPins % 2:
				numberOfPins += 1
		else:
			numberOfPins = DEFAULT_N_PINS

		# Get the config
		config = ConfigParser.ConfigParser()
		config.readfp(open(getConfigFile('defaults.cfg')))
		
		print "packageName : %s" % packageName
		print "numberOfPins: %s" % numberOfPins
		print "forceFlag   : %s" % forceFlag
		print "textOnChip  : %s" % textOnChip
		print "wideMediumOrNarrow: %s" % wideOrNarrow
	
		# Test if the number of pins is an existing DIL Package
		if not (numberOfPins in availablePackages[wideOrNarrow]):
			print >> sys.stderr, """\nWarning: Number of pins is wrong for DIL Package:\n\tDIL packages with %d pins do not exits in the %s package.\n\tThe script will continue, but you might not be able to create a correct PCB\n\twith the resulting part.""" % (numberOfPins, wideOrNarrow)

		if (ARGUMENT_DEBUG):
			# Exit For debugging:
			sys.exit()
	
		# Check if the directory did not already exist
		try:
			os.mkdir(packageName)
		except OSError, error:
			if not forceFlag:
				print >> sys.stderr, "\nError: The directory %s already exists. Script aborted. Use -f to override\n\t for help use --help" % (packageName)
				return 2
		output = packageName
		
		# Setup default values
		metaData = {}
		metaData['moduleID'] = makeUUID()
		metaData['date'] = makeDate()
		metaData['author'] = getUserName()
		sL['metaData'] = metaData
		sL['taxonomy'] = '.'.join((config.get('PartMetadata', 'genus'), packageName))
		sL['description'] = config.get('PartMetadata', 'description')
		sL['label'] = config.get('PartMetadata', 'label')
		sL['reference'] = config.get('PartMetadata', 'reference')
		sL['title'] = packageName
		# make valid fileName from the packageName
		valid_chars = "-_.() %s%s" % (string.ascii_letters, string.digits)
		shortFileName = ''.join(c for c in packageName if c in valid_chars)
		shortFileName = shortFileName.replace(" ", "_")
		shortFileName = shortFileName.lower()

		# Decide Wide or Narrow package
		if (wideOrNarrow == 'narrow'):
			sL['height'] = 33
		elif (wideOrNarrow == 'medium'):
			sL['height'] = 43
		elif (wideOrNarrow == 'wide'):
			sL['height'] = 63
		else:
			# extra wide
			sL['height'] = 93
		sL['heightInch'] = "%.2fin" % (sL['height'] / 100.0) # the template svg is 100 ppi 
		
		# For schematic
		sL['schematicWidth'] = 110
		# Prepare the pins for the breadboard and schematic
		pinList = []
		for num in range(numberOfPins):
			aPin = {}
			aPin['id'] = num
			aPin['description'] = DEFAULT_NAME_OF_PIN
			aPin['number'] = num + 1
			aPin['name'] = "Pin %d" % (num + 1)
			aPin['connectorPinName'] = "connector%dpin" % (num)
			aPin['connectorPadName'] = "connector%dpad" % (num)
			aPin['connectorTerminalName'] = "connector%dterminal" % (num)
			if num < (numberOfPins / 2):
				# Bottom row (breadboard) 
				aPin['startX'] = (num * 10) + 3.5
				aPin['startPinY'] = sL['height'] - 4.34 # 28.66
				aPin['startTermY'] = sL['height'] - 3 # 30
				aPin['startLegY'] = sL['height'] - 6.68 # 26.32
				# and left row (schematic)
				aPin['schematicPinY'] = (num * 26) + 15.15
				aPin['schematicLegY'] = (num * 26) + 16.15
				aPin['schematicTextY'] = (num * 26) + 13
				aPin['schematicPinX'] = 1.15
				aPin['schematicTermX'] = 1.15
				aPin['schematicLegX1'] = 1.15
				aPin['schematicLegX2'] = 32.15
				aPin['schematicTextX'] = 28
				aPin['schematicTextAnchor'] = "end"
			else:
				# Top row (breadboard)
				aPin['startX'] = ((numberOfPins - num) * 10) - 6.5 # 3.5 
				aPin['startPinY'] = 0
				aPin['startTermY'] = 0
				aPin['startLegY'] = 0
				# and right row (schematic)
				aPin['schematicPinY'] = ((numberOfPins - num - 1) * 26) + 15.15
				aPin['schematicLegY'] = ((numberOfPins - num - 1) * 26) + 16.15
				aPin['schematicTextY'] = ((numberOfPins - num - 1) * 26) + 13
				aPin['schematicPinX'] = sL['schematicWidth'] + 53.15
				aPin['schematicTermX'] = sL['schematicWidth'] + 61.15
				aPin['schematicLegX1'] = sL['schematicWidth'] + 32.15
				aPin['schematicLegX2'] = sL['schematicWidth'] + 63.15
				aPin['schematicTextX'] = sL['schematicWidth'] + 36
				aPin['schematicTextAnchor'] = "start"
			pinList.append(aPin) 
		sL['pinList'] = pinList

		# Breadboard & Icon Measurements
		sL['width'] = 10 * (numberOfPins / 2)
		sL['widthInch'] = "%.2fin" % (sL['width'] / 100.0) # the template svg is 100 ppi 
		sL['nMiddlePins'] = (numberOfPins / 2) - 2
		# Schematic Measurements
		sL['schematicWidthViewBox'] = sL['schematicWidth'] + 64.3 # 84.3px
		sL['schematicWidthPx'] = "%.1fpx" % (sL['schematicWidthViewBox'])
		sL['schematicHeightViewBox'] = (26 * (numberOfPins / 2)) + 6.3 # 174.3px
		sL['schematicHeightPx'] = "%.1fpx" % (sL['schematicHeightViewBox']) 
		sL['schematicHeight'] = (26 * (numberOfPins / 2)) + 4 # 82
		sL['schematicMiddleX'] = (sL['schematicWidth'] / 2) + 32.15 # 87.15 
		sL['schematicMiddleY'] = (sL['schematicHeight'] / 2) + 7 # 48
		# General
		sL['textOnChip'] = textOnChip
		sL['breadboardFile'] = "%s.svg" % (shortFileName)
		sL['iconFile'] = "%s_icon.svg" % (shortFileName)
		sL['schematicFile'] = "%s_schematic.svg" % (shortFileName)

		# Breadboard svg file
		if getTemplatefile(SVG_BREADBOARD_TEMPLATE_FILE):
			print >> sys.stdout, "\nWriting out the breadboard svg file ..."
			t = Template(file=getTemplatefile(SVG_BREADBOARD_TEMPLATE_FILE), searchList = [sL])
			writeOutFileInSubDirectoryWithData(output, "breadboard", sL['breadboardFile'], t)
		# Icon svg file
		if getTemplatefile(SVG_ICON_TEMPLATE_FILE):
			print >> sys.stdout, "Writing out the icon svg file ..."
			t = Template(file=getTemplatefile(SVG_ICON_TEMPLATE_FILE), searchList = [sL])
			writeOutFileInSubDirectoryWithData(output, "icon", sL['iconFile'], t)
		# Schematic svg file
		if getTemplatefile(SVG_SCHEMATIC_TEMPLATE_FILE):
			print >> sys.stdout, "Writing out the schematic svg file ..."
			t = Template(file=getTemplatefile(SVG_SCHEMATIC_TEMPLATE_FILE), searchList = [sL])
			writeOutFileInSubDirectoryWithData(output, "schematic", sL['schematicFile'], t)
		# Pick the pcb svg file
		sL['footprint'] = ""
		if (numberOfPins in availablePackages[wideOrNarrow]):
			sL['footprint'] = "pcb/DIP%d%s.svg" % (numberOfPins, letterAppendixForType[wideOrNarrow])
		# fzp part file
		if getTemplatefile(PARTFILE_TEMPLATE_FILE):
			print >> sys.stdout, "Writing out the Fritzing part fzp file ..."
			t = Template(file=getTemplatefile(PARTFILE_TEMPLATE_FILE), searchList = [sL])
			# TODO make this fzp
			writeOutFileInSubDirectoryWithData(output, "", "%s.fzp" % (shortFileName), t)
	
		print "Ready"
		print """----------------------------------------------------------------

Now your new part is created in %s. Next you will have to install these files into their
locations in Fritzing. (See the documentation or whatever ... We have to come up with an
easy install mechanism for this)
And TEMPORALILY, WHILE WE DON'T YET SUPPORT OUR OWN FONTS IN FRITZING' SVGs, if you dare
do the following:
1. open the breadboard, icon & schematic svg files in a texteditor
2. cut (cmd X) the x,y,width, height and viewbox properties from the svg tag
3. open the files in illustrator
4. convert the fonts into outlines
5. save the files
6. reload the files in your texteditor
7. paste the x,y,width, height and viewbox properties back in (cmd V)
8. sorry for this mess. :-(
Then finally open the just created part in Fritzings parts editor and change the part
identification: the correct taxonomy, the right properties and so on.
Also make the names of the pins specific to your chip, so you can easilily identify them.""" % output

	except Usage, err:
		if (err.msg == help_message):
			# More concise help message (not display as error)
			print >> sys.stderr, str(err.msg)
		else:
			print >> sys.stderr, sys.argv[0].split("/")[-1] + ": " + str(err.msg)
			print >> sys.stderr, "\t for help use --help"
		return 2


if __name__ == "__main__":
	sys.exit(main())
