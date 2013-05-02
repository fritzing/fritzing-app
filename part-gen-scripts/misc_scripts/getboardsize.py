# usage:
#   getboardsize.py -f <fzzfile>
#    
#   Unzip and parse the fzzfile looking for a board.  
#   If only one board is found, return its size (x by y) in millimeters.
#   Otherwise return an error.

import getopt, sys, os, re, xml.dom.minidom, xml.dom, zipfile
    
def usage():
    print """
usage:
    getboardsize.py -f <fzzfile> 
    
    Unzip and parse the fzzfile looking for a board.  
    If only one board is found, return its size (x by y) in millimeters.
    Otherwise return an error.
"""

def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hd:f:", ["help", "file"])
    except getopt.GetoptError, err:
        # print help information and exit:
        print str(err) # will print something like "option -a not recognized"
        usage()
        sys.exit(2)
    fzz = None
    
    for o, a in opts:
        #print o
        #print a
        if o in ("-f", "--file", "--fzz"):
            fzz = a
        elif o in ("-h", "--help"):
            usage()
            sys.exit(2)
        else:
            assert False, "unhandled option"
        
    if not(fzz):
        usage()
        sys.exit(2)
        
    if not os.path.exists(fzz):
        assert False, "'" + fzz + "' not found"
        
    if not fzz.endswith("fzz"):
        assert False, "'" + fzz + "' doesn't end with 'fzz'"
 
    zf = None
    try:
        zf = zipfile.ZipFile(fzz) 
    except:
        assert False, "'" + fzz + "' is not a zip file"
    
    fzString = None
    for i, name in enumerate(zf.namelist()):   
        if name.endswith('fz'):
            fzString = zf.read(name)
            break
    
    assert fzString, "no 'fz' file found in '" + fzz + "'"
    
    fzdom = None
    #try:
    fzdom = xml.dom.minidom.parseString(fzString)
    #except:
        #assert False, "unable to parse '" + fzz + "'"
        
    root = fzdom.documentElement
    assert root.tagName == "module", "'" + fzz + "' has no 'module' root"
    
    instances = root.getElementsByTagName("instance")
    assert instances.length > 0, "'" + fzz + "' has no instances"
    
    pairs = []
    customPairs = {}
    
     #now look for custom boards
    for i, name in enumerate(zf.namelist()):   
        if name.endswith('fzp'):
            fzpString = zf.read(name)
            dom = None
            try:
                dom = xml.dom.minidom.parseString(fzpString)
            except:
                continue
                
            if dom == None:
                continue
                
            root = dom.documentElement
            if root.tagName != 'module':
                continue
                
            custom = None
            titles = root.getElementsByTagName("title")
            for title in titles:
                titleString = getText(title.childNodes)
                if "Custom PCB" in titleString:
                    custom = 1
                    break
                    
            if not custom:
                continue
                
            descriptions = root.getElementsByTagName("description")
            for description in descriptions:
                descriptionString = getText(description.childNodes)
                floatMatchString = r'([0-9]*\.?[0-9]+)'
                fullMatchString = floatMatchString + r"x" + floatMatchString + r"mm"
                match = re.search(fullMatchString, descriptionString)
                if not match:
                    continue
                    
                #print "match", match.groups()
                    
                if len(match.groups()) < 2:
                    continue
                    
                w = float(match.group(1))
                h = float(match.group(2))
                customPairs[root.getAttribute("moduleId")] = [w,h]
                break
   
    #now look for board instances in the fz file:
    for instance in instances:
        id = instance.getAttribute('moduleIdRef')
        if id == 'RectanglePCBModuleID' or id == 'TwoLayerRectanglePCBModuleID':
            w = None
            h = None
            properties = instance.getElementsByTagName("property")
            for property in properties:
                pname = property.getAttribute("name")
                if pname == "width":
                    w = float(property.getAttribute("value"))
                elif pname == "height":
                    h = float(property.getAttribute("value"))
            pairs.append(w)
            pairs.append(h)
        elif id == '423120090505' or id == '423120090505_2':           #arduino shield
            pairs.append(69.215)				#width="2.725in" 
            pairs.append(53.37556)			#height="2.1014in" 
        else:
            pair = None
            try:
                pair = customPairs[id]
            except:
                pass
                
            if pair != None:
                pairs.append(pair[0])
                pairs.append(pair[1])
     
    print "pairs", pairs
    
    assert len(pairs) >= 2, "no boards found in '" + fzz + "'"
    assert len(pairs) == 2, "multiple boards found in '" + fzz + "'"
    
    print "result", pairs
        
    return pairs       
 
def getText(nodelist):
    rc = []
    for node in nodelist:
        if node.nodeType == node.TEXT_NODE:
            rc.append(node.data)
    return ''.join(rc) 
    
    
if __name__ == "__main__":
    main()

