#!/usr/bin/python
# -*- coding: utf-8 -*-

# lots of borrowing from http://code.activestate.com/recipes/252508-file-unzip/

import getopt, sys, os, os.path, re, zipfile, shutil, xml.dom.minidom, xml.dom
import xml.etree.ElementTree as ET
    
def usage():
    print """
usage:
    fix.py -f [from directory]


For each fzz file in the from directory, remove all .ino files inside the fzz and <program> elements in the internal fz file.
if the fzz has a matching .ino file (external, in the same folder) then copy the ino file inside the fzz and add a <program> element to the fz
Afterwards you must delete the  external.ino files 
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

    for root, dirs, files in os.walk(fromDir, topdown=False):
        for fzz in files:
            if not fzz.endswith('.fzz'):
                continue
                
            #print fzz
            fzzpath = os.path.join(root, fzz)
            inopath = fzzpath.replace(".fzz", ".ino")
            ino = os.path.basename(inopath)
            gotIno = os.path.isfile(inopath)
            
            tempDir =fromDir + os.sep + "___temp___"
            shutil.rmtree(tempDir, 1)
            os.mkdir(tempDir)

            zf = zipfile.ZipFile(fzzpath)
            zf.extractall(tempDir)
            zf.close()
                        
            modified = False
            
            print fzzpath
        
            # remove old ino files first
            for fino in os.listdir(tempDir):
                if fino.endswith(".ino"):
                    print "    remove old", fino
                    os.remove(os.path.join(tempDir, fino))
                    modified = True

            for fz in os.listdir(tempDir):                    
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
                        print "    remove old programs"
                        top.remove(ps)
                    
                    if gotIno:
                        shutil.copyfile(inopath, os.path.join(tempDir, ino))
                        ps = ET.SubElement(top, "programs")
                        ps.set("pid", "")
                        program = ET.SubElement(ps, "program")
                        program.set("language", "Arduino")
                        program.set("programmer", "")
                        program.text = ino
                        fzmodified = True
                        print "    copy", ino
                    
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



