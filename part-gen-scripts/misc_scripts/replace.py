# usage:
#    replace.py -d <directory> -f <find> -r <replace> -s <suffix>
#    
#    <directory> is a folder, with subfolders, containing <suffix> files.  In each <suffix> file in the directory or its children
#    replace <text> with <replace>

import sys, os, re
import optparse
    
def usage():
    print """
usage:
    replace.py -d [directory] -f [text] -r [replace] -s [suffix]
    
    directory is a folder containing [suffix] files.  
    In each [suffix] file in the directory or its subfolders,
    replace [text] with [replace]
    """
    
    
       
def main():
        
    parser = optparse.OptionParser()
    parser.add_option('-s', '--suffix', dest="suffix" )
    parser.add_option('-d', '--directory', dest="directory")
    parser.add_option('-f', '--find', dest="find" )
    parser.add_option('-r', '--replace', dest="replace")
    (options, args) = parser.parse_args()

    if not options.directory:
        parser.error("directory argument not given")
        usage()
        return
    
    if not options.find:
        parser.error("find argument not given")
        usage()
        return

    if not options.replace:
        parser.error("replace argument not given")
        usage()
        return

    outputDir = options.directory
    findtext = options.find
    replacetext = options.replace
    suffix = options.suffix
    
    
    if not(outputDir):
        usage()
        return
    
    if findtext.startswith('"') or findtext.startswith("'"):
        findtext = findtext[1:-1]
     
    if replacetext.startswith('"') or replacetext.startswith("'"):
        replacetext = replacetext[1:-1]
     
    print "replace text",findtext,"with",replacetext,"in", suffix,"files"
    
    for root, dirs, files in os.walk(outputDir, topdown=False):
        for filename in files:
            if (filename.endswith(suffix)):  
                infile = open(os.path.join(root, filename), "r")
                svg = infile.read();
                infile.close();

                rslt = svg.find(findtext)
                if (rslt >= 0):
                    svg = svg.replace(findtext, replacetext)
                    outfile = open(os.path.join(root, filename), "w")
                    outfile.write(svg);
                    outfile.close()
                    print "{0}".format(os.path.join(root, filename)) 

               
  
    
if __name__ == "__main__":
    main()

