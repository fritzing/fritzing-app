#this object takes an fzfp (an xml-ized gEDA footprint) and makes
# it into a Fritzing footprint SVG.
import xml.dom.minidom

#TODO: rounded pads

class SVGGen(xml.dom.minidom.Element):
    def __init__(self, xmlFile, debug=False):
        self.debug = debug
        self._svg = xml.dom.minidom.Document
    
    def getSvgFile(self):
    # spits out a string with the generated SVG
        return self._svgFile
    
    def _drawNode(self, node):
    # renders a node to SVG
        self._debug("drawing node: ")
        tag = node.nodeName.lower()
        
        if tag == "pin":
            self._debug("pin")
            self._drawPin(node)
        elif tag == "pad:
            self._debug("pad")
            self._drawPad(node)
        elif tag == "elementline":
            self._debug("element line")
            self._drawElementLine(node)
        elif tag == "elementarc":
            self._debug("element arc")
            self._drawElementArc(node)
        elif tag == "mark":
            self._debug("mark")
            self._drawMark(node)
        else:
            #TODO: throw an exception
            self._debug("error - cannot render unrecognized tag")
        
    def _drawPin(self, node):
    # render a pin to SVG
        pass
        
    def _drawPad(self, node):
    # render a copper pad to SVG
        pass
        
    def _drawElementLine(self, node):
    # render a silkscreen line
        pass
        
    def _drawElementArc(self, node):
    # render a silkscreen circular arc
        pass
        
    def _drawMark(self, node):
    # render the center point mark
        pass
        
    def minMax(self, x, y, strokeWidth):
    """
    check to see if the coordinates are within the viewbox
    adjusts if necessary
    """
        pass
        
    def _debug(self, msg):
        if self.debug:
            print msg
