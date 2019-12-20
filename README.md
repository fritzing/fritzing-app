# Fritzing

|Branch|Badge|
|------|-----|
|master|[![Build Status](https://travis-ci.org/fritzing/fritzing-app.svg?branch=master)](https://travis-ci.org/fritzing/fritzing-app)|
|develop|[![Build Status](https://travis-ci.org/fritzing/fritzing-app.svg?branch=develop)](https://travis-ci.org/fritzing/fritzing-app)|

The Fritzing application is an Electronic Design Automation software with a low entry barrier, suited for the needs of makers and hobbyists. It offers a unique real-life "breadboard" view, and a parts library with many commonly used high-level components. Fritzing makes it very easy to communicate about circuits, as well as to turn them into PCB layouts ready for production. It is particularly popular among Arduino and Raspberry Pi users, and is widely used in education and creative tinkering.

* For more information on Fritzing and its related activities, visit [http://fritzing.org](http://fritzing.org). There you can also [download](http://fritzing.org/download) the latest releases for all platforms and get help on getting started.

* To report a problem or suggest improvements, use the [issue tracker](https://github.com/fritzing/fritzing-app/issues) or the [user forum](http://forum.fritzing.org).
Please provide steps, what operating system you are on, including the version. Add screenshots or copies of error messages, describe what behavior you saw and what you expected.

* If you would like to help with the development, please take a look at those labels:
  * [![label: easy start][~easy start]](https://github.com/fritzing/fritzing-app/labels/easy%20start)
  * [![label: challenging start][~challenging start]](https://github.com/fritzing/fritzing-app/labels/challenging%20start)

Some of those don't need C++ skills, like reproducing an issue on a certain platform, or verifying translations of languages we don't speak. If there is something for you, you might want to check the [developer instructions](https://github.com/fritzing/fritzing-app/wiki) next. This includes information about how to compile and run the Fritzing app.

## Project Structure

* **help** - End-user documentation included with the app. 

* **parts** - All the part definitions, including meta data (.fzp) and graphics (.svg), as well as some utility tools. They are kept in a separate repository at [https://github.com/fritzing/fritzing-parts](http://github.com/fritzing/fritzing-parts) and only linked from here.

* **pri** - Submodule definitions for Qt

* **resources** - Binaries and definitions that are supposed not to be touched by users, such as fonts, images, special parts, etc.

* **sketches** - Example circuits/sketches shipped with the application

* **src** - Application logic!

* **tools** - Utility tools for making releases, converting parts, etc.

* **translations** - Language translations

## Credits

The Fritzing app was maintained by the Friends-of-Fritzing e.V., a non-profit foundation based in Berlin, Germany. The project has grown out of a state-funded research project at the [Interaction Design Lab](http://idl.fh-potsdam.de) at [Potsdam University of Applied Sciences](http://fh-potsdam.de). 

The founding team consists of Prof. Reto Wettach, André Knörig, Jonathan Cohen, and Stefan Hermann. Many [fantastic people](http://fritzing.org/about/people/) have contributed to it over the years.

Since 2019, the project is maintained by Kjell Morgenstern, with great support from Peter Van Epp, André Knörig, and AISLER.

The Fritzing app is written on top of the [Qt cross-platform framework](http://qt-project.org).

## Licensing

The source code of Fritzing is under GNU GPL v3, the documentation and part designs under Creative Commons Attribution-ShareALike 3.0 Unported. The full texts of these licenses are shipped with this download.

This means that you can create your own variation of Fritzing, as long as you credit us and also publish it under GPL. Similarly, you may re-publish our documentation, as long as you credit us, and publish it under the same
license. You may publish circuits and diagrams that you create with Fritzing and that use our graphics, again as long as you credit us, and
publish your works under the same license.  A credit can be as simple as "this image was created with Fritzing."

Lookup [our FAQs](http://fritzing.org/faq/) for more details on licensing.

[~help wanted]: https://img.shields.io/badge/-help%20wanted-%23159818
[~easy start]: https://img.shields.io/badge/-easy%20start-%2333AAFF
[~challenging start]: https://img.shields.io/badge/-challenging%20start-%235500EE
