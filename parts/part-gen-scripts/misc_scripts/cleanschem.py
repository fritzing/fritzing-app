
import getopt, sys, os, os.path, re, xml.dom.minidom, xml.dom
    
def usage():
        print """
usage:
    cleanschem.py -d [svg folder]
    cleans files with multiple copies of internal rects and labels
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
            print "unhandled option", o
            usage()
            return
        
            
    if dir == None:
        print "missing directory argument"
        usage()
        return
          
            
    for root, dirs, files in os.walk(dir, topdown=False):
        for filename in files:
            if not filename.endswith(".svg"): 
                continue
                
            svgFilename = os.path.join(root, filename)
            handlesvg(svgFilename)

                
def handlesvg(svgFilename):
    try:
        dom = xml.dom.minidom.parse(svgFilename)
    except xml.parsers.expat.ExpatError, err:
        print str(err), svgFilename
        return
        
    #print svgFilename
    rcount = 0
    toDelete = []
    svg = dom.documentElement
    rectNodes = svg.getElementsByTagName("rect")
    for rect in rectNodes:
        if rect.getAttribute("class") == "interior rect":
            rcount += 1
            if rcount > 1:
                toDelete.append(rect)
                print "\t", rect.toxml("UTF-8")
                
    #print rcount, toDelete
    
    textNodes = svg.getElementsByTagName("text")
    labels = []
    fix = False
    for text in textNodes:
        id = text.getAttribute("id")
        if id.startswith("label"):
            labels.append(text)
            if "_" in id:
                #fix = True
                #text.setAttribute("id", id.replace("_", ""))
                #print "id", id
                pass
            
    if len(labels) > 0:
        for ix in range(len(labels)):
            labeli = labels[ix]
            for jx in range(ix + 1, len(labels)):
                labelj = labels[jx]
                if labeli.toxml("UTF-8") == labelj.toxml("UTF-8"):
                    toDelete.append(labeli)
                    print "\t", labeli.toxml("UTF-8")
                    break
                
    for node in toDelete:
        node.parentNode.removeChild(node)
        
    if len(toDelete) > 0 or fix:
        print "fixing", svgFilename
        outfile = open(svgFilename, 'wb')
        s = dom.toxml("UTF-8")
        outfile.write(s)
        outfile.close()                    
    


            
if __name__ == "__main__":
        main()



