# usage:
#    version.py -d <directory> 
#    
#    <directory> is a folder, with subfolders, containing .fzp files.  In each fzp file in the directory or its children
#    look for a <version> element

import getopt, sys, os, re
    
def usage():
    print """
usage:
    version.py -d [directory]
    
    directory is a folder containing .fzp files.  
    In each fzp file in the directory or its subfolders,
    look for a <version> element.
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
                match = re.search('(<version.*>)(.+)(</version>)', fzp)
                oldfzp = None;
                
                if (match != None):
                    print "{0} {1}".format(os.path.join(root, filename), match.group(2))
                else:
                    print "{0} {1}".format(os.path.join(root, filename), "None")

                expr = r'(?P<aa><version.*>)(?P<bb>.+)(?P<cc></version>)'
                if (match == None):
                    oldfzp = re.sub(r'(<module.*>)', r'\1\n<version>1</version>', fzp);
                elif match.group(2) == "1.0":	
                    oldfzp = re.sub(expr, r'\g<aa>1\g<cc>', fzp);
                elif match.group(2) == "1.1":
                    oldfzp = re.sub(expr, r'\g<aa>2\g<cc>', fzp);
                elif match.group(2) == "2.0":
                    oldfzp = re.sub(expr, r'\g<aa>3\g<cc>', fzp);
                 
                if (oldfzp != None):
                    outfile = open(os.path.join(root, filename), "w")
                    outfile.write(oldfzp);
                    outfile.close()
                
  
    
if __name__ == "__main__":
    main()

