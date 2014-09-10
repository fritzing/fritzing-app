# usage:
#    footprintgrid.py -o [outputfile] -u ['in' or 'mm' units] -l [left padding (units)] -t [top padding (units)] -r [right padding  (units)] -b [bottom padding  (units)] -x [horizontal pin count] -y [vertical pin count] -g [grid size (units)] -h [hole diameter (units)] -r [ring thickness (units)]
#   
#    writes an svg footprint file to [outputfile] 
#    units are either all in inches or millimeters
#    padding left and top is the distance from the edge of the part to the center of the top left pin
#    padding right and bottom is the distance from the edge of the part to the center of the bottom right pin
#    x and y define the number of pins
#    gridsize is the distance from the center of one pin to the center of another pin
#    drillhole is the diameter of the pin's drillhole
#    ring thickness is the width of the ring (not including the drillhole)
#
#    example: footprintgrid.py -o arduino_mini.svg -u in -l 0.1 -t 0.1 -r0.1 -b 0.1 -x 12 -y 7 -g 0.1 -c 0.02 -d 0.035
 



import getopt, sys, os, re
    
def usage():
    print """
usage:
    footprintgrid.py -o [outputfile] -u ['in' or 'mm' units] -l [left padding (units)] -t [top padding (units)] -r [right padding  (units)] -b [bottom padding  (units)] -x [horizontal pin count] -y [vertical pin count] -g [grid size (units)] -d [drillhole diameter (units)] -c [copper thickness (units)]
    
    writes an svg footprint file to [outputfile] 
    units are either all in inches or millimeters
    padding left and top is the distance from the edge of the part to the center of the top left pin
    padding right and bottom is the distance from the edge of the part to the center of the bottom right pin
    x and y define the number of pins
    g is the distance from the center of one pin to the center of another pin
    drillhole is the diameter of the pin's drillhole
    copper thickness is the width of the copper ring (not including the drillhole)
    """
    
  	
	   
def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "ho:u:l:t:r:b:x:y:g:d:c:", ["help", "output", "units", "left", "top", "right", "bottom", "x", "y", "grid", "drillhole", "copper"])
    except getopt.GetoptError, err:
        # print help information and exit:
        print str(err) # will print something like "option -a not recognized"
        usage()
        sys.exit(2)
    outputFile = None
    units = None
    left = None
    right = None
    top = None
    bottom = None
    x = None
    y = None
    gridSize = None
    drillHole = None
    ringThickness = None
    
    for o, a in opts:
        if o in ("-o", "--output", "--outputfile"):
            outputFile = a
        elif o in ("-h", "--help"):
            usage()
            sys.exit(2)
        elif o in ("-u", "--units"):
            units = a
        elif o in ("-l", "--left"):
            left = float(a)
        elif o in ("-r", "--right"):
            right = float(a)
        elif o in ("-t", "--top"):
            top = float(a)
        elif o in ("-b", "--bottom"):
            bottom = float(a)
        elif o in ("-x"):
            x = int(a)
        elif o in ("-y"):
            y = int(a)
        elif o in ("-g", "--grid", "--gridsize"):
            gridSize = float(a)
        elif o in ("-d", "--drill", "--drillhole"):
            drillHole = float(a)
        elif o in ("-c", "--ring", "--ringthickness", "--thickness", "--copper", "--copperthickness"):
            ringThickness = float(a)
        else:
            print ("params %s %s" % (o, a))
            assert False, "unhandled option " 
    
    for thing in (outputFile, units, left, right, top, bottom, x, y, gridSize, drillHole, ringThickness):
        if(not(thing)):
            usage()
            sys.exit(2)
            
    if units == 'mm':
        left /= 25.4
        right /= 25.4
        top /= 25.4
        bottom /= 25.4
        gridSize /= 25.4
        drillHole /= 25.4
        ringThickness /= 25.4
    
    width = left + right + ((x - 1) * gridSize)
    width000 = width * 1000
    height = top + bottom + ((y - 1) * gridSize)
    height000 = height * 1000
    
    radius = (drillHole + ringThickness) / 2.0
        
    svg = "<?xml version='1.0' encoding='UTF-8' standalone='no'?>\n"
    svg += "<svg xmlns='http://www.w3.org/2000/svg' version='1.2' baseProfile='tiny' x='0in' y='0in' width='%fin' height='%fin' viewBox='0 0 %f %f'>\n" % (width, height, width000, height000)
    svg += "<g id='silkscreen'>\n"
    #across the top:
    svg += "<line x1='4' y1='4' x2='%f' y2='4' stroke='#ffffff' stroke-width='8' stroke-linecap='round' />\n" % (width000 - 8)
    #across the bottom:
    svg += "<line x1='4' y1='%f' x2='%f' y2='%f' stroke='#ffffff' stroke-width='8' stroke-linecap='round' />\n" % (height000 - 8, width000 - 8, height000 - 8)
    #down left with a break in the middle
    svg += "<line x1='4' y1='4' x2='4' y2='%f' stroke='#ffffff' stroke-width='8' stroke-linecap='round' />\n" % ((height000 / 2.0) - (1000 * gridSize / 2.0))
    svg += "<line x1='4' y1='%f' x2='4' y2='%f' stroke='#ffffff' stroke-width='8' stroke-linecap='round' />\n" % ((height000 / 2.0) + (1000 * gridSize / 2.0), height000 - 8)
    #down right
    svg += "<line x1='%f' y1='4' x2='%f' y2='%f' stroke='#ffffff' stroke-width='8' stroke-linecap='round' />\n" % (width000 - 8,  width000 - 8, height000 - 8)
    svg += "</g>\n"
    svg += "<g id='copper0'>\n"
    cy = top
    for iy in range(y):
        cx = left
        for ix in range(x):
            svg += "<circle id='connector%dpin' fill='none' cx='%f' cy='%f' r='%f' stroke='rgb(255, 191, 0)' stroke-width='%f' />\n" % (ix + ((y - iy -1) * x), cx * 1000, cy * 1000, radius * 1000, ringThickness * 1000)
            if (ix == 0 and iy == y - 1):
                svg += "<rect x='%f' y='%f' width='%f' height='%f' fill='none' stroke='rgb(255, 191, 0)' stroke-width='%f'/>\n" % ((cx - radius) * 1000, (cy - radius) * 1000, radius * 1000 * 2, radius * 1000 * 2, ringThickness * 1000)
            cx += gridSize
        
        cy += gridSize
        
    svg += "</g>\n"
    svg += "</svg>"
    infile = open(outputFile, "w")
    infile.write(svg);
    infile.close();
   
    
               
  
    
if __name__ == "__main__":
    main()

