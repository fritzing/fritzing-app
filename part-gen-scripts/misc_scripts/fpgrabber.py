# usage:
#	fpgrabber.py -u [url to crawl] -d [output directory]
#	 recursively crawl the url and download fp files into the output directory.

# code borrowed from http://theanti9.wordpress.com/2009/02/14/python-web-crawler-in-less-than-50-lines/

import getopt, sys, os, os.path, re, urllib2, urlparse
    
def usage():
    print """
usage:
    fpgrabber.py -u [url to crawl] -d [output directory]
    recursively crawl the url and download fp files into the output directory.
    """
 
linkregex = re.compile('<a\s*href=[\"\'](.[^\"\']+)[\"\']')
redirectex = re.compile('content=[\"\']\d+;\s*URL=(.[^\"\']+)[\"\']')
tocrawl = []
crawled = set([])
outputdir = None
inputurl = None
 
def main():
    global inputurl, outputdir
    
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hu:d:", ["help", "url", "directory"])
    except getopt.GetoptError, err:
        # print help information and exit:
        print str(err) # will print something like "option -a not recognized"
        usage()
        sys.exit(2)
    
    for o, a in opts:
        #print o
        #print a
        if o in ("-u", "--url"):
            inputurl = a
        elif o in ("-d", "--directory"):
            outputdir = a
        elif o in ("-h", "--help"):
            usage()
            sys.exit(2)
        else:
            assert False, "unhandled option"
    
    if(not(inputurl)):
        usage()
        sys.exit(2)

    if(not(outputdir)):
        usage()
        sys.exit(2)

    if not outputdir.endswith(':') and not os.path.exists(outputdir):
        os.mkdir(outputdir)
        
    if not inputurl.endswith("/"):
        #footprint site seems to require this
        inputurl = inputurl + "/"
       
    tocrawl.append(inputurl)
    crawl()
    
def savefp(url):
    print "   saving " + url
    filehandle = None
    try:
        filehandle = urllib2.urlopen(url)
    except Exception as inst:
        print inst
        pass
        
    if (not filehandle):
        print "unable to open 1 " + url
        return
    
    p = url.split('/')
    fn = p[len(p) - 1]
    fn = fn[:-3]
    fn = urllib2.unquote(fn)
    f = open(os.path.join(outputdir, fn),'w')
    if (not f):
        print "unable to save " + fn
        filehandle.close()
        return
        
    for lines in filehandle.readlines():
        f.write(lines)

    f.close()
    filehandle.close()       
    return

def crawl():
        
    while len(tocrawl) > 0:
        crawling = tocrawl.pop()
        print "crawling " + crawling
          
        try:
            response = urllib2.urlopen(crawling)
        except:
            print "failed to open " + crawling
            return
            
        #print "code " + str(response.getcode())
        #url = urlparse.urlparse(response.geturl())
        if crawling != response.geturl():
            #this never gets triggered by the footprint site, geturl doesn't seem to return redirects
            print "changed " + crawling + " " + response.geturl()
            
        #info = response.info()
        #print info
        
        msg = response.read()
        response.close()
        links = linkregex.findall(msg)
        if (len(links) == 0):
            if (msg.find("<meta") >= 0):
                links = redirectex.findall(msg)
                #print "     redirect "  + msg + " " + str(len(links))
            
        crawled.add(crawling)
        for link in (links.pop(0) for _ in xrange(len(links))):    
            newlink = urlparse.urljoin(crawling, urllib2.quote(link, ":/?%+"))
            #print "        link " + newlink + " " + link
            if (newlink.endswith("fp?dl")):
                savefp(newlink)
                continue
                
            if (newlink.find(inputurl) >= 0) and (newlink not in crawled):   
                tocrawl.append(newlink)
   
if __name__ == "__main__":
    main()



