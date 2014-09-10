# usage:
#    oldfamily.py -d <directory> 
#    
#    <directory> is a folder containing .fzp files.  In each fzp file in the directory containing a <family> text element:
#    like <family>x</family> 
#	 the text is renamed to "obsolete x" and saved back to the file

import getopt, sys, os, re
    
def usage():
    print """
usage:
    oldfamily.py -d [directory]
    
    directory is a folder containing .fzp files.  
    In each fzp file in the directory containing a <family> text element:
    like <family>x</family> 
    the text is renamed to "obsolete x" and saved back to the file.
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
        
    
    for filename in os.listdir(outputDir): 
        if (filename.endswith(".fzp")):  
            infile = open(os.path.join(outputDir, filename), "r")
            fzp = infile.read();
            infile.close();
            match = re.search('(<property.+name=\"family\".*>)(.+)(</property>)', fzp)
            if (match != None):
                if (not match.group(2).startswith("obsolete")):
                    oldfzp = ""
                    if match.group(2).startswith("old "):
                        oldfzp = re.sub(r'(<property.+name=\"family\".*>)old (.+)(</property>)', r'\1obsolete \2\3', fzp);
                    else:
                        oldfzp = re.sub(r'(<property.+name=\"family\".*>)(.+)(</property>)', r'\1obsolete \2\3', fzp);
                    print "{0}:{1}".format(filename, match.group(2))
                    outfile = open(os.path.join(outputDir, filename), "w")
                    outfile.write(oldfzp);
                    outfile.close()

    
if __name__ == "__main__":
    main()

