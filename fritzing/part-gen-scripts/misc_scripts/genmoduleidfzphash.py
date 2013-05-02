# usage:
#    genmoduleidfzphash.py -d [directory of fzp files] -o [output file name]
#   loads the each fzp file, extracts the moduleID, and saves the id-filename pair to the output file

import getopt, sys, os, os.path, re, xml.dom.minidom, xml.dom
    
def usage():
        print """
usage:
    genmoduleidfzphash.py -d [directory of fzp files] -o [output file name]
    loads the each fzp file, extracts the moduleID, and saves the id-filename pair to the output file
"""
    
        
           
def main():
        try:
                opts, args = getopt.getopt(sys.argv[1:], "hd:o:", ["help", "directory", "output"])
        except getopt.GetoptError, err:
                # print help information and exit:
                print str(err) # will print something like "option -a not recognized"
                usage()
                sys.exit(2)
        inputDir = None
        output = None
            
        for o, a in opts:
                #print o
                #print a
                if o in ("-d", "--directory"):
                        inputDir = a
                elif o in ("-o", "--output"):
                        output = a
                elif o in ("-h", "--help"):
                        usage()
                        sys.exit(2)
                else:
                        assert False, "unhandled option"
            
        if(not(inputDir)):
                usage()
                sys.exit(2)

        if(not(output)):
                usage()
                sys.exit(2)
        
        outfile = open(output, 'wb')
        for fname in os.listdir(inputDir):
            if not fname.endswith(".fzp"):
                continue
                
            dom = xml.dom.minidom.parse(os.path.join(inputDir, fname))
            root = dom.documentElement
            id = root.getAttribute("moduleId")
            if (id):
                outfile.write(id)
                outfile.write(",")
                outfile.write(fname.replace(".fzp", ""))
                outfile.write("\n")
                
        outfile.flush()
        outfile.close()                        
        

if __name__ == "__main__":
        main()



