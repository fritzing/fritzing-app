#!/usr/bin/python
# -*- coding: utf-8 -*-

# lots of borrowing from http://code.activestate.com/recipes/252508-file-unzip/

import getopt, sys, os, os.path, re, zipfile, shutil, xml.dom.minidom, xml.dom
import xml.etree.ElementTree as ET
    
def usage():
    print """
usage:
    fix.py -f [from directory]


For each fzz file in the to directory, see whether the fz specifies an ino file and if it matches the ino file name.
Clean the pid and the path. If there is nothing or no match, see what's in the equivalent file in the from folder.
Note that the from folder and to folder should point to the same level of hierarchy (e.g. each points to the 'sketches' folder) 
Probably safest to make a copy of the from directory first
"""
           
def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "f:", ["from"])
    except getopt.GetoptError, err:
        # print help information and exit:
        print str(err) # will print something like "option -a not recognized"
        usage()
        return
    
    fromDir = None
    
    for o, a in opts:
        #print o
        #print a
        if o in ("-f", "--from"):
            fromDir = a
        else:
            print "unhandled option",o
            usage()
            return
    
    if not(fromDir):
        print "missing 'from' argument"
        usage()
        return
        
    try:
        import zlib
        compression = zipfile.ZIP_DEFLATED
    except:
        compression = zipfile.ZIP_STORED

    for root, dirs, files in os.walk(toDir, topdown=False):
        for fzz in files:
            if not fzz.endswith('.fzz'):
                continue
                
            #print fzz
            fzzpath = os.path.join(root, fzz)
            inopath = fzzpath.replace(".fzz", ".ino")
            ino = os.path.basename(inoPath)
            gotIno = os.path.isfile(inopath)
            
            tempDir =toDir + os.sep + "___temp___"
            shutil.rmtree(tempDir, 1)
            os.mkdir(tempDir)

            temp2Dir =toDir + os.sep + "___temp2___"
            shutil.rmtree(temp2Dir, 1)
            os.mkdir(temp2Dir)

            zf = zipfile.ZipFile(fzzpath)
            zf.extractall(tempDir)
            zf.close()
            
            fzzbase = os.path.splitext(fzz)[0]
            
            modified = False
        
            for fz in os.listdir(tempDir):
                if fz.endswith(".ino"):
                    os.path.remove(os.path.join(tempDir, fz))
                    modified = True
                    continue
                    
                if not fz.endswith(".fz"):
                    continue
                    
                try: 
                    fzpath = os.path.join(tempDir, fz)
                    tree = ET.parse(fzpath)
                    top = tree.getroot()
                    programs = []
                    for ps in top.findall('programs'):
                        programs.append(ps)
                    for ps in programs:
                        fzmodified = True
                        top.remove(ps)
                    
                    if gotIno:
                        shutil.copyfile(inoPath, os.path.join(tempDir, ino)
                        ps = ET.SubElement(top, "programs")
                        ps.set("pid", "")
                        program = ET.SubElement(ps, "program")
                        program.set("language", "Arduino")
                        program.set("programmer", "")
                        program.text = ino
                        fzmodified = True
                    
                    if fzmodified:
                        modified = True
                        tree.write(fzpath)
                except:
                    print "exception", fzpath, sys.exc_info()[0]
                    return                   

            if modified:
                os.remove(fzzpath)
                zf = zipfile.ZipFile(fzzpath, mode='w')
                for fn in os.listdir(tempDir):
                    zf.write(os.path.join(tempDir, fn), fn, compression)
                zf.close()

            shutil.rmtree(tempDir, 1)

if __name__ == "__main__":
    main()



