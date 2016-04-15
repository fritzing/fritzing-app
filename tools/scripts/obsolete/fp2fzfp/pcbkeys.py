#defines attribute constants for gEDA PCB parameters
#note - some tags accept variable numbers of arguments

tagMap = {}
tagMap["Element"] = [["SFlags","Desc","Name","Value","MX","MY","TX","TY",\
            "TDir","TScale","TSFlags"],\
           ["NFlags","Desc","Name","Value","TX","TY","TDir","TScale","TNFlags"],\
           ["NFlags","Desc","Name","TX","TY","TDir","TScale","TNFlags"],\
           ["Desc","Name","TX","TY","TDir","TScale","TNFlags"]]
tagMap["Pin"] = [["rX","rY","Thickness","Clearance","Mask","Drill","Name",\
        "Number","SFlags"],\
       ["aX","aY","Thickness","Drill","Name","Number","NFlags"],\
       ["aX","aY","Thickness","Drill","Name","NFlags"],\
       ["aX","aY","Thickness","Name","NFlags"]]
tagMap["Pad"] = [["rX1","rY1","rX2","rY2","Thickness","Clearance","Mask",\
        "Name","Number","SFlags"],\
       ["rX1","rY1","rX2","rY2","Thickness","Name","Number","NFlags"],\
       ["rX1","rY1","rX2","rY2","Thickness","Name","NFlags"]]
tagMap["ElementLine"] = [["X1","Y1","X2","Y2","Thickness"]]
tagMap["ElementArc"] = [["X","Y","Width","Height","StartAngle","DeltaAngle",\
        "Thickness"]]
tagMap["Attribute"] = [["Name","Value"]] 
tagMap["Mark"] = [["Y","X"]]

def getNextAttr(attributes):
    """
    searches through a string to find the next space separated attribute,
    while being careful not to mess up double-quoted strings.
    returns a 2-tuple with the attribute followed by the remaining string
    """
    attributes = attributes.strip()
    if(attributes[0] == '"'):
        attributes = attributes[1:]
        attr, tmp, trimmed = attributes.partition('"')
    else:
        tmp = attributes.split(None, 1)
        if(len(tmp)<2):
            return tmp[0], ""
        attr = tmp[0]
        trimmed = tmp[1]
    return attr, trimmed

def makeDict(tag, attributes):
    """
    splits a list of attributes given a tag and returns a dict with 
    attributed names as keys.
    If the tag is not in the lookup table, an empty dict is returned.
    """
    if(not(tagMap.has_key(tag))):
        return {}
        
    #attrList = attributes.split()
    
    attrList = []
    while(attributes != ""):
        attr, attributes = getNextAttr(attributes)
        attrList.append(attr)
    
    tagList = None
    for selection in tagMap[tag]:
        if len(selection) == len(attrList):
            tagList = selection
    
    #no possible mapping found - should throw an exeption actually
    if(not(tagList)):
        return {}
    
    #attrListCleaned = []
    
    #strip quotes
    #for attr in attrList:
    #    if attr[0] == '"':
    #        tmp1, tmp2, attr = attr.partition('"')
    #        attr, tmp1, tmp2 = attr.rpartition('"')
    #    attrListCleaned.append(attr)
    
    #merge the two into a dictionary
    rtn = {}
    for i in xrange(len(tagList)):
        rtn[tagList[i]] = attrList[i]
    
    return rtn
