
import sys, os, os.path, re, optparse
    
def usage():
        print """
usage:
    checkascii.py -f [folder] 
    recursively checks that all filenames in folder are ascii
"""
    
def main():
    parser = optparse.OptionParser()
    parser.add_option('-f', '--folder', dest="folder" )
    (options, args) = parser.parse_args()
        
    if not options.folder:
        usage()
        parser.error("folder argument not given")
        return       
            
    for root, dirs, files in os.walk(options.folder, topdown=False):
        for filename in files:
            remainder = re.sub('[ -~]', '', filename)
            if len(remainder) > 0:
                print "not ascii", os.path.join(root, filename)
            
                

if __name__ == "__main__":
        main()



