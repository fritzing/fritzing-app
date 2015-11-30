import os, os.path

inputDir = "C:\\Users\\joev\\Documents\\AAA\\Fritzing\\scripting begin\\input"
outputDir = "C:\\Users\\joev\\Documents\\AAA\\Fritzing\\scripting begin\\output\\"

def remove_namespace(doc, namespace):


    ns = u'{%s}' % namespace
    nsl = len(ns)
    for tree in doc.getiterator():
        if tree.tag.startswith(ns):
            tree.tag = tree.tag[nsl:]


def myFilter(svgFilename, filename):

	#print "im myfilter",svgFilename
	import xml.etree.ElementTree as ET  #import ElementTree
	parser = ET.XMLParser(encoding="utf-8")
	tree = ET.parse(svgFilename, parser)
	remove_namespace(tree, u'http://www.w3.org/2000/svg') #call remove_namespace for cleaning the element-tags
	svgRoot = tree.getroot()
	# print "tree", tree

	parent_map = dict((c, p) for p in tree.getiterator() for c in p) #make a dictonary with a map of the hole file 
	#it has the childs and the its parents stored

	coppers = set() #make a set (set is like a list but without doubled same entries)
	for child in parent_map.values():
		removed = False
		# print "child", child
		id = child.attrib.get("id")
		if id and id.startswith('copper'): #get the child ids to decide if we need to clean or not
			coppers.add(child)
			
			
		paths = list() 
		
		for copper in coppers:
			paths.extend(copper.findall("path")) #writes the list where are path-elemnts are in that are in a parent called "copper"
			
			#print "paths", paths
			for path in paths:
				#print "path", path
				#print "parent_map", parent_map
				id = path.attrib.get("id")
				
				if id is None:
					try:
						#print "id", id
						parent_map[path].remove(path) #remove the child from his parent
						removed = True

					except:
						 continue#print "fuck", path, path.tag, parent_map.get(path)
				
				else:
					#print "id", id
					removed = False

			if removed: #write the new file in a new output directory
				svgRoot.set("xmlns:svg", "http://www.w3.org/2000/svg")
				svgRoot.set("xmlns", "http://www.w3.org/2000/svg")
				#print "root", root, "tree",tree, "filename", svgFilename
				outFilename = outputDir + filename
				#print "outputdir", outputDir, "outFilename", outFilename
				
				tree.write(outFilename)
				with open (outFilename, "r+") as f:
					old = f.read()
					f.seek(0)
					f.write('<?xml version="1.0" encoding="UTF-8" standalone="no"?>\n' + '<!-- Created with Fritzing (http://www.fritzing.org/) -->' + '\n' + old)
					f.close()        
				print "finised:", filename
				

def main(): 
	for root, dirs, files in os.walk(inputDir, topdown=True): #go to the dicrectory and get all files in a list called "files"
		for filename in files:
			if filename.endswith(".svg"):  #if it is an svg -> go on
				#print "im main"
				svgFilename = os.path.join(root, filename)
				#print root, filename
				myFilter(svgFilename, filename) #go to function myFilter
				continue
				



if __name__ == "__main__":
        main()

