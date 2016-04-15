# usage:
#    svnremove.py -d [directory]
#    
#    directory is a folder containing .svn directories.
#    recursively delete all .svn folders

import getopt, sys, os, re
import shutil, stat

def usage():
    print """
usage:
    svnremove.py -d [directory]
    
    directory is a folder containing .svn directories.  
    recursively delete all .svn folders
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
        for dirname in dirs:
            if dirname == ".svn":  
                removeall(os.path.join(root, dirname))
                print "removing", os.path.join(root, dirname)

               
def removeall(top):
    for root, dirs, files in os.walk(top, topdown=False):
        for name in files:
            fullname = os.path.join(root, name)
            fileAtt = os.stat(fullname)[0]
            if (not fileAtt & stat.S_IWRITE):
                # File is read-only, so make it writeable
                os.chmod(fullname, stat.S_IWRITE)
            os.remove(fullname)
        for name in dirs:
            os.rmdir(os.path.join(root, name))	
    os.rmdir(top)
    
if __name__ == "__main__":
    main()

