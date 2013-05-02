# usage:
#    replace.py -d <directory> -f <find> -r <replace> -s <suffix>
#    
#    <directory> is a folder, with subfolders, containing <suffix> files.  In each <suffix> file in the directory or its children
#    replace <text> with <replace>

import getopt, sys, os, re
    
def usage():
    print """
usage:
    replace.py -d [directory] -f [text] -r [replace] -s [suffix]
    
    directory is a folder containing [suffix] files.  
    In each [suffix] file in the directory or its subfolders,
    replace [text] with [replace]
    """
    
  	
	   
def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hd:f:r:s:", ["help", "directory", "find", "replace", "suffix"])
    except getopt.GetoptError, err:
        # print help information and exit:
        print str(err) # will print something like "option -a not recognized"
        usage()
        sys.exit(2)
    outputDir = None
    findtext = ""
    replacetext = ""
    suffix = ""
    
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
        elif o in ("-r", "--replace"):
            replacetext = a;
        elif o in ("-s", "--suffix"):
            suffix = a;
        elif o in ("-n", "--not"):
            gotnot = 1;

        else:
            assert False, "unhandled option"
    
    if(not(outputDir)):
        usage()
        sys.exit(2)
     

    print "replace text " + findtext + " with " + replacetext + " in " + suffix + " files"
    
    for root, dirs, files in os.walk(outputDir, topdown=False):
        for filename in files:
            if (filename.endswith(suffix)):  
                infile = open(os.path.join(root, filename), "r")
                svg = infile.read();
                infile.close();

                rslt = svg.find(findtext)
                if (rslt >= 0):
                    svg = svg.replace(findtext, replacetext)
                    outfile = open(os.path.join(root, filename), "w")
                    outfile.write(svg);
                    outfile.close()
                    print "{0}".format(os.path.join(root, filename)) 

               
  
    
if __name__ == "__main__":
    main()

