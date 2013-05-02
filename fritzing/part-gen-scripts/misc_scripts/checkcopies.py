

import getopt, sys, os, re, time
    
def usage():
    print """
usage:
    checkcopies.py -d <directory> 
    
    <directory> is a folder containing svg files.  
    looks for files that have the same content but different names
    
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
    gotnot = 0
    
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
    
    if(not(outputDir)):
        print "missing -d {directory} parameter"
        usage()
        return
     
    fs = []
    for f in os.listdir(outputDir):
        if f.endswith(".svg"):
            fs.append(os.path.splitext(f)[0])
            #print "appending", f, os.path.splitext(f)[0]
    fs.sort(key=str.lower)                                               # remove the .svg so the sort works better
    dirs = []
    for f in fs:
        dirs.append(f + ".svg")
        
    dirlen = len(dirs)
    matches = []
    for i in range(dirlen):
        if i in matches:
            continue
            
        f1 = dirs[i]
        path = os.path.join(outputDir, f1)
        if not os.path.isfile(path):
            continue
            
        if not f1.endswith(".svg"): 
            continue
            
        # print "testing", f1
            
        txt1 = None
        try:
            infile = open(path, "r")
            txt1 = infile.read()
            infile.close()
        except:
            print "failure", f1
            
        if txt1 == None:
            continue
             
        for j in range(i + 1, dirlen):
            f2 = dirs[j]
            path = os.path.join(outputDir,f2)
            if not os.path.isfile(path):
                continue
                
            if not f2.endswith(".svg"): 
                continue
                
            if f1 == f2:
                continue
                
            txt2 = None
            try:
                infile = open(path, "r")
                txt2 = infile.read()
                infile.close()
            except:
                print "failure", f2
                continue
                
            if txt2 == None:
                continue
                
            if txt1 == txt2:
                matches.append(j)
                print "<map package='{0}' to='{1}' />".format(f1, f2) 
    
if __name__ == "__main__":
    main()

