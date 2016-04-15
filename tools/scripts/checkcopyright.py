# usage:
#    checkcopyright.py -d <directory> 
#    
#    <directory> is a folder, with subfolders, containing .cpp and .h files.  
#    update Copyright (c) 2007-xxxx Fritzing

import getopt, sys, os, re, time
    
def usage():
    print """
usage:
    checkcopyright.py -d <directory> 
    
    <directory> is a folder, with subfolders, containing .cpp and .h files.  
    update Copyright (c) 2007-xxxx Fritzing
    
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
                try:
                    path = os.path.join(root, filename)
                    infile = open(path, "r")
                    ttuple = time.localtime(os.path.getmtime(path))
                    year = ttuple[0]
                    txt = infile.read();
                    infile.close();
                    pattern = 'Copyright \(c\) (\d\d\d\d)-(\d\d\d\d) Fritzing'
                    m = re.search(pattern, txt)
                    if (m):
                        if (year > int(m.group(2))):
                            newtxt = re.sub(pattern, 'Copyright (c) ' + m.group(1) + '-' + str(year) + ' Fritzing', txt)
                            print "{0} {1} {2}".format(path, year, m.group(2))     
                            #print "{0}".format(newtxt[:150])
                            infile = open(path, "w")
                            infile.write(newtxt)
                            infile.close();

                except IOError as (errno, strerror):
                    print "exception {0} {1}".format(path, strerror) 
  
    
if __name__ == "__main__":
    main()

