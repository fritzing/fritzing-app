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

valuesAndColors_forResistors.py

Specific Script to generate a set of Resistors

"""

import sys
import os

E3Set = [100, 220, 470]
E6Set = [100, 150, 220, 330, 470, 680]
E12Set = [100, 120, 150, 180, 220, 270, 330, 390, 470, 560, 680, 820]
E24Set = [100, 110, 120, 130, 150, 160, 180, 200, 220, 240, 270, 300, 330, 360, 390, 430, 470, 510, 560, 620, 680, 750, 820, 910]

NamedSets = {'e3': E3Set, 'e6': E6Set, 'e12': E12Set, 'e24': E24Set}
 
SetMultiplication = [0.1, 1, 10, 100, 1000, 10000]
letterDict = { 'k': 1000, 'M': 1000000, 'G': 1000000000}

colorBands = ['black', 'brown', 'red', 'orange', 'yellow', 'green', 'blue', 'purple', 'grey', 'white']
colors = { 'black': (0, 0, 0),
		'brown': (138, 61, 6),
		'red': (196, 8, 8),
		'orange': (255, 77, 0),
		'yellow': (255, 213, 0),
		'green': (0, 163, 61),
		'blue': (0, 96, 182),
		'purple': (130, 16, 210),
		'grey': (140, 140, 140),
		'white': (255, 255, 255) }

def resistorSetForSetName(setName):
	if (setName[-3:].lower() == 'Set'.lower()):
		setName = setName[:-3]
	if NamedSets.has_key(setName.lower()):
		currentSet = NamedSets[setName]
		resistorSet = []
		for m in SetMultiplication:
			# if last of the set Multiplication
			if (m == SetMultiplication[len(SetMultiplication) - 1]):
				# only do the first
				value = currentSet[0] * m
				resistorSet.append(value)
			else:
				# do all in E set
				for r in currentSet:
					value = int(r * m)
					resistorSet.append(value)
		return resistorSet
	else:
		print >> sys.stderr, "No set with this name: %s (in %s)" % (setName, __file__)

def valueStringFromValue(value):
	short = False
	for k in reversed(letterDict.keys()):
		if value / letterDict[k]:
			short = True
			remainder = str(value % letterDict[k])
			remainder = remainder.rstrip('0')
			if remainder:
				return "%d.%d%s" % ((value / letterDict[k]), int(remainder), k)
			else:
				return "%d%s" % ((value / letterDict[k]), k)
			break
	if not short:
		return str(value)

def valueStringWithoutDots(value):
	return "_".join(valueStringFromValue(value).split("."))

def hexColorsForResistorValue(value):
	thirdBand = 0
	for m in range(6, 1, -1):
		if (value / pow(10, m)):
			thirdBand = m - 1
			break
	firstBand = colorBands[int(str(value)[:1])]
	secondBand = colorBands[int(str(value)[1:2])]
	thirdBand = colorBands[thirdBand]
	return ["#%02X%02X%02X" % (colors[c]) for c in (firstBand, secondBand, thirdBand)]

def main():
	resistorSet = resistorSetForSetName('E6Set')
	print resistorSet
	
	for r in resistorSet:
		print valueStringFromValue(r)
		print hexColorsForResistorValue(r)
	
	print "number of resistors in set: %d" % (len(resistorSet))
	for r in resistorSet:
		# Printing the value
		short = False
		for k in reversed(letterDict.keys()):
			if r / letterDict[k]:
				short = True
				remainder = str(r % letterDict[k])
				remainder = remainder.rstrip('0')
				print "spoken value: %d%s%s" % (r / letterDict[k], k, remainder)
				if remainder:
					print "official value: %d.%d%s" % ((r / letterDict[k]), int(remainder), k)
				else:
					print "official value: %d%s" % ((r / letterDict[k]), k)
				break
		if not short:
			print r
		# Printing the color bands
		thirdBand = 0
		for m in range(6, 1, -1):
			if (r / pow(10, m)):
				thirdBand = m - 1
				break
		firstBand = colorBands[int(str(r)[:1])]
		secondBand = colorBands[int(str(r)[1:2])]
		thirdBand = colorBands[thirdBand]
		print "%s, %s, %s" % (firstBand, secondBand, thirdBand)
		print "#%02X%02X%02X" % (colors[firstBand])
		print "#%02X%02X%02X" % (colors[secondBand])
		print "#%02X%02X%02X" % (colors[thirdBand])

	# for c in range(10):
	# 	print "#%02X%02X%02X" % (colors[colorBands[c]])


if __name__ == '__main__':
	main()
