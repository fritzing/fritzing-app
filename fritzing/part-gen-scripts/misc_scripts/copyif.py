

import getopt, sys, os, re, time, shutil
    
def usage():
    print """
usage:
    copyif.py -s <source directory> -d <dest directory>
    
    copies files from source into dest, if the file already exists in dest
    
    """
    
    
       
def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hs:d:", ["help", "source","dest"])
    except getopt.GetoptError, err:
        # print help information and exit:
        print str(err) # will print something like "option -a not recognized"
        usage()
        return
    
    source = None
    dest = None
    
    for o, a in opts:
        #print o
        #print a
        if o in ("-d", "--dest"):
            dest = a
        elif o in ("-s", "--source"):
            source = a
        elif o in ("-h", "--help"):
            usage()
            return
        else:
            print "unknown option", o
            return
    
    if not dest:
        print "missing -d (dest) argument"
        usage()
        return
     
    if not source:
        print "missing -s (source) argument"
        usage()
        return
     
    fs = []
    for f in os.listdir(source):
        df = os.path.join(dest, f)
        if os.path.exists(df) and os.path.isfile(df):
            shutil.copy(os.path.join(source,f), df)
    
if __name__ == "__main__":
    main()

