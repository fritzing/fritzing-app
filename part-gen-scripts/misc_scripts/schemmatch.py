
import getopt, sys, os, re
    
def usage():
    print """
usage:
    schemmatch.py -f <fritzing parts folder>
    
    looks for all svgs in fritzing parts/whatever and tries to find a matching obsolete/0.3.schem. file  
    ignore the ones that start with sparkfun-
    """
        
       
def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "f:", ["fritzing"])
    except getopt.GetoptError, err:
        # print help information and exit:
        print str(err) # will print something like "option -a not recognized"
        usage()
        return
    
    fritzingDir = None
    
    for o, a in opts:
        #print o
        #print a
        if o in ("-f", "--fritzing"):
            fritzingDir = a
        else:
           print "unhandled option", o
           usage()
           return
            
    if not fritzingDir:
        usage()
        return
        
    for root, dirs, files in os.walk(fritzingDir, topdown=False):
        for name in files:
            if not name.endswith("svg"):
                continue
            if name.startswith("sparkfun-"):
                continue
                
            fullname = os.path.join(root, name)
            pathlist = fullname.split(os.sep)
            if pathlist[-4] != "svg":
                continue
                
            if pathlist[-3] == "obsolete":
                continue
            
            if pathlist[-2] != "schematic":
                continue
            
            pathlist[-3] = "obsolete"
            pathlist[-1] = "0.3.schem." + pathlist[-1]
            
            try:
                with open(os.sep.join(pathlist)):
                    pass
            except IOError:
                print "no 0.3.schem for", fullname
            

if __name__ == "__main__":
    main()

