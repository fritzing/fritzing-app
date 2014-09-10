# usage:
#	thtsms.py -d [fzp folder] -t [tht | smd]
#       add or update a package <property> element.

import getopt, sys, os, os.path, re, xml.dom.minidom, xml.dom
    
def usage():
        print """
usage:
    thtsmd.py -d [fzp folder] -t [tht | smd]
    add or update a package <property> element.
"""
           
def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hd:t:", ["help", "directory", "type"])
    except getopt.GetoptError, err:
        # print help information and exit:
        print str(err) # will print something like "option -a not recognized"
        usage()
        sys.exit(2)
        
    dir = None
    typ = None
            
    for o, a in opts:
        #print o
        #print a
        if o in ("-d", "--directory"):
            dir = a
        elif o in ("-t", "--type"):
            typ = a
        elif o in ("-h", "--help"):
            usage()
            sys.exit(2)
        else:
            assert False, "unhandled option"
            
    if not(dir):
        usage()
        sys.exit(2)    
        
    if not(typ):
        usage()
        sys.exit(2)       
    
    for root, dirs, files in os.walk(dir, topdown=False):
        for filename in files:
            if not filename.endswith(".fzp"): 
                continue
                
            fzpFilename = os.path.join(root, filename)
            infile = open(os.path.join(root, filename), "r")
            content = infile.read()
            infile.close()
            try:
                dom = xml.dom.minidom.parseString(content)
            except xml.parsers.expat.ExpatError, err:
                print str(err), fzpFilename
                continue
                
            #print "testing", fzpFilename
                
            fzp = dom.documentElement
            properties = None
            propertiesNodes = fzp.getElementsByTagName("properties")
            if (propertiesNodes.length == 0):
                properties = dom.createElement("properties")
                fzp.insertBefore(properties, fzp.firstChild)
            elif (propertiesNodes.length == 1):
                properties = propertiesNodes.item(0)                
            else:
                print "multiple properties elements in", fzpFilename
                continue
             

            #print "got properties", properties
            
            propertyNodes = properties.getElementsByTagName("property")
            package = None
            for property in propertyNodes:
                name = property.getAttribute("name")
                #print "got name", name
                if name == "package":
                    package = property
                    break
 
            #print "got package", package
 
            if package == None:
                package = dom.createElement("property")
                properties.appendChild(package)
                package.setAttribute("name", "package")
                t = dom.createTextNode(typ)
                package.appendChild(t)
            else:
                package.normalize()  #make sure all the text is in one node
                t = None
                for child in package.childNodes:
                    if child.nodeType == child.TEXT_NODE:
                        t = child
                        break                        
                
                if t == None:
                    t = dom.createTextNode(typ)
                    package.appendChild(t)
                else:
                    if ("SMD" in t.nodeValue):
                        continue
                    if ("THT" in t.nodeValue):
                        continue
                    
                    t1 = dom.createTextNode(" [" + typ + "]")
                    package.appendChild(t1)
                    package.normalize()
            
            print "adding package property " + typ + " to", fzpFilename
            outfile = open(fzpFilename, 'wb')
            s = dom.toxml("UTF-8")
            outfile.write(s)
            outfile.flush()
            outfile.close()                    
            
if __name__ == "__main__":
        main()



