"""
Implement the Schwartizan Transform method of sorting
a list by an arbitrary metric (see the Python FAQ section
4.51).
--------------------------------------------------------------------

This program is licensed under the GNU General Public License (GPL).
See http://www.fsf.org for details of the license.

Andrew Sterian
Padnos School of Engineering
Grand Valley State University
<steriana@claymore.engineer.gvsu.edu>
<http://claymore.engineer.gvsu.edu/~steriana>
"""

def stripit(pair):
  return pair[1]

def schwartz(List, Metric):
  def pairing(element, M = Metric):
    return (M(element), element)

  paired = map(pairing, List)
  paired.sort()
  return map(stripit, paired)

def stripit2(pair):
  return pair[0]

def schwartz2(List, Metric):
  "Returns sorted list and also corresponding metrics"

  def pairing(element, M = Metric):
    return (M(element), element)

  paired = map(pairing, List)
  paired.sort()
  theList = map(stripit, paired)
  theMetrics = map(stripit2, paired)
  return (theList, theMetrics)
