# usage:
#	fzzrename.py -f [fzz original path] -t [fzz new path] 
#	renames fzz file as well as internal fz file.  if the -t parameter is not a directory, the fzz is renamed but not copied

# lots of borrowing from http://code.activestate.com/recipes/252508-file-unzip/

import getopt, sys, os, os.path, re, zipfile, shutil
    
def usage():
    print """
usage:
    fzzrename.py -f [fzz original path] -t [fzz new path] 
    renames fzz file as well as internal fz file.  if the -t parameter is not a directory, the fzz is renamed but not copied
    """
    
           
def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hf:t:", ["help", "from", "to"])
    except getopt.GetoptError, err:
        # print help information and exit:
        print str(err) # will print something like "option -a not recognized"
        usage()
        sys.exit(2)
    fromName = None
    toName = None
    
    for o, a in opts:
        #print o
        #print a
        if o in ("-f", "--from"):
            fromName = a
        elif o in ("-t", "--to"):
            toName = a
        elif o in ("-h", "--help"):
            usage()
            sys.exit(2)
        else:
            assert False, "unhandled option"
    
    if(not(fromName)):
        usage()
        sys.exit(2)
        
    if not(fromName.endswith(".fzz")):
        print "-f is not .fzz"
        usage()
        sys.exit(2)
        
    if(not(toName)):
        usage()
        sys.exit(2)

    if not(toName.endswith(".fzz")):
        print "-t is not .fzz"
        usage()
        sys.exit(2)

    outputDir = os.path.dirname(toName)
    if len(outputDir) == 0:
        outputDir = os.path.dirname(fromName)
        toName = outputDir + "/" + toName
    else:
        if not outputDir.endswith(':') and not os.path.exists(outputDir):
            os.mkdir(outputDir)
   
    tempDir = outputDir + "/" + "___temp___"
    shutil.rmtree(tempDir, 1)
    os.mkdir(tempDir)
    
    zf = zipfile.ZipFile(fromName)
    zf.extractall(tempDir)
    
    fromBaseName = os.path.basename(fromName)
    toBaseName = os.path.basename(toName)
    
    fromFzName = os.path.splitext(fromBaseName)[0] + ".fz"
    toFzName = os.path.splitext(toBaseName)[0] + ".fz"
    
    print os.path.join(tempDir, fromFzName), os.path.join(tempDir, toFzName)
    os.rename(os.path.join(tempDir, fromFzName), os.path.join(tempDir, toFzName))
    
    # helpful examples in http://www.doughellmann.com/PyMOTW/zipfile/
    try:
        import zlib
        compression = zipfile.ZIP_DEFLATED
    except:
        compression = zipfile.ZIP_STORED

    zf = zipfile.ZipFile(outputDir + "/" + toBaseName, mode='w')
    for fn in os.listdir(tempDir):
        zf.write(os.path.join(tempDir, fn), fn, compression)
    zf.close()

    shutil.rmtree(tempDir, 1)


if __name__ == "__main__":
    main()



