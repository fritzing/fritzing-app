# usage:
#	    copper1fzp.py -d [fzp folder]
#       add or update a <version> element.

import getopt, sys, os, os.path, re, xml.dom.minidom, xml.dom
    
def usage():
        print """
usage:
    version.py -d [fzp folder]
    add or update a <version> element.
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
            if not filename.endswith(".fzp"): 
                continue
                
            fzpFilename = os.path.join(root, filename)
            try:
                dom = xml.dom.minidom.parse(fzpFilename)
            except xml.parsers.expat.ExpatError, err:
                print str(err), fzpFilename
                continue
                
            fzp = dom.documentElement
            versionNodes = fzp.getElementsByTagName("version")
            version = None
            if (versionNodes.length == 0):
                version = dom.createElement("version")
                fzp.insertBefore(version, fzp.firstChild)
            elif (versionNodes.length == 1):
                txt = ""
                version = versionNodes.item(0)                
            else:
                print "multiple version elements in", fzpFilename
                continue
                
            while version.hasChildNodes():
                oldChild = version.firstChild
                version.removeChild(oldChild)
                oldChild.unlink()
                
            vtext = dom.createTextNode("4")
            version.appendChild(vtext)
            
            print "adding version 4 to", fzpFilename
            outfile = open(fzpFilename, 'wb')
            s = dom.toxml("UTF-8")
            outfile.write(s)
            outfile.flush()
            outfile.close()                    
            
if __name__ == "__main__":
        main()



