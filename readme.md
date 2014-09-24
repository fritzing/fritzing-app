# Fritzing

The Fritzing application is an Electronic Design Automation software with a low entry barrier, suited for the needs of makers and hobbyists. It offers a unique real-life "breadboard" view, and a parts library with many commonly used high-level components. Fritzing makes it very easy to communicate about circuits, as well as to turn them into PCB layouts ready for production. It is particularly popular among Arduino and Raspberry Pi users, and is widely used in education and creative tinkering.

* For more information on Fritzing and its related activities, visit [http://fritzing.org](http://fritzing.org). There you can also [download](http://fritzing.org/download) the latest releases for all platforms and get help on getting started.

* To report a problem or suggest improvements, use the [issue tracker](https://github.com/fritzing/fritzing-app/issues) or the [user forums](http://fritzing.org/forum)

* If you would like to help with the development, please take a look at the [developer instructions](https://github.com/fritzing/fritzing-app/wiki). This includes information about how to compile and run the Fritzing app in a few steps.


# Folder Structure

* **bins** - Part bins (.fzb, aka part libraries) are collections of parts, most importantly the "core" parts bin, and vendor-specific collections.

* **help** - End-user documentation included with the app. This should really be moved back to the website.

* **parts** - All the part definitions, including meta data (.fzp) and graphics (.svg), as well as some utility tools. They are kept in a separate repository at [https://github.com/fritzing/fritzing-parts](http://github.com/fritzing/fritzing-parts) and only linked from here.

* **pri** - Submodule definitions for Qt

* **resources** - Binaries and definitions that are supposed to not be touched by users, such as fonts, images, special parts, etc.

* **sketches** - Example circuits/sketches shipped with the application

* **src** - Application logic!

* **tools** - Utility tools for making releases, converting parts, etc.

* **translations** - Language translations


# Credits

The Fritzing app is maintained by the Friends-of-Fritzing e.V., a non-profit foundation based in Berlin, Germany. The project has grown out of a state-funded research project at the [Interaction Design Lab](http://idl.fh-potsdam.de) at [Potsdam University of Applied Sciences](http://fh-potsdam.de).

The core team consists of Prof. Reto Wettach, André Knörig, Jonathan Cohen, and Stefan Hermann. Many [fantastic people](http://fritzing.org/about/people/) have contributed to it over the years.

The Fritzing app is written on top of the [Qt cross-platform framework](http://qt-project.org).


# Licensing

The source code of Fritzing is licensed under GNU GPL v3, the documentation and part designs under Creative Commons Attribution-ShareALike 3.0 Unported. The full text of these licenses are shipped with this download.

This means that you can create your own variation of Fritzing, as long as you credit us and also publish it under GPL. Similarly, you may re-publish our documentation, as long as you credit us, and publish it under the same 
license. You may publish circuits and diagrams that you create with Fritzing and that use our graphics, again as long as you credit us, and 
publish your works under the same license.  A credit can be as simple as "this image was created with Fritzing."

Look up [our FAQs](http://fritzing.org/faq/) for more details on licensing.
