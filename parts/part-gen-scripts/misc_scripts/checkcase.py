
import sys, os, os.path, re, xml.dom.minidom, xml.dom,  optparse
    
def usage():
        print """
usage:
    checkcase.py -f [fzp folder] -s [svg folder]
    ensure all fzp files case-sensitively match svg file names
"""
    
        
           
def main():
    parser = optparse.OptionParser()
    parser.add_option('-f', '--fzp', dest="fzpdir" )
    parser.add_option('-s', '--svg', dest="svgdir" )
    (options, args) = parser.parse_args()
        
    if not options.fzpdir:
        usage()
        parser.error("fzp dir argument not given")
        return       
            
    if not options.svgdir:
        usage()
        parser.error("svg dir argument not given")
        return   

    allsvgs = []
    lowersvgs = {}
    for root, dirs, files in os.walk(options.svgdir, topdown=False):
        for filename in files:
            if not filename.endswith(".svg"): 
                continue
            path = os.path.join(root, filename)
            allsvgs.append(path)
            lowersvgs[path.lower()] = filename
            
    for root, dirs, files in os.walk(options.fzpdir, topdown=False):
        for filename in files:
            if not filename.endswith(".fzp"): 
                continue
                
            fzpFilename = os.path.join(root, filename)
            try:
                dom = xml.dom.minidom.parse(fzpFilename)
            except xml.parsers.expat.ExpatError, err:
                print str(err), fzpFilename
                continue
             
            doUpdate = False
            fzp = dom.documentElement
            layerss = fzp.getElementsByTagName("layers")
            for layers in layerss:
                image = layers.getAttribute("image").replace("/", "\\")
                if ("dip_" in image) and ("mil_" in image):
                    continue
                if ("sip_" in image) and ("mil_" in image):
                    continue
                if ("jumper_" in image) and ("mil_" in image):
                    continue
                if ("screw_terminal_" in image):
                    continue
                if ("jumper" in image):
                    continue
                if ("mystery_" in image):
                    continue
                if ("LED-" in image):
                    continue
                if ("axial_lay" in image):
                    continue
                if ("resistor_" in image):
                    continue
                if ("generic" in image) and ("header" in image):
                    continue
                path1 = os.path.join(options.svgdir, "core", image)
                path2 = os.path.join(options.svgdir, "contrib", image)
                path3 = os.path.join(options.svgdir, "obsolete", image)
                if os.path.isfile(path1) or os.path.isfile(path2) or os.path.isfile(path3):
                    for path in [path1, path2, path3]:
                        try:
                            handle = open(path)
                            if not path in allsvgs:
                                print "mismatch", fzpFilename
                                print "\t", path
                                print
                                thing = layers.getAttribute("image").split("/")
                                thing[1] = lowersvgs[path.lower()]
                                layers.setAttribute("image", "/".join(thing))
                                doUpdate = True
                                
                        except:
                            pass
                else:
                    print "missing", fzpFilename, image
                
            
            if doUpdate:
                outfile = open(fzpFilename, 'wb')
                s = dom.toxml("UTF-8")
                outfile.write(s)
                outfile.close()                    
                

if __name__ == "__main__":
        main()



