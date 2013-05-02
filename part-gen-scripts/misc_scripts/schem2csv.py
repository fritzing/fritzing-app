# usage:
#    schem2csv.py -d <directory> 
#    
#    <directory> is a folder containing schematic .svg files.  
#    save a csv file to <csv file path> 

import getopt, sys, os, re, csv, xml.dom.minidom, xml.dom
    
def usage():
    print """
usage:
    schem2csv.py -d <directory> -c <csv file path>
    
    <directory> is a folder containing .svg files.  
    save a csv file to <csv file path> 
    """
        
       
def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hd:c:", ["help", "directory", "csv"])
    except getopt.GetoptError, err:
        # print help information and exit:
        print str(err) # will print something like "option -a not recognized"
        usage()
        sys.exit(2)
    outputDir = None
    csvPath = None
    
    for o, a in opts:
        #print o
        #print a
        if o in ("-d", "--directory"):
            outputDir = a
        elif o in ("-c", "--csv"):
            csvPath = a
        elif o in ("-h", "--help"):
            usage()
            sys.exit(2)
        else:
            assert False, "unhandled option"
    
    if not outputDir:
        usage()
        sys.exit(2)
        
    if not csvPath:
        usage()
        sys.exit(2)
        
    writer = None
    file = None
    try:
        file = open(csvPath, 'wb')
        writer = csv.writer(file, delimiter=',')
        writer.writerow(["current svg","location","disp","use svg"] )
        writer.writerow(["","","rectangular\nillustrated\nsparkfun","path of sparkfun svg to use"] )
        writer.writerow(["","","",""] )
    except:
        print "unable to save to", csvPath
        sys.exit(2)
            
    names = []
    for filename in os.listdir(outputDir): 
        if (filename.endswith(".svg")):  
            svgFilename = os.path.join(outputDir, filename)
            try:
                dom = xml.dom.minidom.parse(svgFilename)
            except xml.parsers.expat.ExpatError, err:
                print str(err), svgFilename
                continue
            
            svg = dom.documentElement
            viewBox = svg.getAttribute("viewBox").split(" ")
            area = sys.float_info.max
            if len(viewBox) != 4:
                print "no viewBox in", svgFilename
            else:   
                try:
                    viewBoxWidth = float(viewBox[2]) - float(viewBox[0])
                    viewBoxHeight = float(viewBox[3]) - float(viewBox[1])
                    area = viewBoxWidth * viewBoxHeight * 0.75
                    if (area <= 0):
                        print "bad viewBox (1) in", svgFilename
                except:
                    print "bad viewBox (2) in", svgFilename
               
            disp = "illustrated"
            rects = svg.getElementsByTagName("rect")
            for rect in rects:
                try:
                    width = float(rect.getAttribute("width"))
                    height = float(rect.getAttribute("height"))
                    if width * height > area:
                        disp = "rectangular"
                        break
                except:
                    print "bad rect in", svgFilename
                
            location = "core"
            if "contrib" in svgFilename:
                location = "contrib"
            
            writer.writerow([filename, location, disp, ""])
    
    if file:
        file.close()
        
            
def getText(nodelist):
    rc = []
    for node in nodelist:
        if node.nodeType == node.TEXT_NODE:
            rc.append(node.data)
    return ''.join(rc) 
    
if __name__ == "__main__":
    main()

