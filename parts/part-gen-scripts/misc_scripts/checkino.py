#!/usr/bin/python
# -*- coding: utf-8 -*-

# lots of borrowing from http://code.activestate.com/recipes/252508-file-unzip/

import getopt, sys, os, os.path, re, zipfile, shutil, xml.dom.minidom, xml.dom
import xml.etree.ElementTree as ET
    
def usage():
    print """
usage:
    checkino.py -f [from directory] -e [=yes extract .ino files]

For each fzz file in the from directory, see whether the fz specifies an ino file and if it matches the ino file name.  Probably safest to make a copy of the from directory first
"""
           
def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "e:f:", ["extract", "pull"])
    except getopt.GetoptError, err:
        # print help information and exit:
        print str(err) # will print something like "option -a not recognized"
        usage()
        return
    
    inputDir = None
    extract = None
    
    for o, a in opts:
        #print o
        #print a
        if o in ("-f", "--from"):
            inputDir = a
        elif o in ("-e", "--extract"):
            extract = a
        else:
            print "unhandled option",o
            usage()
            return
    
    if not(inputDir):
        print "missing 'from' argument"
        usage()
        return
        
    try:
        import zlib
        compression = zipfile.ZIP_DEFLATED
    except:
        compression = zipfile.ZIP_STORED

    for root, dirs, files in os.walk(inputDir, topdown=False):
        for fzz in files:
            if not fzz.endswith('.fzz'):
                continue
                
            #print fzz
            fzzpath = os.path.join(root, fzz)
            
            tempDir = inputDir + os.sep + "___temp___"
            shutil.rmtree(tempDir, 1)
            os.mkdir(tempDir)

            zf = zipfile.ZipFile(fzzpath)
            zf.extractall(tempDir)
            zf.close()
            
            fzzbase = os.path.splitext(fzz)[0]
            
            renamed = False
            
            inos = []
            for fz in os.listdir(tempDir):
                if fz.endswith(".ino"):
                    if extract == "yes":
                        inoPath = os.path.join(tempDir, fz)
                        shutil.copyfile(inoPath, os.path.join(inputDir, fz))
                        print "copy", inoPath, os.path.join(inputDir, fz)
                    inos.append(fz)
                    continue
                    
                if not fz.endswith(".fz"):
                    continue
                    
                try: 
                    fzpath = os.path.join(tempDir, fz)
                    tree = ET.parse(fzpath)
                    top = tree.getroot()
                    for program in top.iter('program'):
                        print "program", program.text, fzzpath
                        
                except:
                    print "exception", fzpath, sys.exc_info()[0]
                    return    

            for ino in inos:
                print "    ino", ino, fzzpath

            if renamed:
                os.remove(fzzpath)
                zf = zipfile.ZipFile(fzzpath, mode='w')
                for fn in os.listdir(tempDir):
                    zf.write(os.path.join(tempDir, fn), fn, compression)
                zf.close()

            shutil.rmtree(tempDir, 1)

if __name__ == "__main__":
    main()



