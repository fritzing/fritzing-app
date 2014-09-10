# usage:
#    replacedby.py -d <directory> 
#    
#    <directory> is a folder containing .fzp files.  add a <property name="replaced by">some name</property>

import getopt, sys, os, re
    
def usage():
    print """
usage:
    replacedby.py -d [directory]
    
    directory is a folder containing .fzp files.  
    add a <property name="replaced by">some name</property>
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
        
    
    for filename in os.listdir(outputDir): 
        if (filename.endswith(".fzp")):  
            infile = open(os.path.join(outputDir, filename), "r")
            fzp = infile.read();
            infile.close();
            match = re.search('<version.+replacedby.*>(.+)</version>', fzp)
            if (match == None):
                newprop = ""
                if filename.startswith("resistor"):
                    newprop = 'ResistorModuleID'
                elif filename.startswith("generic-female-header_"):
                    fmatch = re.search('generic-female-header_(\d+)\.', filename)
                    newprop = 'generic_female_pin_header_' + fmatch.group(1) + '_100mil'
                elif filename.startswith("generic-female-header-rounded_"):
                    fmatch = re.search('generic-female-header-rounded_(\d+)\.', filename)
                    newprop = 'generic_rounded_female_pin_header_' + fmatch.group(1) + '_100mil'
                elif filename.startswith("generic-male-header_"):
                    fmatch = re.search('generic-male-header_(\d+)\.', filename)
                    newprop = 'generic_male_pin_header_' + fmatch.group(1) + '_100mil'
                elif filename.startswith("mystery_part"):
                    fmatch = re.search('mystery_part(\d+)\.', filename)
                    newprop = 'mystery_part_' + fmatch.group(1)
                elif filename.startswith("pcb-plain"):
                    newprop = 'RectanglePCBModuleID'
                   
                if (newprop):
                    newfzp = fzp.replace("<version", "<version " + "replacedby=\"" + newprop + "\" ", 1);
                    print "{0}:{1}".format(filename, newprop)
                    outfile = open(os.path.join(outputDir, filename), "w")
                    outfile.write(newfzp);
                    outfile.close()

    
if __name__ == "__main__":
    main()

