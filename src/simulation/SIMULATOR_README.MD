# Simulator Description #

This document describes the features of the implemented simulator in Fritzing. The purpose of the simulator is to teach electronics to beginners. Therefore, the current capabilities are very basics. In the future, we could extend this code to handle advanced analyses, but currently it is not a priority. If you want to help us, please, contact us to discuss collaborations (see the contribute section of this file). 

## Features ##

The current implementation performs a dc analysis (operating point). By default, there is no output of the voltages or currents in the circuit. To be able to measure them, you should use the multimeters of the core bin. There are five multimeters but currently only the dc multimeters work (voltmeter, ammeter and ohmmeter). 

### Currently supported devices ###

Currently, a very limited types of devices are supported:

* Resistors
* Capacitors
* Inductors
* Switches
* Potentiometers
* Diodes 
* LEDs
* Bipolar Junction Transistors (TODO: FET transistors)
* Multimeters

If you want to add support to more devices, see the contribute section.

### Error checking ###

We check that the devices that are being simulated are withing its specifications. A smoke symbol is added on top of a device if its working outside its specifications.

Currently, we have implemented these tests

* Three probes connected to a multimeter (Err message in the screen appears)
* Max current in diodes and LEDs (TODO: inductors)
* Max power in resistors
* Reverse voltage in tantalum and electrolytic capacitors
* Max voltage in capacitors 


### Input devices ###

The status of the switches and potentiometers can be change through a menu in the inspector pane. The switches change their image bases on its status (pressed/released) in the breadboard view and the schematics view. Currently, the potentiometers do not change its image.


### Output devices ###

The only output devices that we handle is the LED. LEDs change its color based on the current that flows through them. RGB LEDs are currently not supported.

## Install ##

Right now, this code is not integrated in the main code of Fritzing. If you want to try it, you need to download my simulator branch, which is hosted [here](https://github.com/failiz/fritzing-app). Please, note that you also need the parts repository with spice fields. Some of these fields are in the Fritzing-parts repository but others are still missing. It is recommended to use my parts repositorty, which can be found [here](https://github.com/failiz/fritzing-parts). 

The simulator uses Ngspice as a spice simulator. It is needed to download the shared library of Ngspice (.dll or .so) from [here](http://ngspice.sourceforge.net/shared.html) and it needs to be in the library path of your system.

After having both repositories downloaded and the Ngspice added to your library path, build the program following the steps that can be found in the Fritzing wiki: [Building Fritzing](https://github.com/fritzing/fritzing-app/wiki/1.-Building-Fritzing).


## Developer documentation ## 

The code is based on the Kicad code that interfaces with Ngspice and the Fritzing code that exports spice netlists. Basically, when the simulate function is called the following actions are performed:

* Creates an instance of the Ngspice simulator (if it was not created before)
* Gets the current spice netlist
* Loads the netlist in Ngspice
* Runs a operating point analysis in a background thread
* Remove all previous items placed by the simulator (smokes, messages in the multimeters)
* Grey out the parts that are not being simulated 
* Wait until the simulation has finished (timeout of 3s)
* Iterate for all parts being simulated to
    * Check if they work withing specifications, add smoke if needed
    * Update display messages in the multimeters
    * Update LEDs colors

## Contribute ##

There several things that you can contribute, and you do not need to be a programmer for some of them.

### Adding spice fields to components ###

The simulator uses spice fields that are stored in the part file (.fpz). You can help us to populate these fields in the components that do not have them yet. You should send us pull requests to the part repository. Please, be aware of copyrights when uploading spice fields! 

### Improving svgs of the switches ###

Currently, the svgs of the switches have two states (pressed or released). However, this distinction is subtle in some parts as the point of view is from above. If you can improve this visualization, send us a pull request.

### Code related ###

Pull request are welcome! 

Some features that could be added:

* Better handling of Ngspice errors
* Perform transitory analysis and plot the results in an oscilloscope. 
* Switches and potentiometers could be activated interactively

## Acknowledgments ##

We want to thank you to all the persons who contribute to (1) the [Ngspice](http://ngspice.sourceforge.net/index.html) simulator and (2) the [Kicad](https://kicad.org/) team who developed the Ngspice interface (We basically took their code, ported it from Wx to Qt and integrated it in Fritzing). Thanks a lot for four your effort! 

