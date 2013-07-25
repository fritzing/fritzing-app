
import getopt, sys, os, os.path, re, xml.dom.minidom, xml.dom
    
def usage():
        print """
usage:
    unusedsvgs.py -f [fzp folder] -s [svg folder]
    lists orphan svgs
"""
    
        
           
def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hf:s:", ["help", "fzp", "svg"])
    except getopt.GetoptError, err:
        # print help information and exit:
        print str(err) # will print something like "option -a not recognized"
        usage()
        return
        
    fzpdir = None
    svgdir = None
            
    for o, a in opts:
        #print o
        #print a
        if o in ("-f", "--fzp"):
            fzpdir = a
        elif o in ("-s", "--svg"):
            svgdir = a
        elif o in ("-h", "--help"):
            usage()
            return
        else:
            print "unhandled option", o
            return
            
    if not fzpdir:
        print "missing fzp folder argument"
        usage()
        return           
            
    if not svgdir:
        print "missing svg folder argument"
        usage()
        return  

    svgfiles = {}
    for root, dirs, files in os.walk(svgdir, topdown=False):
        for filename in files:
            if not filename.endswith(".svg"): 
                continue
            
            basename = os.path.basename(root)
            if svgfiles.get(basename) == None:
                svgfiles[basename] = []
                
            svgfiles[basename].append(filename)
            
    viewnames = ["iconView", "breadboardView", "schematicView", "pcbView"]
            
    for root, dirs, files in os.walk(fzpdir, topdown=False):
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
            for viewname in viewnames:
                viewNodes = fzp.getElementsByTagName(viewname)
                for viewNode in viewNodes:
                    layersNodes = viewNode.getElementsByTagName("layers")
                    for layersNode in layersNodes:
                        image = layersNode.getAttribute("image")
                        dn = os.path.dirname(image)
                        viewFiles = svgfiles.get(dn)
                        if viewFiles:
                            fn = os.path.basename(image)
                            try:
                                if fn in viewFiles:
                                    print "{0} uses {1}/{2}".format(os.path.basename(root), dn, fn)
                                    viewFiles.remove(fn)
                            except:
                                pass
                        
                        
    for key in svgfiles.keys():
        for name in svgfiles.get(key):
            print "unused {0}/{1}".format(key, name)

            
if __name__ == "__main__":
        main()



