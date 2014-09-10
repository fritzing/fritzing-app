#!/usr/bin/python
# -*- coding: utf-8 -*-

import urllib2
import sys
import os
import traceback
import re
import getopt
import HTMLParser

def usage():
    print """
usage:
    875fzpzs.py -d <directory> 
    
    <directory> is a to contain downloaded fzpz files.  
    """

def main():
    
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hd:", ["help", "directory"])
    except getopt.GetoptError, err:
        # print help information and exit:
        print str(err) # will print something like "option -a not recognized"
        usage()
        return

    outputDir = None
    
    for o, a in opts:
        #print o
        #print a
        if o in ("-d", "--directory"):
            outputDir = a
        elif o in ("-h", "--help"):
            usage()
            return
        else:
            assert False, "unhandled option"
    
    if not(outputDir):
        print "missing -d {directory} parameter"
        usage()
        return
        
    if not os.path.exists(outputDir):
        print "output directory doesn't exist"
        usage()
        return
        
    mainUrl = "https://code.google.com/p/fritzing/issues/detail?id=875"
    increment = 0
    try:
        req = urllib2.Request(mainUrl)
        #print "req", req
        response = urllib2.urlopen(req)
        print "response", response
        html = response.read()
        print "got html"
        
        parser = HTMLParser.HTMLParser()
        
        pattern = r'<a\s*href="([^"]*)"[^<]*>download'
        regex = re.compile(pattern, re.IGNORECASE)
        
        pattern2 = r'name=([^\&]*)'
        regex2 = re.compile(pattern2, re.IGNORECASE)
        
        pattern3 = r'(\d*)[^\d]'
        regex3 = re.compile(pattern3, re.IGNORECASE)
        
        commentPhrase = 'href="/p/fritzing/issues/detail?id=875#c'
    
        for match in regex.finditer(html):
            url = "http:" + parser.unescape(match.group(1))
            try:
                
                prefix = ""
                ix = html.rfind(commentPhrase, 0, match.start())
                if (ix > 0):
                    match3 = re.search(regex3, html[ix + len(commentPhrase):])
                    try:
                        int(match3.group(1))
                        prefix = match3.group(1).zfill(4)
                    except:
                        pass
                
                if prefix == "":
                    prefix = "0000"
                    
                match2 = re.search(regex2, url)
                basename = match2.group(1)
                print "downloading", url, "to", basename
                f = urllib2.urlopen(url)
                path = os.path.join(outputDir, prefix + "_" + basename)
                with open(path, "wb") as local_file:
                    local_file.write(f.read())

            except:
                info = sys.exc_info()
                traceback.print_exception(info[0], info[1], info[2])
                
    except:
        info = sys.exc_info()
        traceback.print_exception(info[0], info[1], info[2])
        

if __name__ == '__main__':
    main()
