# usage:
#	    copper1svg.py -d [svg folder]
#       adds a <g id="copper1"> if there isn't one found already.


import getopt, sys, os, os.path, re, xml.dom.minidom, xml.dom
    
def usage():
        print """
usage:
    findsvgunits.py -d [svg folder]
    looks for <svg> width and height attributes with no units or px units
"""
           
def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hd:", ["help", "directory"])
    except getopt.GetoptError, err:
        # print help information and exit:
        print str(err) # will print something like "option -a not recognized"
        usage()
        sys.exit(2)
        
    dir = None
            
    for o, a in opts:
        #print o
        #print a
        if o in ("-d", "--directory"):
            dir = a
        elif o in ("-h", "--help"):
            usage()
            sys.exit(2)
        else:
            assert False, "unhandled option"
            
    if(not(dir)):
        usage()
        sys.exit(2)            
            
    for root, dirs, files in os.walk(dir, topdown=False):
        for filename in files:
            if not filename.endswith(".svg"): 
                continue
            
            svgFilename = os.path.join(root, filename)
            printFilename = svgFilename[len(dir):]
            illustratorString = "illustrator"
            noIllustratorString = "           "
            
            try:
                dom = xml.dom.minidom.parse(svgFilename)
            except xml.parsers.expat.ExpatError, err:
                print str(err), svgFilename
                continue
                
            svg = dom.documentElement
            w = svg.getAttribute("width")
            h = svg.getAttribute("height")
            hok = None
            wok = None
            for unit in ["in", "cm", "mm"]:
                if w.endswith(unit):
                    wok = 1
                if h.endswith(unit):
                    hok = 1
        
            if (hok and wok):
                continue
                
            descr = noIllustratorString
            s = dom.toxml("UTF-8")
            if "Generator: Adobe Illustrator" in s:
                descr = illustratorString
            
            if not (w.endswith("px") or h.endswith("px")):
                print "no units {0} {1}".format(descr, printFilename)  
                continue
                
            print "px units {0} {1}".format(descr, printFilename)  
                                                    
            
if __name__ == "__main__":
        main()



