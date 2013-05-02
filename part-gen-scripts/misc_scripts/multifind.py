# usage:
#    multifind.py -d [directory] [-f [text]]+   -s [suffix]
#   
#    directory is a folder containing [suffix] files.  
#    In each [suffix] file in the directory or its subfolders,
#    display all files that match all the texts ('and' search)

import getopt, sys, os, re
    
def usage():
    print """
usage:
    multifind.py -d [directory] [-f [text]]+   -s [suffix]
    
    directory is a folder containing [suffix] files.  
    In each [suffix] file in the directory or its subfolders,
    display all files that match all the texts ('and' search)
    """
    
  	
	   
def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hd:f:s:", ["help", "directory", "find", "suffix"])
    except getopt.GetoptError, err:
        # print help information and exit:
        print str(err) # will print something like "option -a not recognized"
        usage()
        sys.exit(2)
    inputDir = None
    findtexts = []
    suffix = ""
    
    for o, a in opts:
        #print o
        #print a
        if o in ("-d", "--directory"):
            inputDir = a
        elif o in ("-h", "--help"):
            usage()
            sys.exit(2)
        elif o in ("-f", "--find"):
            findtexts.append(a)
        elif o in ("-s", "--suffix"):
            suffix = a;

        else:
            assert False, "unhandled option"
    
    if(not(inputDir)):
        usage()
        sys.exit(2)
	
    if len(findtexts) <= 0:
        usage()
        sys.exit(2)
            
    for root, dirs, files in os.walk(inputDir, topdown=False):
        for filename in files:
            if (filename.endswith(suffix)):  
                infile = open(os.path.join(root, filename), "r")
                content = infile.read();
                infile.close();

                found = 0
                for findtext in findtexts:
                    rslt = content.find(findtext)
                    if (rslt >= 0):
                        found += 1
                    else:
                        break
                      
                if found == len(findtexts):
                    print "found {0}".format(os.path.join(root, filename)) 

               
  
    
if __name__ == "__main__":
    main()

