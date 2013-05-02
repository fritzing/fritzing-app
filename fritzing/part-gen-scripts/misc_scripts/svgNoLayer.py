# usage:
#    svgNoLayer.py -d <directory> 
#    
#    <directory> is a folder, with subfolders, containing .svg files.  In each svg file in the directory or its children
#    look for id='[layer]' where layer is the set of all layers in fritzing

import getopt, sys, os, re, xml.dom.minidom, xml.dom
    
def usage():
    print """
usage:
    svgNoLayer.py -d [directory]
    
    <directory> is a folder, with subfolders, containing .svg files.  In each svg file in the directory or its children 
    look for id='[layer]' where layer is the set of all layers in fritzing
    """
layers = ["icon","breadboardbreadboard", "breadboard", "breadboardWire", "breadboardLabel", "breadboardNote", "breadboardRuler", "schematic", "schematicWire","schematicTrace","schematicLabel", "schematicRuler", "board", "ratsnest", "silkscreen", "silkscreenLabel", "groundplane", "copper0", "copper0trace", "groundplane1", "copper1", "copper1trace", "silkscreen0", "silkscreen0Label", "soldermask",  "outline",  "keepout", "partimage", "pcbNote", "pcbRuler"]

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
        
 
    for root, dirs, files in os.walk(outputDir, topdown=False):
        for filename in files:
            if (filename.endswith(".svg")):  
                infile = open(os.path.join(root, filename), "r")
                svg = infile.read()
                infile.close()
                match = None
                for layer in layers:
                    match = re.search('id=[\'\"]' + layer, svg)
                    if (match != None):
                        break
                        
                if match == None:
                    print "{0} {1}".format(os.path.join(root, filename), "has no layer ids")
                else:
        
                   msg = parseIDs(svg)
                   if (msg != None):
                        print "{0} {1}".format(os.path.join(root, filename), msg)
                        

def parseIDs(svg):
    try:
        dom = xml.dom.minidom.parseString(svg)
    except xml.parsers.expat.ExpatError, err:
        return "xml error " + str(err)
    
    root = dom.documentElement
    id = root.getAttribute("id")
    if id != None:
        for layer in layers:
            if (layer == id):
                return "svg contains layer id " + id
     
    for c in root.childNodes:
        if c.nodeType != c.ELEMENT_NODE:
            continue
            
        tag = c.tagName
        if tag == "metadata":
            continue
        if tag == "title":
            continue
        if tag == "desc":
            continue
        if tag == "defs":
            continue
        if tag == "sodipodi:namedview":
            continue
            
        gotOne = 0
        id = c.getAttribute("id")
        if (id != None):
            for layer in layers:
                if (layer == id):
                    gotOne = 1
                    break
        
        if gotOne == 0:
            return "child element '" + tag + "' with no layer id"
    
    return None
    
    
if __name__ == "__main__":
    main()

