# usage:
#	fzpzclean.py -f [fzpz directory] -d [output directory] -o [core | contrib | user]
#	 unzip fzpz files into the output directory and remove all the guids from the filenames and internal names.


#	TODO:
#		check for conflicting names
#		check for incomplete parts (missing views, broken links)

# lots of borrowing from http://code.activestate.com/recipes/252508-file-unzip/

import getopt, sys, os, os.path, re, zipfile
    
def usage():
    print """
usage:
    fzpzclean.py -f [fzpz directory] -d [output directory] -o [core | contrib | user]
    unzip fzpz files into the output directory and remove all the guids from the filenames and internal names.
    """
    
        
           
def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hf:d:o:", ["help", "fzpzs", "directory", "output"])
    except getopt.GetoptError, err:
        # print help information and exit:
        print str(err) # will print something like "option -a not recognized"
        usage()
        sys.exit(2)
    inputdir = None
    outputdir = None
    outputPrefix = None
    
    for o, a in opts:
        #print o
        #print a
        if o in ("-f", "--file"):
            inputdir = a
        elif o in ("-d", "--directory"):
            outputdir = a
        elif o in ("-o", "--output"):
            outputPrefix = a
        elif o in ("-h", "--help"):
            usage()
            sys.exit(2)
        else:
            assert False, "unhandled option"
    
    if(not(inputdir)):
        usage()
        sys.exit(2)

    if(not(outputPrefix)):
        usage()
        sys.exit(2)

    if(not(outputdir)):
        usage()
        sys.exit(2)

    if not outputdir.endswith(':') and not os.path.exists(outputdir):
        os.mkdir(outputdir)
   
    for fn in os.listdir(inputdir):
        if fn.endswith('.fzpz'):
                print fn
                file = os.path.join(inputdir, fn)
                zf = zipfile.ZipFile(file)

                # create directory structure to house files
                createstructure(file, outputdir, outputPrefix)

                # extract files to directory structure
                for i, name in enumerate(zf.namelist()):
                        if not name.endswith('/'):
                                outname = re.sub('^svg\.((icon)|(breadboard)|(schematic)|(pcb))\.', '', name, 1)
                                outname = re.sub('^part\.', '', outname, 1)
                                outname = re.sub('__[0-9a-fA-F]{32}', '', outname)
                                outname = re.sub('__[0-9a-fA-F]{27}', '', outname)
                                outname = re.sub('__[0-9a-fA-F]{20}', '', outname)
                                subdir = None
                                fzp = 0
                                if outname.endswith('.fzp'):
                                        subdir = outputPrefix
                                        fzp = 1;
                                elif outname.find('icon') >= 0:
                                        subdir = 'svg/' + outputPrefix + '/icon'
                                elif outname.find('pcb') >= 0:
                                        subdir = 'svg/' + outputPrefix + '/pcb'
                                elif outname.find('schem') >= 0:
                                        subdir = 'svg/' + outputPrefix + '/schematic'
                                elif outname.find('bread') >= 0:
                                        subdir = 'svg/' + outputPrefix + '/breadboard'
                                outname = re.sub('__((icon)|(breadboard)|(schematic)|(pcb))', '', outname)
                                outfile = open(os.path.join(outputdir, subdir, outname), 'wb')
                                
                                if fzp:
                                        s = zf.read(name)
                                        s = re.sub('__[0-9a-fA-F]{32}', '', s)
                                        s = re.sub('__[0-9a-fA-F]{27}', '', s)
                                        s = re.sub('__[0-9a-fA-F]{20}', '', s)
                                        s = re.sub('__((icon)|(breadboard)|(schematic)|(pcb))', '', s)
                                        outfile.write(s)
                                else:
                                        outfile.write(zf.read(name))
                                outfile.flush()
                                outfile.close()

        
def createstructure(file, dir, outputPrefix):
    # makedirs(listdirs(file), dir)
    dirs = [outputPrefix, 'svg/' + outputPrefix + '/icon', 'svg/' + outputPrefix + '/breadboard', 'svg/' + outputPrefix + '/schematic', 'svg/' + outputPrefix + '/pcb']
    makedirs(dirs, dir)


def makedirs(directories, basedir):
    """ Create any directories that don't currently exist """
    for dir in directories:
        curdir = os.path.join(basedir, dir)
        if not os.path.exists(curdir):
            os.makedirs(curdir)

def listdirs(file):
    """ Grabs all the directories in the zip structure
    This is necessary to create the structure before trying
    to extract the file to it. """
    zf = zipfile.ZipFile(file)

    dirs = []

    for name in zf.namelist():
        if name.endswith('/'):
            dirs.append(name)

    dirs.sort()
    return dirs


if __name__ == "__main__":
    main()



