# usage:
#    props2csv.py -d <directory> 
#    
#    <directory> is a folder containing .fzp files.  
#    save a csv file of props, tags, etc. to <csv file path> 

import getopt, sys, os, re, csv, xml.dom.minidom, xml.dom
    
def usage():
    print """
usage:
    props2csv.py -d <directory> -c <csv file path>
    
    <directory> is a folder containing .fzp files.  
    save a csv file of props, tags, etc. to <csv file path> 
    """
        
       
def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hd:c:", ["help", "directory", "csv"])
    except getopt.GetoptError, err:
        # print help information and exit:
        print str(err) # will print something like "option -a not recognized"
        usage()
        sys.exit(2)
    outputDir = None
    csvPath = None
    
    for o, a in opts:
        #print o
        #print a
        if o in ("-d", "--directory"):
            outputDir = a
        elif o in ("-c", "--csv"):
            csvPath = a
        elif o in ("-h", "--help"):
            usage()
            sys.exit(2)
        else:
            assert False, "unhandled option"
    
    if not outputDir:
        usage()
        sys.exit(2)
        
    if not csvPath:
        usage()
        sys.exit(2)
        
    writer = None
    file = None
    try:
        file = open(csvPath, 'wb')
        writer = csv.writer(file, delimiter=',')
        writer.writerow(["fzp","location","title","description","family","props","tags","taxonomy"] )
    except:
        print "unable to save to", csvPath
        sys.exit(2)
            
    names = []
    for filename in os.listdir(outputDir): 
        if (filename.endswith(".fzp")):  
            fzpFilename = os.path.join(outputDir, filename)
            try:
                dom = xml.dom.minidom.parse(fzpFilename)
            except xml.parsers.expat.ExpatError, err:
                print str(err), fzpFilename
                continue
             
            theLine = filename + ","
            
            fzp = dom.documentElement
            
            titleText = ""
            titles = fzp.getElementsByTagName("title")
            for title in titles:
                titleText = getText(title.childNodes)
                break       # assume only one title
                
            taxonomyText = ""
            taxonomies = fzp.getElementsByTagName("taxonomy")
            for taxonomy in taxonomies:
                taxonomyText = getText(taxonomy.childNodes)
                break       # assume only one title
                
            location = "core"
            if "contrib" in fzpFilename:
                location = "contrib"
            elif "resource" in fzpFilename:
                location = "resources"
            
            descriptionText = ""
            descriptions = fzp.getElementsByTagName("description")
            for description in descriptions:
                descriptionText = getText(description.childNodes)
                break       # assume only one description
            
            tagsText = ""
            tags = fzp.getElementsByTagName("tag")
            for tag in tags:
                tagsText += getText(tag.childNodes) + "\n"

            familyText = ""
            propertiesText = ""
            properties = fzp.getElementsByTagName("property")
            for property in properties:
                name = property.getAttribute('name')
                value = getText(property.childNodes)
                propertiesText += name + ":" + value + "\n"
                if name == "family":
                    familyText = value
            
            writer.writerow([filename, location, titleText.encode("utf-8"), descriptionText.encode("utf-8"), familyText.encode("utf-8"), propertiesText.encode("utf-8"), tagsText.encode("utf-8"), taxonomyText.encode("utf-8")])
    
    if file:
        file.close()
        
            
def getText(nodelist):
    rc = []
    for node in nodelist:
        if node.nodeType == node.TEXT_NODE:
            rc.append(node.data)
    return ''.join(rc) 
    
if __name__ == "__main__":
    main()

