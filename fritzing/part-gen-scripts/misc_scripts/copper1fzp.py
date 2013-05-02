# usage:
#	    copper1fzp.py -d [fzp folder]
#       adds a copper1 layer if there isn't one found already.

import getopt, sys, os, os.path, re, xml.dom.minidom, xml.dom
    
def usage():
        print """
usage:
    copper1fzp.py -d [fzp folder]
    adds a copper1 layer if there isn't one found already.
"""
    
        
           
def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hd:", ["help", "directory"])
    except getopt.GetoptError, err:
        # print help information and exit:
        print str(err) # will print something like "option -a not recognized"
        usage()
        sys.exit(2)
        
    dir = None
            
    for o, a in opts:
        #print o
        #print a
        if o in ("-d", "--directory"):
            dir = a
        elif o in ("-h", "--help"):
            usage()
            sys.exit(2)
        else:
            assert False, "unhandled option"
            
    if(not(dir)):
        usage()
        sys.exit(2)            
            
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
            pcbViewNodes = fzp.getElementsByTagName("pcbView")
            needsCopper1 = 0
            for pcbView in pcbViewNodes:
                layersNodes = pcbView.getElementsByTagName("layers")
                if layersNodes.length == 1:
                    viewsLayers = layersNodes.item(0)
                    copper0 = None
                    copper1 = None
                    for layer in viewsLayers.getElementsByTagName("layer"):
                        if layer.getAttribute("layerId") == "copper1":
                            copper1 = layer
                        elif layer.getAttribute("layerId") == "copper0":
                            copper0 = layer
                
                    if not copper1 and copper0:
                        needsCopper1 = 1
                        copper1 = dom.createElement("layer")
                        copper1.setAttribute("layerId", "copper1")
                        viewsLayers.appendChild(copper1)
                    else:
                        break
                        
                else:
                    connectorsView = pcbView
                    copper1  = None
                    copper0 = None
                    for p in connectorsView.getElementsByTagName("p"):
                        if p.getAttribute("layer") == "copper1":
                            copper1 = p
                        elif p.getAttribute("layer") == "copper0":
                            copper0 = p
                
                    if copper0 and not copper1:
                        copper1 = dom.createElement("p")
                        copper1.setAttribute("layer", "copper1")
                        copper1.setAttribute("svgId", copper0.getAttribute("svgId"))
                        connectorsView.appendChild(copper1)
                    
            if not needsCopper1:
                print "no copper1 needed for", fzpFilename
                continue
                  
            print "adding copper1 to", fzpFilename
            outfile = open(fzpFilename, 'wb')
            s = dom.toxml("UTF-8")
            outfile.write(s)
            outfile.flush()
            outfile.close()                        
            
if __name__ == "__main__":
        main()



