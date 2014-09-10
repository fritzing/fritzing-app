# usage:
#    grep.py -d <directory> -f <text>
#    
#    <directory> is a folder, with subfolders, containing .svg files.  In each svg file in the directory or its children
#    look for <text>

import getopt, sys, os, re
    
def usage():
    print """
usage:
    grep.py -d [directory] -f [text] -n
    
    directory is a folder containing .svg files.  
    In each svg file in the directory or its subfolders,
    look for [text]
    """
    
  	
	   
def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hnd:f:", ["help", "not", "directory", "find"])
    except getopt.GetoptError, err:
        # print help information and exit:
        print str(err) # will print something like "option -a not recognized"
        usage()
        sys.exit(2)
    outputDir = None
    findtext = ""
    gotnot = 0
    
    for o, a in opts:
        #print o
        #print a
        if o in ("-d", "--directory"):
            outputDir = a
        elif o in ("-h", "--help"):
            usage()
            sys.exit(2)
        elif o in ("-f", "--find"):
            findtext = a;
        elif o in ("-n", "--not"):
            gotnot = 1;

        else:
            assert False, "unhandled option"
    
    if(not(outputDir)):
        usage()
        sys.exit(2)
     

    print "finding text " + findtext   
    
    for root, dirs, files in os.walk(outputDir, topdown=False):
        for filename in files:
            if (filename.endswith(".svg")):  
                infile = open(os.path.join(root, filename), "r")
                svg = infile.read();
                infile.close();

                rslt = svg.find(findtext)

                if gotnot and (rslt < 0):
                    print "{0}:{1}".format(os.path.join(root, filename), rslt)
                if (not gotnot) and (rslt > -1):
                    print "{0}:{1}".format(os.path.join(root, filename), rslt)

               
  
    
if __name__ == "__main__":
    main()

