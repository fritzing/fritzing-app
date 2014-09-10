
import getopt, sys, os, os.path, re, xml.dom.minidom, xml.dom
    
def usage():
        print """
usage:
    listModuleIDs.py -f { fz file }
    looks for files where copper0/copper1 are not parent/child or child/parent
"""
           
def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hf:", ["help", "file"])
    except getopt.GetoptError, err:
        # print help information and exit:
        print str(err) # will print something like "option -a not recognized"
        usage()
        return
        
    filename = None
            
    for o, a in opts:
        #print o
        #print a
        if o in ("-f", "--file"):
            filename = a
        elif o in ("-h", "--help"):
            usage()
            return
        else:
            assert False, "unhandled option"
            
    if filename == None:
        usage()
        return           
            
    try:
        dom = xml.dom.minidom.parse(filename)
    except xml.parsers.expat.ExpatError, err:
        print str(err), filename
        return
        
    moduleIDs = {}
                
    root = dom.documentElement
    instances = root.getElementsByTagName("instance")
    for instance in instances:
        moduleID = instance.getAttribute("moduleIdRef")
        path = instance.getAttribute("path")
        if moduleIDs.get(moduleID) == None:
            moduleIDs[moduleID] = path
        
    for moduleID in moduleIDs.keys():
        print moduleID
        print "    ",moduleIDs[moduleID]


if __name__ == "__main__":
        main()



