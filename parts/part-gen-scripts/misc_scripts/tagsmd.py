# usage:
#	tagsmd.py -d [fzp folder] 
#       make sure each fzp has a <tag>smd</tag> 
import getopt, sys, os, os.path, re, xml.dom.minidom, xml.dom
    
def usage():
        print """
usage:
    tagsmd.py -d [fzp folder] 
    add or update a package <property> element.
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
    typ = None
            
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
            
    if not(dir):
        usage()
        sys.exit(2)    
    
    for root, dirs, files in os.walk(dir, topdown=False):
        for filename in files:
            if not filename.endswith(".fzp"): 
                continue
            if not filename.startswith("SMD"): 
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
            tags = None
            tagsNodes = fzp.getElementsByTagName("tags")
            if (tagsNodes.length == 0):
                tags = dom.createElement("tags")
                fzp.insertBefore(tags, fzp.firstChild)
            elif (tagsNodes.length == 1):
                tags = tagsNodes.item(0)                
            else:
                print "multiple tags elements in", fzpFilename
                continue
             

            #print "got properties", properties
            
            tagNodes = tags.getElementsByTagName("tag")
            smd = None
            for tag in tagNodes:
                tag.normalize()  #make sure all the text is in one node
                t = None
                for child in tag.childNodes:
                    if child.nodeType == child.TEXT_NODE:
                        t = child
                        break     
                if t:
                    if t.nodeValue.lower() == "smd":
                        smd = t
                        break
 
            if smd == None:
                smd = dom.createElement("tag")
                tags.appendChild(smd)
                t = dom.createTextNode("SMD")
                smd.appendChild(t)
             
                print "adding smd tag to", fzpFilename
                outfile = open(fzpFilename, 'wb')
                s = dom.toxml("UTF-8")
                outfile.write(s)
                outfile.flush()
                outfile.close()                    
            
if __name__ == "__main__":
        main()



