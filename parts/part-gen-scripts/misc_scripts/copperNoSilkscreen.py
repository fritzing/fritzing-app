# usage:
#    copperNoSilkscreen.py -d <directory> 
#    
#    <directory> is a folder, with subfolders, containing .fzp files.  In each fzp file in the directory or its children
#    look for "copper" and "silkscreen"

import getopt, sys, os, re
    
def usage():
    print """
usage:
    copperNoSilkscreen.py -d [directory]
    
    directory is a folder containing .fzp files.  
    In each fzp file in the directory or its subfolders,
    look for "copper" and "silkscreen".
    """
    
    
       
def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hd:", ["help", "directory"])
    except getopt.GetoptError, err:
        # print help information and exit:
        print str(err) # will print something like "option -a not recognized"
        usage()
        sys.exit(2)
    outputDir = None
    
    for o, a in opts:
        #print o
        #print a
        if o in ("-d", "--directory"):
            outputDir = a
        elif o in ("-h", "--help"):
            usage()
            sys.exit(2)
        else:
            assert False, "unhandled option"
    
    if(not(outputDir)):
        usage()
        sys.exit(2)
        
    
    for root, dirs, files in os.walk(outputDir, topdown=False):
        for filename in files:
            if (filename.endswith(".fzp")):  
                infile = open(os.path.join(root, filename), "r")
                fzp = infile.read();
                infile.close();
                copperMatch = re.search('copper', fzp)
                silkscreenMatch = re.search('silkscreen', fzp)
                
                if (copperMatch == None):
                    print "{0} {1}".format(os.path.join(root, filename), "no copper")
                elif (silkscreenMatch == None):
                    print "{0} {1}".format(os.path.join(root, filename), "no silkscreen")

                
  
    
if __name__ == "__main__":
    main()

