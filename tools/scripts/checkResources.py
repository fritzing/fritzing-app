
import getopt, sys, os, re
    
def usage():
    print """
usage:
    checkResources.py -r <resources directory> -q <path to .qrc file> -s <path to sources directory>
    
    directory is a folder containing resource files.  
    In each file in the directory or its subfolders, check whether the file is listed in the .qrc file
    then for each entry in qrc, check that it exists in a file in the sources
    """
    
def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hr:q:s:", ["help", "resources", "qrc", "sources"])
    except getopt.GetoptError, err:
        # print help information and exit:
        print str(err) # will print something like "option -a not recognized"
        usage()
        return

    resourcesDir = None
    sourcesDir = None
    qrcPath = None
    
    for o, a in opts:
        #print o
        #print a
        if o in ("-r", "--resources"):
            resourcesDir = a
        elif o in ("-s", "--sources"):
            sourcesDir = a
        elif o in ("-q", "--qrc"):
            qrcPath = a
        elif o in ("-h", "--help"):
            usage()
            return
        else:
            assert False, "unhandled option"
    
    if not(resourcesDir):
        print "missing resources directory"
        usage()
        return
        
    if not(sourcesDir):
        print "missing sources directory"
        usage()
        return
        
    if not(qrcPath):
        print "missing qrc path"
        usage()
        return
     
    qrc = ""
    try:
        qrcFile = open(qrcPath, "r")
        qrc = qrcFile.read()
        qrcFile.close()
    except:
        pass
        
    if len(qrc) == 0:
        print "unable to read qrc file", qrcPath
        return
    
    for root, dirs, files in os.walk(resourcesDir, topdown=False):
        for filename in files:
            suffix = root.replace(resourcesDir, "resources")
            lookfor = (suffix + "/" + filename).replace("\\", "/");
            if not lookfor in qrc:
                print "resource file not in qrc", os.path.join(root, filename)
                
    print ""
    print ""
    print ""
                
    lines = qrc.split("\n")
    lines2 = []
    for line in lines:
        if not "<file>" in line:
            continue
            
        lix = line.index("<file>")
        rix = line.index("</file", lix)
        name = line[lix + 6: rix]
        lines2.append(name)
    
    print ""
    print ""
    print ""
                
    for root, dirs, files in os.walk(sourcesDir, topdown=False):
        for filename in files:
            text = ""
            try:
                file = open(os.path.join(root, filename))
                text = file.read()
                file.close()
            except:
                print "unable to open", os.path.join(root, filename)
                continue
                
            for line in lines2:
                if line in text:
                    print "found", line, "in", os.path.join(root, filename)
                    lines2.remove(line)
                    break


    for line in lines2:
        print line, "not found in source"
                
    
if __name__ == "__main__":
    main()

