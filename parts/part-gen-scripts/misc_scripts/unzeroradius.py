
import getopt, sys, os, os.path, re, xml.dom.minidom, xml.dom
    
def usage():
        print """
usage:
    unzeroradius.py -d [svg folder]
    if a file has <circle r='0' replace with r='.00000000001'
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
            circleNodes = svg.getElementsByTagName("circle")

            count = 0
            for circle in circleNodes:
                r = circle.getAttribute("r")
                if r == "0":
                    circle.setAttribute("r", "0.00000000001")
                    count += 1
                    
            if count == 0:
                #print ".",
                continue
                
            print "got zero", svgFilename
            outfile = open(svgFilename, 'wb')
            s = dom.toxml("UTF-8")
            outfile.write(s)
            outfile.flush()
            outfile.close()                        
            
if __name__ == "__main__":
        main()



