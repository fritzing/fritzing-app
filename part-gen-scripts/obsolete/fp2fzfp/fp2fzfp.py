#pcb object reader - loads gEDA PCB files into python tree structure
# caveats:
#   1. comments are escaped except for the case where the # characer appears 
#       in a quoted string... probably need to fix the regex 
#   2. currently this parser assumes proper line breaks - this may choke on 
#      some poorly formatted files - need to use a different tokenizer than string.splitlines()

import re
import pcbkeys
import sys

filedata = open(sys.argv[1]).read()

#strip blank lines
#stripped = re.sub("\n\s*\n*", "\n", filedata)

#strip comments
stripped = re.sub('#.*\n', "", filedata)

#split lines into list
stripped = stripped.splitlines()
cleaned = []

# Get rid of empty lines
for line in stripped:
    # Strip whitespace, should leave nothing if empty line was just "\n"
    if not line.strip():
        continue
    # We got something, save it
    else:
        cleaned.append(line)

def findBrackets(obj):
    cIndex = obj.find('[')
    mIndex = obj.find('(')
    
    #TODO use an exception
    #both are -1 - parse error
    if((cIndex == -1) and (mIndex == -1)):
        print "PCB Parse Error on string: " + obj
        return None, None
        
    if(((cIndex < mIndex) and (cIndex != -1)) or (mIndex == -1)):
        return "[", "]"
    else:
        return "(", ")"
        
def splitObj(obj):
    lBracket, rBracket = findBrackets(obj)
    if not(lBracket):
            print "parse error..."
    #print "bracket: " + str(lBracket)
    tag, sep, params = obj.partition(lBracket)
    tag = tag.strip()
    params = params.rstrip(rBracket)
    return tag, sep, params
     
def toXML(tag, attrDict, sep):
    rtn = "<" + tag
    if(sep == "["):
        rtn = rtn + ' units="cmil"'
    else:
        rtn = rtn + ' units="mils"'
    for key in attrDict.keys():
        rtn = rtn + " " + key + '="' + attrDict[key] + '"'
    rtn = rtn + ">"
    return rtn 
    
def tokenize(instr, index=0, depth=0):
    pcbObj = instr[index].strip()
    sp = depth*"  "
    
    if pcbObj == "(":
        #print "decending..."
        index = index + 1
        parentTag, sep, tmp = splitObj(instr[index-2].strip())
        while(instr[index].strip() != ")"):
            #print "list processing line " + str(index)
            index = tokenize(instr, index, depth+1)
        #print "ascending..."
        outfile.write(sp + "</" + parentTag + ">\n")
    else:
        tag, sep, params = splitObj(pcbObj)
        paramdict = pcbkeys.makeDict(tag, params)
        outfile.write(sp + toXML(tag, paramdict, sep) + "\n")
        if (instr[index+1].strip() != "("):
            outfile.write(sp + "</" + tag + ">\n")
    
    return index + 1
            


idx = 0
outfile = open(sys.argv[2],"w") 
while(idx < len(cleaned)-1):
    idx = tokenize(cleaned, index=idx)
    
outfile.close()
    
