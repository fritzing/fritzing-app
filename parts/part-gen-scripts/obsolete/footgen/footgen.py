#!/usr/bin/python
# package.py
# Copyright (C) 2005-2007 Darrell Harmon
# Generates footprints for PCB from text description
# The GPL applies only to the python scripts.
# the output of the program and the footprint definition files
# are public domain
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

# Added dih patch from David Carr - 8/28/2005 Darrell Harmon

# 1/6/2007
# Bugfix from David Carr to correct row and col reversal in BGA omitballs
# Several improvements sent to me by Peter Baxendale, with some modification
# by DLH: Square pin 1 on all through hole, optional inside silkscreen on
# SO parts with diagonal corner for pin 1, to enable use silkstyle = "inside",
# disabled by default or use silkstyle = "none"

# 9/28/2008
# Added patch by David Carr to add silkstyle="corners" option on qfp
# set silkboxwidth and silkboxheigh if using this style

# 9/28/2008
# Added refdesx and refdesy attributes which allow the reference designator to be positioned as desired
# Previously the reference designator was located at the origin. This is still the default.

# 9/29/2008
# Allow negative numbers in footprint definition file

# 10/22/2008
# added "skew" option to SO
# take out debug print for expandedlist

# 2/9/2009 DLH
# allow exposed pads on SO parts
# added "rect" option
# if "rect" is set, it will create a rectangular exposed pad. It must be >= to the ep value.
# It sets the height, and is only available in so at this time.

# list of attributes to be defined in file
def defattr():
    return [["elementdir",""],
            ["part",""],\
            ["type",""],\
            ["pinshigh", 0],\
            ["pins",0],\
            ["rows",0],\
            ["cols",0],\
            ["pitch",0],\
            ["silkoffset",0],\
            ["silkwidth",0],\
            ["silkboxwidth",0],\
            ["silkboxheight",0],\
            ["silkstyle",""],\
            ["omitballs",""],\
            ["paddia",0],\
            ["dia",0],\
            ["maskclear",0],\
            ["polyclear",0],\
            ["ep",0],\
            ["rect",0],\
            ["padwidth",0],\
            ["padheight",0],\
            ["tabwidth",0],\
            ["tabheight",0],\
            ["width",0],\
            ["height",0],\
            ["refdesx",0],\
            ["refdesy",0],\
            ["skew",0],\
            ["rect",0],\
            ["drill",0]]

# BGA row names 20 total
rowname = ['A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'J',\
           'K', 'L', 'M', 'N', 'P', 'R', 'T', 'U', 'V', 'W', 'Y']

# generate ball name from row and column (for BGA)
def ballname(col, row):
    ball = ""
    # can handle up to ball YY
    while row>20:
        ball=rowname[(row/20) - 1]
        row = row%20
    return ball+rowname[row-1]+str(col)

# find X and Y position of ball from name
def ballpos(ball_name):
    col = ""
    row = 0
    for char in ball_name:
        if char.isdigit():
            col = col+char
        if char.isalpha():
            row = row * 20
            count = 1
            for val in rowname:
                if val==char:
                    row = row + count
                count = count +1
    col = int(col)
    return [col,row]
    
# expand B1:C5 to list of balls
def expandexpr(inputbuf):
    # single ball has no :
    if inputbuf.find(":")==-1:
        return " \"" + inputbuf + "\""
    # multiple balls
    expanded = ""
    pos1 = ballpos(inputbuf[:inputbuf.find(":")])
    pos2 = ballpos(inputbuf[inputbuf.find(":")+1:])
    # Make sure order is increasing 
    if pos1[0]>pos2[0]:
        tmp = pos1[0]
        pos1[0] = pos2[0]
        pos2[0] = tmp
    if pos1[1]>pos2[1]:
        tmp = pos1[1]
        pos1[1] = pos2[1]
        pos2[1] = tmp
    for col in range(pos1[0], pos2[0]+1):
        for row in range(pos1[1], pos2[1]+1):
            expanded = expanded + " " +"\""+ ballname(col, row)+"\""
    return expanded

# expand list of balls to omit
def expandomitlist(omitlist):
    expandedlist = ""
    tmpbuf = ""
    if not omitlist:
        return ""
    for character in omitlist:
        if character.isalpha() or character.isdigit() or character==":":
            tmpbuf = tmpbuf + character
        elif character == ',':
            expandedlist = expandedlist + expandexpr(tmpbuf)
            tmpbuf = ''
    #last value will be in tmpbuf
    expandedlist = expandedlist + expandexpr(tmpbuf)
    # debug: print expandedlist
    return expandedlist
    
def findattr(attrlist, name):
    for attribute in attrlist:
        if attribute[0]==name:
            return attribute[1]
    print "Attribute %s not found" % name
    print attrlist
    return 0

def changeattr(attrlist, name, newval):
    for attribute in attrlist:
        if attribute[0]==name:
            attribute[1] = newval
            return
    print "Attribute %s not found" % name
    print attrlist
    return 0

# Format of pad line:
# Pad[-17500 -13500 -17500 -7000 2000 1000 3000 "1" "1" 0x00000180]
# X1 Y1 X2 Y2 Width polyclear mask ....
def pad(x1, y1, x2, y2, width, clear, mask, pinname, shape):
    if shape=="round":
        flags = "0x00000000"
    else:
        flags = "0x00000100"
    return "\tPad[%d %d %d %d %d %d %d \"%s\" \"%s\" %s]\n"\
           % (x1, y1, x2, y2, width, clear*2, mask+width, pinname, pinname, flags)

def padctr(x,y,height,width,clear,mask,pinname):
    linewidth = min(height,width)
    linelength = abs(height-width)
    if height>width:
        #vertcal pad
        x1 = x
        x2 = x
        y1 = y - linelength/2
        y2 = y + linelength/2
    else:
        #horizontal pad
        x1 = x - linelength/2
        x2 = x + linelength/2
        y1 = y
        y2 = y
    return pad(x1,y1,x2,y2,linewidth,clear,mask,pinname,"square")        

# ball is a zero length round pad
def ball(x, y, dia, clear, mask, name):
    return pad(x, y, x, y, dia, clear, mask, name, "round")

# draw silkscreen line
def silk(x1, y1, x2, y2, width):
    return "\tElementLine [%d %d %d %d %d]\n" % (x1, y1, x2, y2, width)
# draw silkscreen box
def box(x1, y1, x2, y2, width):
    return silk(x1,y1,x2,y1,width)+\
           silk(x2,y1,x2,y2,width)+\
           silk(x2,y2,x1,y2,width)+\
           silk(x1,y2,x1,y1,width)

# draw inside box (PRB)
# draws silkscreen box for SO or QFP type parts with notched pin #1 corner
def insidebox(x1, y1, x2, y2, width):
    return silk(x1,y1,x2,y1,width)+\
           silk(x2,y1,x2,y2+1000,width)+\
           silk(x2,y2+1000,x2+1000,y2,width)+\
           silk(x2+1000,y2,x1,y2,width)+\
           silk(x1,y2,x1,y1,width)

def bga(attrlist):
    # definitions needed to generate bga
    cols = findattr(attrlist, "cols")
    rows = findattr(attrlist, "rows")
    pitch = findattr(attrlist, "pitch")
    balldia = findattr(attrlist, "dia")
    polyclear = findattr(attrlist, "polyclear")
    maskclear = findattr(attrlist, "maskclear")
    silkwidth = findattr(attrlist, "silkwidth")
    silkoffset = findattr(attrlist, "silkoffset")
    silkboxwidth = findattr(attrlist, "silkboxwidth")
    silkboxheight = findattr(attrlist, "silkboxheight")
    refdesx = findattr(attrlist, "refdesx")
    refdesy = findattr(attrlist, "refdesy")
    omitlist = expandomitlist(findattr(attrlist, "omitballs"))
    width = (cols-1)*pitch+balldia
    height = (rows-1)*pitch+balldia
    # ensure silkscreen doesn't overlap balls
    if silkboxwidth<width or silkboxheight<height:
        silkboxx = width/2 + silkoffset
        silkboxy = height/2 + silkoffset
    else:
        silkboxx = silkboxwidth/2
        silkboxy = silkboxheight/2
    bgaelt = "Element[0x00000000 \"\" \"\" \"\" 1000 1000 %d %d 0 100 0x00000000]\n(\n" % (refdesx, -4000 - refdesy)
    # silkscreen outline
    bgaelt = bgaelt + box(silkboxx,silkboxy,-silkboxx,-silkboxy,silkwidth)
    # pin 1 indicator 1mm long tick
    bgaelt = bgaelt + silk(-silkboxx, -silkboxy, -(silkboxx+3940), -(silkboxy+3940), silkwidth)
    # position of ball A1
    xoff = -int((cols+1)*pitch/2)
    yoff = -int((rows+1)*pitch/2)
    for row in range(1, rows+1):
        for col in range(1, 1+cols):
            # found bug here row,col reversed
            if omitlist.find("\""+ballname(col,row)+"\"")==-1:
                x = xoff + (pitch*col)
                y = yoff + (pitch*row)
                bgaelt = bgaelt + ball(x, y, balldia, polyclear, maskclear, ballname(col,row))
    return bgaelt+")\n"

# draw a row of square pads
# pos is center position
# whichway can be up down left right
def rowofpads(pos, pitch, whichway, padlen, padheight, startnum, numpads, maskclear, polyclear):
    pads = ""
    rowlen = pitch * (numpads - 1)
    if whichway == "down":
        x = pos[0]
        y = pos[1] - rowlen/2
        for padnum in range (startnum, startnum+numpads):
            pads = pads + padctr(x,y,padheight,padlen,polyclear,maskclear,str(padnum))
            y = y + pitch
    elif whichway == "up":
        x = pos[0]
        y = pos[1] + rowlen/2
        for padnum in range (startnum, startnum+numpads):
            pads = pads + padctr(x,y,padheight,padlen,polyclear,maskclear,str(padnum))
            y = y - pitch
    elif whichway == "right":
        x = pos[0] - rowlen/2
        y = pos[1]
        for padnum in range (startnum, startnum+numpads):
            pads = pads + padctr(x,y,padheight,padlen,polyclear,maskclear,str(padnum))
            x = x + pitch
    elif whichway == "left":
        x = pos[0] + rowlen/2
        y = pos[1]
        for padnum in range (startnum, startnum+numpads):
            pads = pads + padctr(x,y,padheight,padlen,polyclear,maskclear,str(padnum))
            x = x - pitch
    return pads

def qfp(attrlist):
    pins = findattr(attrlist, "pins")
    pinshigh = findattr(attrlist, "pinshigh")
    padwidth = findattr(attrlist, "padwidth")
    padheight = findattr(attrlist, "padheight")
    pitch = findattr(attrlist, "pitch")
    width = findattr(attrlist, "width")
    height = findattr(attrlist, "height")
    polyclear = findattr(attrlist, "polyclear")
    maskclear = findattr(attrlist, "maskclear")
    silkwidth = findattr(attrlist, "silkwidth")
    silkoffset = findattr(attrlist, "silkoffset")
    silkboxwidth = findattr(attrlist, "silkboxwidth")
    silkboxheight = findattr(attrlist, "silkboxheight")
    silkstyle = findattr(attrlist, "silkstyle")
    ep = findattr(attrlist, "ep")
    refdesx = findattr(attrlist, "refdesx")
    refdesy = findattr(attrlist, "refdesy")
    qfpelt = "Element[0x00000000 \"\" \"\" \"\" 1000 1000 %d %d 0 100 0x00000000]\n(\n" % (refdesx, -4000 - refdesy)
    if pinshigh==0:
        pinshigh = pins/4
        pinswide = pins/4
        height = width
    else:
        pinswide = (pins-2*pinshigh)/2
    if pinshigh:
        # draw left side
        qfpelt = qfpelt + rowofpads([-(width+padwidth)/2,0], pitch, "down", padwidth,\
                                    padheight, 1, pinshigh, maskclear, polyclear)
        # draw right side
        qfpelt = qfpelt + rowofpads([(width+padwidth)/2,0], pitch, "up", padwidth,\
                                    padheight, pinshigh+pinswide+1, pinshigh,\
                                    maskclear, polyclear)
    if pinswide:
        # draw bottom
        qfpelt = qfpelt + rowofpads([0,(height+padwidth)/2], pitch, "right", padheight,\
                                    padwidth, pinshigh+1, pinswide, maskclear, polyclear)
        # draw top
        qfpelt = qfpelt + rowofpads([0,-(height+padwidth)/2], pitch, "left", padheight,\
                                    padwidth, 2*pinshigh+pinswide+1, pinswide, maskclear, polyclear)
    # exposed pad packages:
    if ep:
        qfpelt =qfpelt + pad(0,0,0,0,ep,polyclear,maskclear,str(pins+1),"square")
    if silkstyle == "inside":
        x = width/2 - silkoffset
        y = height/2 - silkoffset
        qfpelt = qfpelt + box(x,y,-x,-y,silkwidth)
        qfpelt = qfpelt + silk(-x,-y,-(x+3940),-(y+3940),silkwidth)
    elif silkstyle == "outside":
        x = (width+2*padwidth)/2 + silkoffset
        y = (height+2*padwidth)/2 + silkoffset
        qfpelt = qfpelt + box(x,y,-x,-y,silkwidth)
        qfpelt = qfpelt + silk(-x,-y,-(x+3940),-(y+3940),silkwidth)
    elif silkstyle == "corners":
        x_stop = 0.5*pitch*(pinswide-1) + .5*padheight + silkwidth
        y_stop = 0.5*pitch*(pinshigh-1) + .5*padheight + silkwidth
        x = silkboxwidth/2 + silkwidth/2
        y = silkboxheight/2 + silkwidth/2
        #UL
        qfpelt = qfpelt + silk(-x,-y,-x_stop,-y,silkwidth)
        qfpelt = qfpelt + silk(-x,-y,-x,-y_stop,silkwidth)
        #UR
        qfpelt = qfpelt + silk(x,-y,x_stop,-y,silkwidth)
        qfpelt = qfpelt + silk(x,-y,x,-y_stop,silkwidth)
        #LL
        qfpelt = qfpelt + silk(-x,y,-x_stop,y,silkwidth)
        qfpelt = qfpelt + silk(-x,y,-x,y_stop,silkwidth)
        #LR
        qfpelt = qfpelt + silk(x,y,x_stop,y,silkwidth)
        qfpelt = qfpelt + silk(x,y,x,y_stop,silkwidth)
        #Pin 1
        qfpelt = qfpelt + silk(-x,-y,-(x+3940),-(y+3940),silkwidth)

    return qfpelt+")\n"

# def rowofpads(pos, pitch, whichway, padlen, padheight, startnum, numpads, maskclear, polyclear):
# uses pins, padwidth, padheight, pitch
def so(attrlist):
    pins = findattr(attrlist, "pins")
    pitch = findattr(attrlist, "pitch")
    width = findattr(attrlist, "width")
    padwidth = findattr(attrlist, "padwidth")
    padheight = findattr(attrlist, "padheight")
    polyclear = findattr(attrlist, "polyclear")
    maskclear = findattr(attrlist, "maskclear")
    silkwidth = findattr(attrlist, "silkwidth")
    silkoffset = findattr(attrlist, "silkoffset")
    silkboxheight = findattr(attrlist, "silkboxheight")
    silkstyle = findattr(attrlist, "silkstyle")
    refdesx = findattr(attrlist, "refdesx")
    refdesy = findattr(attrlist, "refdesy")
    skew = findattr(attrlist, "skew")
    ep = findattr(attrlist, "ep")
    rect = findattr(attrlist, "rect")
    if pins % 2:
        print "Odd number of pins: that is a problem"
        print "Skipping " + findattr(attrlist, "type")
        return ""
    rowpos = (width+padwidth)/2
    soelt = "Element[0x00000000 \"\" \"\" \"\" 1000 1000 %d %d 0 100 0x00000000]\n(\n" % (refdesx, -4000 - refdesy)
    soelt = soelt + rowofpads([-rowpos,-skew], pitch, "down", padwidth, padheight, 1, pins/2, maskclear, polyclear)
    soelt = soelt + rowofpads([rowpos,skew], pitch, "up", padwidth, padheight, 1+pins/2, pins/2, maskclear, polyclear)
    silkboxheight = max(pitch*(pins-2)/2+padheight+2*silkoffset,silkboxheight)
    
    silky = silkboxheight/2
    len = 0
    print "EP" + str(ep)
    print "EPHEIGHT" + str(rect)
    if rect:
        len = (rect - ep) / 2
    if ep:
        soelt = soelt + pad(0,len,0,-len,ep,polyclear,maskclear,str(pins+1),"rect")
                                      
    # Inside box with notched corner as submitted in patch by PRB
    if(silkstyle == "inside"):
        silkx = width/2 - silkoffset
        soelt = soelt + insidebox(silkx,silky,-silkx,-silky,silkwidth)
    else:
        silkx = width/2 + silkoffset + padwidth
        soelt = soelt + box(silkx,silky,-silkx,-silky,silkwidth)
        soelt = soelt + silk(0,-silky+2000,-2000,-silky,silkwidth)
        soelt = soelt + silk(0,-silky+2000,2000,-silky,silkwidth)
    return soelt+")\n"

def twopad(attrlist):
    width = findattr(attrlist, "width")
    padwidth = findattr(attrlist, "padwidth")
    padheight = findattr(attrlist, "padheight")
    polyclear = findattr(attrlist, "polyclear")
    maskclear = findattr(attrlist, "maskclear")
    silkwidth = findattr(attrlist, "silkwidth")
    silkboxwidth = findattr(attrlist, "silkboxwidth")
    silkboxheight = findattr(attrlist, "silkboxheight")
    silkoffset = findattr(attrlist, "silkoffset")
    refdesx = findattr(attrlist, "refdesx")
    refdesy = findattr(attrlist, "refdesy")
    twopadelt = "Element[0x00000000 \"\" \"\" \"\" 1000 1000 %d %d 0 100 0x00000000]\n(\n" % (refdesx, -4000 - refdesy)
    twopadelt = twopadelt + rowofpads([0,0], width+padwidth, "right", padwidth, padheight, 1, 2, maskclear, polyclear)
    silkx = max((width+2*padwidth)/2 + silkoffset,silkboxwidth/2)
    silky = max(padheight/2 + silkoffset, silkboxheight/2)
    twopadelt = twopadelt + box(silkx,silky,-silkx,-silky,silkwidth)
    return twopadelt+")\n"

# SOT223, DDPAK, TO-263, etc
def tabbed(attrlist):
    pins = findattr(attrlist, "pins")
    tabwidth = findattr(attrlist, "tabwidth")
    tabheight = findattr(attrlist, "tabheight")
    height = findattr(attrlist, "height")
    padwidth = findattr(attrlist, "padwidth")
    padheight = findattr(attrlist, "padheight")
    polyclear = findattr(attrlist, "polyclear")
    maskclear = findattr(attrlist, "maskclear")
    silkwidth = findattr(attrlist, "silkwidth")
    silkoffset = findattr(attrlist, "silkoffset")
    pitch = findattr(attrlist, "pitch")
    refdesx = findattr(attrlist, "refdesx")
    refdesy = findattr(attrlist, "refdesy")
    totalheight = height+tabheight+padheight
    totalwidth = max(tabwidth, (pins-1)*pitch+padwidth)
    silkx = totalwidth/2 + silkoffset
    silky = totalheight/2 + silkoffset
    taby = -(totalheight-tabheight)/2
    padsy = -(padheight - totalheight)/2
    tabbedelt = "Element[0x00000000 \"\" \"\" \"\" 1000 1000 %d %d 0 100 0x00000000]\n(\n" % (refdesx, -4000 - refdesy)
    tabbedelt = tabbedelt + rowofpads([0,padsy], pitch, "right", padwidth, padheight, 1, pins, maskclear, polyclear)
    tabbedelt = tabbedelt + padctr(0,taby,tabheight,tabwidth,polyclear,maskclear,str(pins+1))
    tabbedelt = tabbedelt + box(silkx,silky,-silkx,-silky,silkwidth)
    return tabbedelt+")\n"
#  Pin[17500 -24000 6400 2000 6400 3500 "" "1" 0x00000001]
# x,y,paddia,polyclear,maskclear,drill,name,name,flags

def pin(x,y,dia,drill,name,polyclear,maskclear):
    if name == "1":
        pinflags = "0x00000100"
    else:
        pinflags = "0x00000000"
    return "\tPin[ %d %d %d %d %d %d \"%s\" \"%s\" %s]\n" % (x,y,dia,polyclear*2,maskclear+dia,drill,name,name,pinflags)

def dip(attrlist):
    pins = findattr(attrlist, "pins")
    drill = findattr(attrlist, "drill")
    paddia = findattr(attrlist, "paddia")
    width = findattr(attrlist, "width")
    polyclear = findattr(attrlist, "polyclear")
    maskclear = findattr(attrlist, "maskclear")
    silkwidth = findattr(attrlist, "silkwidth")
    pitch = findattr(attrlist, "pitch")
    refdesx = findattr(attrlist, "refdesx")
    refdesy = findattr(attrlist, "refdesy")
    y = -(pins/2-1)*pitch/2
    x = width/2
    dipelt = "Element[0x00000000 \"\" \"\" \"\" 1000 1000 %d %d 0 100 0x00000000]\n(\n" % (refdesx, -4000 - refdesy)
    for pinnum in range (1,1+pins/2):
        dipelt = dipelt + pin(-x,y,paddia,drill,str(pinnum),polyclear,maskclear)
        y = y + pitch
    y = y - pitch
    for pinnum in range (1+pins/2, pins+1):
        dipelt = dipelt + pin(x,y,paddia,drill,str(pinnum),polyclear,maskclear)
        y = y - pitch
    silky = pins*pitch/4
    silkx = (width+pitch)/2
    dipelt = dipelt + box(silkx,silky,-silkx,-silky,silkwidth)
    dipelt = dipelt + box(-silkx,-silky,-silkx+pitch,-silky+pitch,silkwidth)
    return dipelt+")\n"

def dih(attrlist):
    pins = findattr(attrlist, "pins")
    drill = findattr(attrlist, "drill")
    paddia = findattr(attrlist, "paddia")
    width = findattr(attrlist, "width")
    polyclear = findattr(attrlist, "polyclear")
    maskclear = findattr(attrlist, "maskclear")
    silkwidth = findattr(attrlist, "silkwidth")
    pitch = findattr(attrlist, "pitch")
    refdesx = findattr(attrlist, "refdesx")
    refdesy = findattr(attrlist, "refdesy")
    y = -(pins/2-1)*pitch/2
    x = width/2
    dihelt = "Element[0x00000000 \"\" \"\" \"\" 1000 1000 %d %d 0 100 0x00000000]\n(\n" % (refdesx, -4000 - refdesy)
    for pinnum in range (1,1+pins,2):
        dipelt = dipelt + pin(-x,y,paddia,drill,str(pinnum),polyclear,maskclear)
        y = y + pitch
    y = -(pins/2-1)*pitch/2
    for pinnum in range (2,1+pins,2):
        dipelt = dipelt + pin(x,y,paddia,drill,str(pinnum),polyclear,maskclear)
        y = y + pitch
    silky = pins*pitch/4
    silkx = (width+pitch)/2
    dipelt = dipelt + box(silkx,silky,-silkx,-silky,silkwidth)
    dipelt = dipelt + box(-silkx,-silky,-silkx+pitch,-silky+pitch,silkwidth)
    return dipelt+")\n"

def sip(attrlist):
    pins = findattr(attrlist, "pins")
    drill = findattr(attrlist, "drill")
    paddia = findattr(attrlist, "paddia")
    polyclear = findattr(attrlist, "polyclear")
    maskclear = findattr(attrlist, "maskclear")
    silkwidth = findattr(attrlist, "silkwidth")
    pitch = findattr(attrlist, "pitch")
    refdesx = findattr(attrlist, "refdesx")
    refdesy = findattr(attrlist, "refdesy")
    y = -(pins-1)*pitch/2
    sipelt = "Element[0x00000000 \"\" \"\" \"\" 1000 1000 %d %d 0 100 0x00000000]\n(\n" % (refdesx, -4000 - refdesy)
    for pinnum in range (1,1+pins):
        sipelt = sipelt + pin(0,y,paddia,drill,str(pinnum),polyclear,maskclear)
        y = y + pitch
    silky = pins*pitch/2
    silkx = pitch/2
    sipelt = sipelt + box(silkx,silky,-silkx,-silky,silkwidth)
    sipelt = sipelt + box(-silkx,-silky,-silkx+pitch,-silky+pitch,silkwidth)
    return sipelt+")\n"

def genpart(attributes):
    parttype = findattr(attributes, "type")
    if parttype=='bga':
        return bga(attributes)
    elif parttype=='qfp':
        return qfp(attributes)
    elif parttype=='so':
        return so(attributes)
    elif parttype=='twopad':
        return twopad(attributes)
    elif parttype=='tabbed':
        return tabbed(attributes)
    elif parttype == 'dip':
        return dip(attributes)
    elif parttype == 'dih':
        return dih(attributes)
    elif parttype == 'sip':
        return sip(attributes)
    else:
        print "Unknown type "+parttype
    print ""
    return ""                                                                                                           

def inquotes(origstring):
    quotepos = re.search("\".*\"", origstring)
    if quotepos:
        return origstring[quotepos.start()+1:quotepos.end()-1]
    return ""

def str2pcbunits(dist):
    number = re.search('[-?0-9\.]+', dist)
    if number:
        val = float(number.group())
    else:
        return 0
    if dist[number.end():].find("mm")!=-1:
        return int(0.5 + 100000 * val/25.4)
    elif dist[number.end():].find("mil")!=-1:
        return int(0.5 + val * 100)
    elif dist[number.end():].find("in")!=-1:
        return int(0.5 + val * 100000)
    return int(val+0.5)	

import sys
#import string
import re

# open input file
if sys.argv[1:]:
    try:
        in_file = open(sys.argv[1], 'r')
    except IOError, msg:
        print 'Can\'t open "%s":' % sys.argv[1], msg
        sys.exit(1)
else:
    in_file = sys.stdin
    print "No file given so using stdin"

# list of attributes to be defined in file
attributes = defattr()
# main processing loop enter processing elements
linenum = 1
while 1:
    validline = 0
    line = in_file.readline()
    linenum = linenum + 1
    if not line:
        break
    # strip comments
    if line.find("#") == 0:
        continue
    if line.find("#")!=-1:
        line = line[:line.find("#")-1]
    # strip whitespace
    line = line.strip()
    if line == "":
        continue
    if line.find("clearall")!=-1:
        attributes = defattr()
        continue
    for attribute in attributes:
        if line.find(attribute[0])!=-1:
            attribute[1]=inquotes(line)
            if not attribute[1]:
                attribute[1]=str2pcbunits(line)
            validline = 1
            break
    if attribute[0] == "part":
        filename = findattr(attributes,"elementdir")+"/"+findattr(attributes,"part")
        print "generated %s" % filename
        output_file = open(filename, "w")
        output_file.write(genpart(attributes))
        output_file.close()
        validline = 1
    if not validline:
        print "Ignoring garbage at line %d :%s" % (linenum, line)
			
print "%d lines read" % linenum
sys.exit(0)
