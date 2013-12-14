Fritzing - from prototype to product
www.fritzing.org


ABOUT THIS RELEASE
This is a preview release of Fritzing. Try it out on your project 
and let us know what requirements you have. Contribute to the part 
library and if you're a developer, have a look at the code. We greatly 
appreciate every contribution. Please join the discussion in our forums.  

WARNING
This is an early release and the software is not yet stable. We welcome
your bug reports.  Also please be aware that file formats may change 
as we move closer to a final release, so there's no guarantee that your
sketch files will open in future releases.


ABOUT FRITZING
Fritzing is an open-source initiative to support designers and artists in
taking the step from physical prototyping to actual product. We are
creating this software in the spirit of Processing and Arduino. Fritzing's 
goal is to allow the designer / artist / researcher / hobbyist to document 
their breadboard-based prototype and create a PCB layout for 
manufacturing.
We hope our website will be a place for people to share and discuss 
their projects and experiences.

Fritzing is essentially Electronic Design Automation software suited to the 
needs of designers and artists. It uses the metaphor of the breadboard, 
so that it is easy to transfer a hardware sketch to the software by using a 
drag-and-drop-based GUI to copy your sketch. From there it is possible 
to create PCB layouts for turning your prototype into a robust PCB, 
either on your own, or with the help of a manufacturer.

To launch Fritzing: 
	on Mac, double-click the Fritzing application
	on Linux, double-click Fritzing, or try ./Fritzing in your shell window
	on Windows, double-click fritzing.exe


LICENSING
The source code of Fritzing is licensed under GNU GPL v3, the documentation 
and part designs under Creative Commons Attribution-ShareALike 3.0 Unported.
The full text of these licenses are shipped with this download.

This means that you can create your own variation of Fritzing, as long as you 
credit us and also publish it under GPL. Similarly, you may re-publish our 
documentation, as long as you credit us, and publish it under the same 
license. 
You may publish circuits and diagrams that you create with 
Fritzing and that use our graphics, again as long as you credit us, and 
publish your works under the same license.  A credit can be as simple as 
"this image was created with Fritzing."


BUILDING FRITZING
This section is mostly for Linux users. Fritzing is built using the Qt Framework, 
so build instructions follow the model for Qt applications.  The easiest approach 
is to download the QT SDK and use the QtCreator IDE.  You can also build natively 
using the following steps (qmake is a tool supplied by Qt):
	qmake
	make
	make install

Fritzing requires Qt 4.6 and up, and the Qt-sqlite and Qt-jpeg plugins.


(c) 2007-2013 Fachhochschule Potsdam
design.fh-potsdam.de