# usage:
#	bbhack.py -i [breadboard input file] -o [breadboard output file]
#	 loads the breadboard input file, simplifies the connectors, and saves it to the breadboard output file

import getopt, sys, os, os.path, re, xml.dom.minidom, xml.dom
    
def usage():
        print """
usage:
    bbhack.py -i [breadboard input file] -o [breadboard output file]
    loads the breadboard input file, simplifies the connectors, and saves it to the breadboard output file.
"""
    
        
           
def main():
        try:
                opts, args = getopt.getopt(sys.argv[1:], "hi:o:", ["help", "input", "output"])
        except getopt.GetoptError, err:
                # print help information and exit:
                print str(err) # will print something like "option -a not recognized"
                usage()
                sys.exit(2)
        input = None
        output = None
            
        for o, a in opts:
                #print o
                #print a
                if o in ("-i", "--input"):
                        input = a
                elif o in ("-o", "--output"):
                        output = a
                elif o in ("-h", "--help"):
                        usage()
                        sys.exit(2)
                else:
                        assert False, "unhandled option"
            
        if(not(input)):
                usage()
                sys.exit(2)

        if(not(output)):
                usage()
                sys.exit(2)
        
        dom = xml.dom.minidom.parse(input)
        root = dom.documentElement
        gNodes = root.getElementsByTagName("g")
        socketsNode = None
        for g in gNodes:
                if g.getAttribute("id") == "sockets":
                        socketsNode = g
                        break
                
        if not socketsNode:
                print "no sockets node found"
                sys.exit(0)
                
        rects = []
        for r in root.childNodes:
                if r.nodeType != g.ELEMENT_NODE:
                        continue
                        
                if r.tagName != "rect":
                        continue
                        
                id = r.getAttribute("id")
                if not id:
                        continue
                 
                if id.find("pin") >= 0:
                        rects.append(r)
                        
        for r in rects:
                root.removeChild(r)
                r.unlink()
                
        for g in socketsNode.childNodes:
                if g.nodeType != g.ELEMENT_NODE:
                        continue
                        
                if g.tagName != "g":
                        continue
                        
                p1 = None
                p2 = None
                c1 = None
                for c in g.childNodes:
                        if c.nodeType != c.ELEMENT_NODE:
                                continue
                        
                        if c.tagName == "circle":
                                c1 = c
                        elif c.tagName == "path":
                                if not p1:
                                        p1 = c
                                else:
                                        p2 = c
                                        
                if not c1:
                        continue
                        
                if not p1:
                        q = g.nextSibling
                        while q:
                                if q.nodeType == c.ELEMENT_NODE:
                                        if q.tagName == "path":
                                                if not p1:
                                                        p1 = q
                                                else:
                                                        p2 = q
                                                        break
                                        elif q.tagName == "g":
                                                break
                                q = q.nextSibling
                                
                if not p1:
                        continue
                if not p2:
                        continue
                        
                id = g.getAttribute("id")
                if id:
                        if id.find("xpin") >= 0:
                                g.setAttribute("id", id.replace("xpin", "pin"))
                        elif id.find("pinx") >= 0:
                                g.setAttribute("id", id.replace("pinx", "pin"))

                print g.getAttribute("id")
                
                p1.parentNode.removeChild(p1)
                p2.parentNode.removeChild(p2)
                c1.parentNode.removeChild(c1)
                
                np1 = dom.createElement("path")
                g.appendChild(np1)
                np1.setAttribute("fill", p1.getAttribute("fill"))
                
                r = float(c1.getAttribute("r"))
                cx = float(c1.getAttribute("cx"))
                cy = float(c1.getAttribute("cy"))
                
                np1.setAttribute("d", ''.join(["M", `cx - r - r`, ",", `cy`, " a",`r * 2`, ",", `r * 2`, " ", `0`, " ", `0`, " ", `1`, " ", `r * 4`, ",", `0`]))
                
                np2 = dom.createElement("path")
                g.appendChild(np2)
                np2.setAttribute("fill", p2.getAttribute("fill"))

                np2.setAttribute("d", ''.join(["M", `cx + r + r`, ",", `cy`, " a",`r * 2`, ",", `r * 2`, " ", `0`, " ", `1`, " ", `1`, " ", `-r * 4`, ",", `0`]))

                g.appendChild(c1)
                p1.unlink()
                p2.unlink()
                       
        outfile = open(output, 'wb')
        s = dom.toxml("UTF-8")
        s = re.sub('\s*\n\s*\n', '', s)   # ghosts of removed (and unlinked) nodes seem to generate newlines, so tighten up the xml
        outfile.write(s)
        outfile.flush()
        outfile.close()                        
        

if __name__ == "__main__":
        main()



