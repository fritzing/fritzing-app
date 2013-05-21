
import getopt, sys, os, os.path, re
import xml.etree.ElementTree as ETree
import traceback
    
def usage():
        print """
usage:
    syncconnectors.py -o { old fzp file } -n { new fzp file } -c { output csv file } -a
    adds 'replacedby' attribute to old connectors to point to new ones, when a partial name match is found
    overwrites the old fzp file, and saves record of changes to the csv file. 
    to append to the csv file rather than overwrite it, use the -a option
"""

IndexFinder = None

def getIndex(element):
    global IndexFinder
    
    id = element.attrib.get("id")
    if id == None:
        return -1
        
    match = IndexFinder.search(id)
    if match == None:
        return -1
        
    return int(match.group(1))
        
    
           
def main():
    global IndexFinder
    
    try:
        opts, args = getopt.getopt(sys.argv[1:], "ho:n:c:a", ["help", "old", "new","csv","append"])
    except getopt.GetoptError, err:
        # print help information and exit:
        print str(err) # will print something like "option -a not recognized"
        usage()
        return
        
    oldfn = None
    newfn = None
    csvfn = None
    append = False
            
    for o, a in opts:
        #print o
        #print a
        if o in ("-o", "--old"):
            oldfn = a
        elif o in ("-n", "--new"):
            newfn = a
        elif o in ("-c", "--csv"):
            csvfn = a
        elif o in ("-a", "--append"):
            append = True
        elif o in ("-h", "--help"):
            usage()
            return
        else:
            assert False, "unhandled option"
            
    if oldfn == None:
        print "'old' argument missing"
        usage()
        return  
        
    if newfn == None:
        print "'new' argument missing"
        usage()
        return     

    if csvfn == None:
        print "'csv' argument missing"
        usage()
        return    
        
    if not append:
        try:
            os.remove(csvfn)
        except:
            pass
            
    csvfile = None
    try:
        flags = "w"
        if append:
            flags = "a"
        csvfile = open(csvfn, flags)
    except:
        pass
        
    if csvfile == None:
        print "unable to open", csvfn
        usage()
        return
    
    oldroot = None
    newroot = None
    try:
        oldroot = ETree.parse(oldfn)
        newroot = ETree.parse(newfn)
    except :
        info = sys.exc_info()
        traceback.print_exception(info[0], info[1], info[2])
        return
    
    pattern = r'(\d+)'
    IndexFinder = re.compile(pattern, re.IGNORECASE)
        
    newelements = newroot.findall(".//connector")
    newelements.sort(key=getIndex)
    newelementmarkers = [None] * len(newelements)
    
    oldelements = oldroot.findall(".//connector")
    oldelements.sort(key=getIndex)
    oldelementmarkers = [None] * len(oldelements)
    
    for oldix in range(0, len(oldelements)):
        oldelement = oldelements[oldix]
        oldname = oldelement.attrib.get("name")
        for newix in range(0, len(newelements)):
            if newelementmarkers[newix] != None:
                continue
                
            newelement = newelements[newix]
            if newelement.attrib.get("name") == oldname:
                oldelementmarkers[oldix] = newelements[newix]
                newelementmarkers[newix] = oldelements[oldix]
                break
                
    for oldix in range(0, len(oldelements)):
        if oldelementmarkers[oldix] != None:
            continue
            
        oldelement = oldelements[oldix]
        oldname = oldelement.attrib.get("name").lower()
        for newix in range(0, len(newelements)):
            if newelementmarkers[newix] != None:
                continue
                
            newelement = newelements[newix]
            newname = newelement.attrib.get("name").lower()
            if (newname in oldname) or (oldname in newname):
                oldelementmarkers[oldix] = newelements[newix]
                newelementmarkers[newix] = oldelements[oldix]
                oldelement.attrib['replacedby'] = newelements[newix].attrib["name"]
                break
        
    for oldix in range(0, len(oldelements)):
        if oldelementmarkers[oldix] != None:
            continue

        oldelement = oldelements[oldix]
        oldname = oldelement.attrib.get("name").lower()
        for newix in range(0, len(newelements)):
            if newelementmarkers[newix] != None:
                continue

            newelement = newelements[newix]
            newname = newelement.attrib.get("name").lower()
            if (oldname == "rst" and newname == "reset") or ("d0" in oldname and "d0" in newname) or ("d1" in oldname and "d1" in newname):
                oldelementmarkers[oldix] = newelements[newix]
                newelementmarkers[newix] = oldelements[oldix]
                oldelement.attrib['replacedby'] = newelements[newix].attrib["name"]
                break
     
    formatstring = ",{oldid},{oldname},{action},{newid},{newname}\n"
    
    if append:
        csvfile.write("\n")
        
    csvfile.write("{0} => {1},,,,,\n".format(os.path.basename(oldfn), os.path.basename(newfn)))
    
    for oldix in range(0, len(oldelements)):
        oldelement = oldelements[oldix]
        oldmarker =  oldelementmarkers[oldix]
        oldid = oldelement.attrib.get("id")
        oldname = oldelement.attrib.get("name")
        if oldmarker == None:
            csvfile.write(formatstring.format(oldid=oldid, oldname=oldname, action="not found", newid="", newname=""))
            continue
        
        newid = oldmarker.attrib.get("id")
        newname = oldmarker.attrib.get("name")
        if oldname.lower() == newname.lower():
            csvfile.write(formatstring.format(oldid=oldid, oldname=oldname, action="same", newid=newid, newname=newname))
            continue
            
        csvfile.write(formatstring.format(oldid=oldid, oldname=oldname, action="replacedby", newid=newid, newname=newname))
        
    for newix in range(0, len(newelements)):
        newelement = newelements[newix]
        newmarker =  newelementmarkers[newix]
        newid = newelement.attrib.get("id")
        newname = newelement.attrib.get("name")
        if newmarker == None:
            csvfile.write(formatstring.format(oldid="", oldname="", action="not found", newid=newid, newname=newname))

    csvfile.close()
    oldroot.write(oldfn)
    
if __name__ == "__main__":
        main()



