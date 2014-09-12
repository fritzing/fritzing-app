*IMPORTENT: please make sure cloning recursive to get all submodules and add [boost 1.55 library](http://www.boost.org/users/history/version_1_55_0.html) to fritzing-app/src/lib/ *

  ```git clone --recursive git://github.com/fritzing/fritzing-app/```

*


# About fritzing #

**[fritzing](http://fritzing.org)** is an open-source initiative to support designers and artists to take the step from physical prototyping to actual product. We are creating this software in the spirit of Processing and Arduino, that allows the designer / artist / researcher / hobbyist to document their Arduino-based prototype and create a PCB layout for manufacturing. The complimenting website helps to share and discuss drafts and experiences as well as to reduce manufacturing costs.

**[fritzing-app](http://fritzing.org/download/)** is essentially an Electronic Design Automation software with a low entry barrier, suited for the needs of designers and artists. It uses the metaphor of the breadboard, so that it is easy to transfer your hardware sketch to the software. From there it is possible to create PCB layouts for turning it into a robust PCB yourself or by help of a manufacturer.

**[fritzing-projects](http://fritzing.org/projects/)** is a user-based library of already crated opensource hardware. it is an inspiration and a learning platform for beginners as well as engineers.

**[fritzing-fab](http://fab.fritzing.org)** is where fritzing projects begin to life. With the fab-service you are able to oder your hardware directly from the fritzing fzz. 

**[fritzing-parts](http://github.com/fritzing/fritzing-parts/)** is the git repository for all the parts in fritzing. you can easily add your own created part to the repository and the next fritzing version will have it in the library. 


*More information may be found on the project website at [http://fritzing.org](http://fritzing.org)*

Downloads are available at [http://fritzing.org/download](http://fritzing.org/download)

# Folderstructure

### bins
fritzing-part-bin files (fzb) integrated in the Parts-palette

### help
helping file for the parts-editor

### parts
the fritzing parts folders. all the svg- and fzp-files for the fritzing-parts and python-scripts make the part business easier. it is based on a remotebranch and can be easily updated. the parts repository:
[https://github.com/fritzing/fritzing-parts](http://github.com/fritzing/fritzing-parts)


### pdb
the fritzing data-base referenz for all the core-parts included in parts.db. the folder is empty and stays for the database rewrite.

### pri

### recources 
recources for the fritzing application. including main-bins, fonts, images, parts, obsolete, styles, templates etc.

### sketches
including the example-sketches which gets shipped with the application.

### src
the programmcode for the fritzing-application

### tools
tools for fritzing. including brd2svg, stable-release-maker and other tools

### translations ###
the languagefiles for fritzing.
