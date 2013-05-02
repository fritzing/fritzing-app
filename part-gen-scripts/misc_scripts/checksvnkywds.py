# usage:
#    checksvnkywds.py -d <directory>
#    
#    <directory> is a folder, with subfolders, containing .cpp and .h files.  
#    see which .cpp or .h files are missing svn keywords

import getopt, sys, os, re
    
def usage():
    print """
usage:
    checksvnkywds.py -d [directory]
    
    directory is a folder, with subfolders, containing .cpp or .h files.  
    see which .cpp or .h files are missing svn keywords
    
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
        else:
            assert False, "unhandled option"
    
    if(not(outputDir)):
        usage()
        sys.exit(2)
         
    for root, dirs, files in os.walk(outputDir, topdown=False):
        for filename in files:
            if filename.endswith(".h") or filename.endswith(".cpp"): 
                svgFilename = os.path.join(root, ".svn", "prop-base", filename + ".svn-base");
                gotIt = 1
                try:
                    infile = open(svgFilename, "r")
                    txt = infile.read();
                    infile.close();
                    if not txt.find("svn:keywords"):
                        gotIt = 0
                    elif not txt.find("Date"):
                        gotIt = 0  
                    elif not txt.find("Author"):
                        gotIt = 0  
                    elif not txt.find("Revision"):
                        gotIt = 0             
                except IOError as (errno, strerror):
				    gotIt = 0
				
                if (not gotIt):
                    print "{0}".format(os.path.join(root, filename))                   

               
  
    
if __name__ == "__main__":
    main()

