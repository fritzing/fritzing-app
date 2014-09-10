
import getopt, sys, os, re
    
def usage():
    print """
usage:
    schemobs.py -f <fritzing folder> -h <hand drawn schematic folder> -g <generated schematics folder>
    
    in the <parts folder>/svg/core/schematic folder match files from the -h and -g folders
    print the list of files unaccounted for in -f and -g
    ignore the ones that start with sparkfun-
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
        
    toMatch = []
    schemDir = os.path.join(fritzingDir, "parts", "svg", "core", "schematic")
    for filename in os.listdir(schemDir):
        if filename.startswith("sparkfun-"):
            continue
        
        toMatch.append(filename)
        
        
    for filename in os.listdir(handDir):
        if not filename in toMatch:
            print "unable to find {0} from hand".format(filename)
        else:
            toMatch.remove(filename)
            
    for filename in os.listdir(generatedDir):
        if not filename in toMatch:
            print "unable to find {0} from generated".format(filename)
        else:
            toMatch.remove(filename)
    
    print ""
    pdbDir = os.path.join(fritzingDir, "pdb", "core")
    fzps = []
    for filename in os.listdir(pdbDir):
        infile = open(os.path.join(pdbDir, filename), "r")
        txt = infile.read()
        infile.close()
        for matchName in toMatch:
            if matchName in txt:
                print "no match for {0} in {1}".format(matchName, filename)
                fzps.append(filename)
                toMatch.remove(matchName)
                break
    
    print ""
    for filename in fzps:
        print filename
    
if __name__ == "__main__":
    main()

