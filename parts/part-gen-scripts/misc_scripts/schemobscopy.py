
import getopt, sys, os, re, shutil
    
def usage():
    print """
usage:
    schemobscopy.py -f <fritzing folder> -h <hand drawn schematic folder> -g <generated schematics folder>
    
    in the <parts folder>/svg/core/schematic folder match files from the -h and -g folders
    copy the old files into obsolete with a prefix of "0.3.schem" and copy the new files into place
    """
        
       
def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "h:g:f:", ["hand", "generated", "fritzing"])
    except getopt.GetoptError, err:
        # print help information and exit:
        print str(err) # will print something like "option -a not recognized"
        usage()
        sys.exit(2)
    fritzingDir = None
    handDir = None
    generatedDir = None
    
    for o, a in opts:
        #print o
        #print a
        if o in ("-h", "--hand"):
            handDir = a
        elif o in ("-g", "--generated"):
            generatedDir = a
        elif o in ("-f", "--fritzing"):
            fritzingDir = a
        else:
           print "unhandled option", o
           usage()
           return
            
    
    if not fritzingDir:
        usage()
        return
        
    if not generatedDir:
        usage()
        return
        
    if not handDir:
        usage()
        return
        
    for filename in os.listdir(handDir):
        copyOne(filename, handDir, fritzingDir)
            
    for filename in os.listdir(generatedDir):
        copyOne(filename, generatedDir, fritzingDir)
 
def copyOne(filename, fromDir, fritzingDir):
    schDir = os.path.join(fritzingDir, "parts", "svg", "core", "schematic")
    obsDir = os.path.join(fritzingDir, "parts", "svg", "obsolete", "schematic")
    try:
        shutil.copy(os.path.join(schDir, filename), os.path.join(obsDir, "0.3.schem." + filename))
        shutil.copy(os.path.join(fromDir, filename), os.path.join(schDir, filename))
    except:
        print "unable to copy", filename
    
if __name__ == "__main__":
    main()

