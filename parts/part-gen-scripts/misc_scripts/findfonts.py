# usage:
#    findfonts.py -d <directory> -f [font1] -f [font2] ....
#    
#    <directory> is a folder, with subfolders, containing .svg files.  In each svg file in the directory or its children
#    look for fonts that aren't in the list

import getopt, sys, os, re
    
def usage():
    print """
usage:
    droid.py -d [directory] -f [font1] -f [font2] ...
    
    directory is a folder containing .svg files.  
    In each svg file in the directory or its subfolders,
    look for fonts that aren't in the list
    """
    
  	
	   
def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hd:f:", ["help", "directory", "font"])
    except getopt.GetoptError, err:
        # print help information and exit:
        print str(err) # will print something like "option -a not recognized"
        usage()
        sys.exit(2)
    outputDir = None
    fonts = []
    
    for o, a in opts:
        #print o
        #print a
        if o in ("-d", "--directory"):
            outputDir = a
        elif o in ("-h", "--help"):
            usage()
            sys.exit(2)
        elif o in ("-f", "--font"):
            fonts.append(a);
        else:
            assert False, "unhandled option"
    
    if(not(outputDir)):
        usage()
        sys.exit(2)
        
    
    for root, dirs, files in os.walk(outputDir, topdown=False):
        for filename in files:
            if (filename.endswith(".svg")):  
                infile = open(os.path.join(root, filename), "r")
                svg = infile.read();
                infile.close();

                matches = re.findall('font-family\\s*=\\s*\"(.+)\"', svg)
                listMatches(matches, fonts, root, filename);
                
                matches = re.findall('font-family\\s*:\\s*(.+)[\\";]', svg)
                listMatches(matches, fonts, root, filename);
				
                    
def listMatches(matches, fonts, root, filename):
    for m in matches:
        gotone = 0
        for fontname in fonts:
            if (m.find(fontname) >= 0):
                gotone = 1;
                break;
        if not gotone:
            print "{0}::{1}".format(os.path.join(root, filename), m)
               
  
    
if __name__ == "__main__":
    main()

