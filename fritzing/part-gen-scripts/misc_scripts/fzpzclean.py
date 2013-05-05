#	TODO:
#		check for incomplete parts (missing views)
#       check if svgs are plain copies of already existing ones and link them instead?
#       check for conflicting names

# lots of borrowing from http://code.activestate.com/recipes/252508-file-unzip/

import getopt, sys, os, os.path, re, zipfile
    
def usage():
    print """
usage:
    fzpzclean.py -f [fzpz directory] -d [output directory] -o [core | contrib | user] (-r)

    -f   directory of input fzpzs
    -d   directory for output of the cleanup process
    -o   parent folder name, depending on fzp category
    -r   optional: take the fzpz filename to rename the fzp filename

Unzips fzpz files into the output directory and cleans up file names and references.
"""
    
        
           
def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hf:d:o:r", ["help", "fzpzs", "directory", "output", "rename"])
    except getopt.GetoptError, err:
        # print help information and exit:
        print str(err) # will print something like "option -a not recognized"
        usage()
        sys.exit(2)
    inputdir = None
    outputdir = None
    outputPrefix = None
    rename = False
    
    for o, a in opts:
        #print o
        #print a
        if o in ("-f", "--fzpzs"):
            inputdir = a
        elif o in ("-d", "--directory"):
            outputdir = a
        elif o in ("-o", "--output"):
            outputPrefix = a
        elif o in ("-r", "--rename"):
            rename = True
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

                # record svg renamings and fzp location for fixing paths later
                svgrenames = []
                fzplocation = None

                # extract files to directory structure
                for i, name in enumerate(zf.namelist()):
                        if not name.endswith('/'):

                                # identify file type
                                ending = name.split('.')[-1]
                                if ending == 'svg':
                                        pass
                                elif ending == 'fzp':
                                        pass
                                else:
                                        print "WARNING wrong file type:", name
                                        return

                                # sort files into subdirectories
                                subdir = None
                                if ending == 'fzp':
                                        subdir = outputPrefix
                                elif name.find('icon') >= 0:
                                        subdir = 'svg/' + outputPrefix + '/icon'
                                elif name.find('pcb') >= 0:
                                        subdir = 'svg/' + outputPrefix + '/pcb'
                                elif name.find('schem') >= 0:
                                        subdir = 'svg/' + outputPrefix + '/schematic'
                                elif name.find('bread') >= 0:
                                        subdir = 'svg/' + outputPrefix + '/breadboard'

                                # remove junk from filenames
                                outname = name
                                outname = re.sub('^svg\.((icon)|(breadboard)|(schematic)|(pcb))\.', '', outname, 1)
                                refname = outname  # this is how the svgs are referenced in the fzp
                                outname = re.sub('^part\.', '', outname, 1)
                                outname = re.sub('((__)|(_))[0-9a-fA-F]{32}', '', outname)
                                outname = re.sub('((__)|(_))[0-9a-fA-F]{27}', '', outname)
                                outname = re.sub('((__)|(_))[0-9a-fA-F]{20}', '', outname)
                                outname = re.sub('((__)|(_))((icon)|(breadboard)|(schematic)|(pcb))', '', outname)
                                # remove duplicate file endings
                                outname = re.sub('.' + ending, '', outname)
                                outname += '.' + ending

                                # optionally rename the fzp to that of the fzpz
                                if ending == 'fzp' and rename:
                                        base = os.path.basename(zf.filename)
                                        outname = os.path.splitext(base)[0]
                                        outname += '.fzp'

                                # write new files
                                filelocation = os.path.join(outputdir, subdir, outname)
                                outfile = open(filelocation, 'wb')
                                outfile.write(zf.read(name))
                                outfile.flush()
                                outfile.close()

                                # store paths for reference update
                                if ending == 'svg':
                                        svgrenames.append((refname, outname))
                                elif ending == 'fzp':
                                        fzplocation = filelocation

                # update svg references in fzp
                fzpfile = open(fzplocation, 'r+')
                s = fzpfile.read()
                for svgre in svgrenames:
                        if s.find(svgre[0]) == -1:
                                print "WARNING reference could not be found:", svgre[0]
                        s = s.replace(svgre[0], svgre[1])
                fzpfile.seek(0)
                fzpfile.truncate()
                fzpfile.write(s)
                fzpfile.flush()
                fzpfile.close()


        
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



