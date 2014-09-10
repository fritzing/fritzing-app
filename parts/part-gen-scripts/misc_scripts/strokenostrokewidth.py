# usage:
#	    strokenostrokewidth.py -d [svg folder] -f yes/no
#       looks for stroke attribute with no stroke-width attribute; if -f is "yes" then set stroke-width to 1


import getopt, sys, os, os.path, re, xml.dom.minidom, xml.dom
    
def usage():
        print """
usage:
    strokenostrokewidth.py -d [svg folder] -f yes
    looks for stroke attribute with no stroke-width attribute; if -f is "yes" then set stroke-width to 1
"""
           
def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hf:d:", ["help", "fix", "directory"])
    except getopt.GetoptError, err:
        # print help information and exit:
        print str(err) # will print something like "option -a not recognized"
        usage()
        sys.exit(2)
        
    dir = None
    fix = None
            
    for o, a in opts:
        #print o
        #print a
        if o in ("-d", "--directory"):
            dir = a
        elif o in ("-h", "--help"):
            usage()
            sys.exit(2)
        elif o in ("-f", "--fix"):
            fix = a
        else:
            assert False, "unhandled option"
            
    if(not(dir)):
        usage()
        sys.exit(2)            
    
    for root, dirs, files in os.walk(dir, topdown=False):
        for filename in files:
            if not filename.endswith(".svg"): 
                continue
                
            svgFilename = os.path.join(root, filename)
            try:
                dom = xml.dom.minidom.parse(svgFilename)
            except xml.parsers.expat.ExpatError, err:
                print str(err), svgFilename
                continue
               
            changed = 0
            todo = [dom.documentElement]
            while len(todo) > 0:
                element = todo.pop(0)
                for node in element.childNodes:
                    if node.nodeType == node.ELEMENT_NODE:
                        todo.append(node)
                stroke = element.getAttribute("stroke")
                strokewidth = element.getAttribute("stroke-width")
                if len(stroke) == 0:
                    style = element.getAttribute("style")
                    if len(style) != 0:
                        style = style.replace(";", ":")
                        styles = style.split(":")
                        for index, name in enumerate(styles):
                            if name == "stroke":
                                stroke = styles[index + 1]
                            elif name == "stroke-width":
                                strokewidth = styles[index + 1]
                        
                if len(stroke) > 0  and stroke != "none":
                    if len(strokewidth) == 0:
                        print "no stroke width", svgFilename   # , 
                        if fix != "yes":
                            break
                         
                        # print "fixing", element.toxml("UTF-8")
                        element.setAttribute("stroke-width", "1")
                        changed = 1
                        
            if changed:
                outfile = open(svgFilename, 'wb')
                s = dom.toxml("UTF-8")
                outfile.write(s)
                outfile.flush()
                outfile.close()                        
               
            
if __name__ == "__main__":
        main()



