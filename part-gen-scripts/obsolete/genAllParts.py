# usage:
#    genAllParts.py -d <directory> -a <all parts bin filename> -o <add obsolete: t or f>
#    
#    <directory> is a folder, with subfolders, containing fzp files.  In each fzp file in the directory or its children
#    add a ref to that part to the all parts bin.  if add obsolete is true, then add obsolete files

import getopt, sys, os, re
    
def usage():
    print """
usage:
    genAllParts.py -d [directory]+ -a [all parts bin filename] -o [add obsolete: t or f]
    
    each directory is a folder containing fzp files.  
    In each fzp file in the directory or its subfolders,
    add a ref to that fzp file to the all parts bin.  Skip obsolete if add obsolete is not true
    """
    
  	
	   
def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hd:a:o:", ["help", "directory", "allparts", "obsolete"])
    except getopt.GetoptError, err:
        # print help information and exit:
        print str(err) # will print something like "option -a not recognized"
        usage()
        sys.exit(2)
    inputDirs = []
    allPartsBinFilename = ""
    allowObsolete = 0
    
    for o, a in opts:
        #print o
        #print a
        if o in ("-d", "--directory"):
            inputDirs.append(a)
        elif o in ("-h", "--help"):
            usage()
            sys.exit(2)
        elif o in ("-a", "--allparts"):
            allPartsBinFilename = a;
        elif o in ("-o", "--obsolete"):
            allowObsolete = (a == "t" or a == "T" or a == "true" or a == "TRUE")

        else:
            assert False, "unhandled option"
    
    if(len(inputDirs) <= 0):
        usage()
        sys.exit(2)
    if(not(allPartsBinFilename)):
        usage()
        sys.exit(2)
     
    outfile = open(allPartsBinFilename, "w")
    outfile.write("""<?xml version="1.0" encoding="UTF-8"?>
<module fritzingVersion="0.4.3b.0.0">
    <title>All Parts</title>
    <instances>
""")

    skip = ['NoteModuleID', 'GroupModuleID']
    
    for inputDir in inputDirs:
        for root, dirs, files in os.walk(inputDir, topdown=False):
            for filename in files:
                if (filename.endswith(".fzp")):  
                    allow = 1
                    if (not allowObsolete) and (root.find("obsolete") >= 0):
                        allow = 0
                    if allow:
                        infile = open(os.path.join(root, filename), "r")
                        fzp = infile.read();
                        infile.close();
                        match = re.search('moduleId\s*=\s*\"(.+)\"', fzp)
                        if (match != None):
                            g1 = match.group(1)
                            if g1 in skip:
                                print " skipping {0}".format(g1)
                            else:
                               print "{0}".format(os.path.join(root, filename))
                               outfile.write('      <instance moduleIdRef="' + g1 + '">')
                               outfile.write("""
        <views>
          <iconView layer="icon">
            <geometry z="-1" x="-1" y="-1"></geometry>
          </iconView>
        </views>
      </instance>
""")



               
    outfile.write("""  
    </instances>
</module>
    """)
    outfile.close


if __name__ == "__main__":
    main()

