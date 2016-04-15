#!/usr/bin/env python
# encoding: utf-8
"""
/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-08 Fachhochschule Potsdam - http://fh-potsdam.de

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

resistorSetGenerator.py

"""

import sys, getopt, ConfigParser, os, uuid
from datetime import date
from Cheetah.Template import Template
from xml.dom import minidom


import valuesAndColors_forResistors

help_message = """
Usage:
	resistorSetGenerator.py -s [Resistor Set] -o [Output Dir] -b [Existing Resistor Bin]
	
	Resistor Set          - the named set of standard resistors that should be generated
					        Possible values are: E3, E6, E12, E24 for the E3Set, E6Set etc.
					
	Output Dir            - the location where the output files are written
	
	Existing Resistor Bin - the Fritzing bin file (.fzb) from which this script should try
	                        to find the matching moduleID's. If no bin is given, all new
	                        resistors with new moduleID's will be generated. 
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

def getExistingResistorsFromBin(filename):
	if os.path.exists(filename):
		# Load the bin file as xml
		try:
			binFile = open(filename)
			xmldoc = minidom.parse(binFile).documentElement
			binFile.close()
		except (IOError, OSError):
			print >> sys.stderr, "Couldn't open bin: %s " % (fileName)
			sys.exit(2)
		# See if the bin got any resistors
		foundResistors = []
		for aPart in xmldoc.getElementsByTagName("instance"):
			# print aPart
			# print aPart.attributes.keys()
			if aPart.attributes.has_key("path") and aPart.attributes.has_key("moduleIdRef"):
				partFilename = os.path.split(aPart.attributes["path"].value)[1]
				if (partFilename.startswith("resistor_")):
					# There is a resistor. Strip of the resistor_ and .fzp part
					resistorValueString = os.path.splitext(partFilename)[0][9:]
					thisResistor = {}
					thisResistor['value'] = resistorValueString.replace("_", ".")
					thisResistor['moduleID'] = aPart.attributes["moduleIdRef"].value
					foundResistors.append(thisResistor)
			else:
				print "Couldn't determine following part in bin, because it has no path and moduleIdRef property: %s" % (aPart.toxml())
		return foundResistors
	else:
		print >> sys.stderr, "No Fritzing bin file: %s found" % (fileName)
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
			opts, args = getopt.getopt(argv[1:], "ho:s:b:", ["help", "output=", "set=", "bin="])
		except getopt.error, msg:
			raise Usage(msg)
		
		output = "."
		verbose = False
		namedSet = None
		existingBin = None
	
		# option processing
		for option, value in opts:
			if option in ("-h", "--help"):
				raise Usage(help_message)
			if option in ("-o", "--output"):
				output = value
			if option in ("-s", "--set"):
				namedSet = value
			if option in ("-b", "--bin"):
				existingBin = value
		
		if(not(namedSet) or not(output)):
			# print >> sys.stderr, "No Resistor Set or Output Dir specified"
			raise Usage("Error: No Resistor Set or Output Dir specified")
		
		config = ConfigParser.ConfigParser()
		config.readfp(open(getConfigFile('defaults.cfg')))
		
		resistorSet = valuesAndColors_forResistors.resistorSetForSetName(namedSet)
		print "number of resistors in set: %d" % (len(resistorSet))
		
		# Analyzing bin for existing resistor moduleID's
		existingResistors = []
		if existingBin:
			print "analyzing bin for existing resistor moduleID's ..."
			existingResistors = getExistingResistorsFromBin(existingBin)
			existingResistorsValues = [r['value'] for r in existingResistors]
			
		resistorBin = []
		skipThisResistor = False
		for r in resistorSet:
			sL = {}
			metaData = {}
			resistorInBin = {}
			resistanceString = valuesAndColors_forResistors.valueStringFromValue(r)
			# check if this resistor doesn't have a moduleID already 
			if existingResistors and (resistanceString in existingResistorsValues):
				if (not config.getboolean('ExistingResistors', 'regenerateExitsingParts')):
					# Don't regnerate this resistor (leave it out the new set)
					print "Skipping %s resistor, because it is already in the existing bin" % (resistanceString)
					skipThisResistor = True
				i = existingResistorsValues.index(resistanceString)
				metaData['moduleID'] = existingResistors[i]['moduleID']
			else:
				metaData['moduleID'] = makeUUID()
			if (not skipThisResistor):
				metaData['date'] = makeDate()
				metaData['author'] = getUserName()
				sL['metaData'] = metaData
				sL['resistance'] = resistanceString
				# sL['taxonomy'] = "discreteParts.resistor.%s" % (resistanceString)
				# The resistance with the dot notation in the taxonomy might be a problem, so we'd better do:
				sL['taxonomy'] = "discreteParts.resistor.%d" % (r)
				sL['colorBands'] = valuesAndColors_forResistors.hexColorsForResistorValue(r)
				# Breadboard svg
				svg = Template(file=getTemplatefile("resistor_breadboard_svg.tmpl"), searchList = [sL])
				writeOutFileInSubDirectoryWithData(output, "breadboard", "resistor_%s.svg" % (resistanceString), svg)
				# Icon svg
				svg = Template(file=getTemplatefile("resistor_icon_svg.tmpl"), searchList = [sL])
				writeOutFileInSubDirectoryWithData(output, "icon", "resistor_icon_%s.svg" % (resistanceString), svg)
				# Partfile fzp
				fzpfilename = "resistor_%s.fzp" % valuesAndColors_forResistors.valueStringWithoutDots(r)
				fzp = Template(file=getTemplatefile("resistor_fzp.tmpl"), searchList = [sL])
				writeOutFileInSubDirectoryWithData(output, "fzp_files", fzfilename, fzp)
				# For in the bin
				resistorInBin['moduleID'] = metaData['moduleID']
				resistorInBin['filename'] = fzpfilename
				resistorBin.append(resistorInBin)
		# Bin file
		if (config.getboolean('OutputBin', 'outputBin')):
			sL = {}
			sL['title'] = "%s Resistor Set" % (namedSet)
			partsDir = config.get('OutputBin', 'partsDir')
			for r in resistorBin:
				r['filepath'] = os.path.join(partsDir, r['filename'])
			sL['resistors'] = resistorBin
			fzb = Template(file=getTemplatefile("bin_fzb.tmpl"), searchList = [sL])
			writeOutFileInSubDirectoryWithData(output, "bin", "%sBin.fzb" % (namedSet), fzb)
	
	except Usage, err:
		print >> sys.stderr, sys.argv[0].split("/")[-1] + ": " + str(err.msg)
		print >> sys.stderr, "\t for help use --help"
		return 2


if __name__ == "__main__":
	sys.exit(main())
	