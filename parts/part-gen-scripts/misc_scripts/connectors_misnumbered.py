# usage:
#	    copper1fzp.py -d [fzp folder]
#       adds a copper1 layer if there isn't one found already.

import getopt, sys, os, os.path, re, xml.dom.minidom, xml.dom
    
def usage():
        print """
usage:
    connectors_misnumbered.py -d [fzp folder]
    checks that connectors with integer names are correctly mapped to connector numbers
"""
    
        
           
def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hd:", ["help", "directory"])
    except getopt.GetoptError, err:
        # print help information and exit:
        print str(err) # will print something like "option -a not recognized"
        usage()
        return
        
    dir = None
            
    for o, a in opts:
        #print o
        #print a
        if o in ("-d", "--directory"):
            dir = a
        elif o in ("-h", "--help"):
            usage()
            return
        else:
            assert False, "unhandled option"
            
    if(not(dir)):
        usage()
        return       

    pattern = r'(\d+)'
    numberFinder = re.compile(pattern, re.IGNORECASE)
            
    for root, dirs, files in os.walk(dir, topdown=False):
        for filename in files:
            if not filename.endswith(".fzp"): 
                continue
                
            fzpFilename = os.path.join(root, filename)
            try:
                dom = xml.dom.minidom.parse(fzpFilename)
            except xml.parsers.expat.ExpatError, err:
                print str(err), fzpFilename
                continue
                
            fzp = dom.documentElement
            connectors = fzp.getElementsByTagName("connector")
            gotInt = False
            for connector in connectors:
                try:
                    intname = int(connector.getAttribute("name"))
                    gotInt = True
                except:
                    continue
                    
            if not gotInt:
                continue
                
            idZero = False
            for connector in connectors:
                try:
                    id = connector.getAttribute("id")
                    match = numberFinder.search(id)
                    if match == None:
                        continue
        
                    if match.group(1) == '0':
                        idZero = True
                        break
                except:
                    continue
                
            nameZero = False
            for connector in connectors:
                if connector.getAttribute("name") == "0":
                    nameZero = True
                    break
                    
               
            mismatches = []
            for connector in connectors:
                idInt = 0
                nameInt = 0
                try:
                    id = connector.getAttribute("id")
                    match = numberFinder.search(id)
                    if match == None:
                        continue
        
                    idInt = int(match.group(1))
                    nameInt = int(connector.getAttribute("name"))
                    
                except:
                    continue
                    
                mismatch = False
                if nameZero and idZero:
                    mismatch = (idInt != nameInt)
                elif nameZero:
                    mismatch = (idInt != nameInt + 1)
                elif idZero:
                    mismatch = (idInt + 1 != nameInt)
                else:
                    mismatch = (idInt != nameInt)
                if mismatch:
                    mismatches.append(connector)
            
            if len(mismatches) > 0:
                print fzpFilename, nameZero, idZero
                for connector in mismatches:
                    strings = connector.toxml().split("\n")
                    print strings[0]
                print
                    
            
if __name__ == "__main__":
        main()



