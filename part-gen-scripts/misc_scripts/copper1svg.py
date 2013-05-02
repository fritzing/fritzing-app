# usage:
#	    copper1svg.py -d [svg folder]
#       adds a <g id="copper1"> if there isn't one found already.


import getopt, sys, os, os.path, re, xml.dom.minidom, xml.dom
    
def usage():
        print """
usage:
    copper1svg.py -d [svg folder]
    adds a <g id="copper1"> if there isn't one found already.
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
            try:
                dom = xml.dom.minidom.parse(svgFilename)
            except xml.parsers.expat.ExpatError, err:
                print str(err), svgFilename
                continue
                
            svg = dom.documentElement
            gNodes = svg.getElementsByTagName("g")
            copper1 = None
            copper0 = None
            for g in gNodes:
                if g.getAttribute("id") == "copper1":
                    copper1 = g
                if g.getAttribute("id") == "copper0":
                    copper0 = g

            if copper1:
                continue
                
            if not copper0:
                print "copper0 not found in", svgFilename
                continue
                
            print "adding copper1 to", svgFilename
            copper1 = dom.createElement("g")
            copper1.setAttribute("id", "copper1")
            copper0.parentNode.insertBefore(copper1, copper0)
            copper1.appendChild(copper0)
            outfile = open(svgFilename, 'wb')
            s = dom.toxml("UTF-8")
            # s = re.sub('\s*\n\s*\n', '', s)   # ghosts of removed (and unlinked) nodes seem to generate newlines, so tighten up the xml
            outfile.write(s)
            outfile.flush()
            outfile.close()                        
            
if __name__ == "__main__":
        main()



