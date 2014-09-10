# usage:
#    listpropnames.py -d <directory> 
#    
#    <directory> is a folder containing .fzp files.  In each fzp file in the directory containing a <property> element:
#    like <property name="whatever" ...>
#	 list the name, if it's not already listed

import getopt, sys, os, re
    
def usage():
    print """
usage:
    listpropnames.py -d [directory]
    
    directory is a folder containing .fzp files.  
    In each fzp file in the directory containing a <property> element:
    list each different name attribute. 
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
        
    
    names = []
    for filename in os.listdir(outputDir): 
        if (filename.endswith(".fzp")):  
            infile = open(os.path.join(outputDir, filename), "r")
            fzp = infile.read();
            infile.close();
            match = re.search('<property.+name=\"(.+)\".*>.+</property>', fzp)
            if (match != None):
                if not (match.group(1) in names):
                    names.append(match.group(1));
                    print "{0}".format(match.group(1))
    
if __name__ == "__main__":
    main()

