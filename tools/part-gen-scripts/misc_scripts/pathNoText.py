# usage:
#    copperNoSilkscreen.py -d <directory> 
#    
#    <directory> is a folder, with subfolders, containing .fzp files.  In each fzp file in the directory or its children
#    look for "copper" and "silkscreen"

import getopt, sys, os, re
    
def usage():
    print """
usage:
    pathNoText.py -d [directory]
    
    directory is a folder containing .svg files.  
    In each fzp file in the directory or its subfolders,
    look for "<path>" and no "<text>".
    """
    
def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hd:", ["help", "directory"])
    except getopt.GetoptError, err:
        # print help information and exit:
        print str(err) # will print something like "option -a not recognized"
        usage()
        return
    outputDir = None
    
    for o, a in opts:
        #print o
        #print a
        if o in ("-d", "--directory"):
            outputDir = a
        elif o in ("-h", "--help"):
            usage()
            return
        else:
            assert False, "unhandled option"
    
    if not(outputDir) :
        usage()
        return
        
    
    for root, dirs, files in os.walk(outputDir, topdown=False):
        for filename in files:
            if filename.endswith(".svg"):  
                infile = open(os.path.join(root, filename), "r")
                svg = infile.read()
                infile.close()
                textMatch = '<text' in svg
                pathMatch = '<path' in svg
                
                if not(textMatch) and pathMatch:
                    print "{0} {1}".format(os.path.join(root, filename), "path no text")

                
    
if __name__ == "__main__":
    main()

