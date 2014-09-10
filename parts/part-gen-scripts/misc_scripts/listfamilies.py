
import sys, os, os.path, re, xml.dom.minidom, xml.dom,  optparse
    
def usage():
        print """
usage:
    listfamilies.py -d [fzp folder] {-p [prefix] }
    lists families and optionally provides a prefix
"""
    
        
           
def main():
    parser = optparse.OptionParser()
    parser.add_option('-d', '--directory', dest="dir" )
    parser.add_option('-p', '--prefix', dest="prefix" )
    (options, args) = parser.parse_args()
        
    if not options.dir:
        usage()
        parser.error("dir argument not given")
        return       
            
    names = []
    for root, dirs, files in os.walk(options.dir, topdown=False):
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
            properties = fzp.getElementsByTagName("property")
            for property in properties:
                if property.getAttribute("name") == 'family':
                    value = getText(property.childNodes)
                    if not value in names:
                        names.append(value)
                        
                    if options.prefix != None:
                        for node in property.childNodes:
                            if node.nodeType == node.TEXT_NODE:
                                if not (options.prefix.lower() in node.data.lower()):
                                    node.data = options.prefix + " " + node.data
                                    outfile = open(fzpFilename, 'wb')
                                    s = dom.toxml("UTF-8")
                                    outfile.write(s)
                                    outfile.close()                    

                                break
                    break
                    
    names.sort()
    for name in names:
        print name

                
def getText(nodelist):
    rc = []
    for node in nodelist:
        if node.nodeType == node.TEXT_NODE:
            rc.append(node.data)
    return ''.join(rc)                 

                    
            
if __name__ == "__main__":
        main()



