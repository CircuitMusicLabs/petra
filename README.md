# Development discontinued - package maintained by Cycling '74
I have officially discontinued further development and maintenance for the Petra package. For this reason, Cycling '74 has taken over maintenance of the repository. Please refer to the official fork at [https://github.com/Cycling74/petra](https://github.com/Cycling74/petra).

**Matthias W. Müller - [@verenasounds](https://twitter.com/verenasounds) - [www.circuitmusiclabs.com](http://www.circuitmusiclabs.com/)**

petra is now available in the official Max Package Manager. Just open Max, go to the Package Manager and search for the petra package to download and install.

**Note:**
This repository might be in active development at any time, so any of the existing branches might be unstable or contain incomplete code. If you're interested in downloading stable code only, please find the latest release [here](https://github.com/CircuitMusicLabs/petra/releases).

## Package Description
### External Objects for Max
petra is a is a collection of external audio objects neatly packed into a Package for Max by [Cycling '74](https://cycling74.com/). It is used for polyphonic granulation of pre-recorded sounds. The package is loosely based on the principle of asynchronous granular synthesis (outlined in Curtis Roads' book [Microsound](https://mitpress.mit.edu/books/microsound)). The objects are made for sample precision granulation of both single- and dual-channel audio files.

In addition, petra contains an audio object for live input granulation. It makes use of a circular buffer and an adjustable, and optionally randomised, delay control over the duration of the entire buffer.

All objects share the following controls, which consist of an upper and lower range, from within which a random value is generated for each grain:

* start position within the selected sample buffer~ object
* grain length
* grain pitch
* panorama position per grain
* gain per grain

### Max for Live Device
As of v1.1.0, the petra package will contain a Max for Live Device, which showcases the potential of the external objects:

![m4l device](http://circuitmusiclabs.com/wp-content/uploads/petra-m4l-device.png)

The device uses the `cm.indexcloud~`external object and merely serves as an example of what can be done with the objects contained in the petra package. The device can be found in the [extras](https://github.com/CircuitMusicLabs/petra/blob/master/extras) folder.

#### How to use the Max for Live Device
Drop the Max for Live Device onto a new MIDI Track in Ableton Live and you are ready to go.

Descriptions for all parameters can be viewed in the Ableton Live Info View in the bottom left corner of the Live window. Just hover with the mouse over a parameter and the respective description is displayed.

## System Requirements for Compiled Externals
* Mac OS 10.9.5 or above or Microsoft Windows 7 or later
* Max 6.1.8 or above (compatible with Max 7 and Max 8)
* Max 32 bit and 64 bit

## Installing the Source Code
### Prerequisites
* [Git](http://git-scm.com/)
* XCode (Mac OS)
* Visual Studio (Windows)

### Cloning the Repository
Clone the petra repository via the Terminal using the following commands:

	cd ~/yourdirectory
	git clone https://github.com/CircuitMusicLabs/petra

After you cloned the repository, you have to pull the contents of the Max SDK submodule with the following commands:

	cd petra
	git submodule update --init --recursive

### Compiling the External Objects (Mac OS)
Go to ~/yourdirectory/petra/source, open the included Xcode projects and compile the externals with via Product > Build (Command-B).

### Compiling the External Objects (Windows)
Currently, the repository does not contain preconfigured project files for Visual Studio on Windows. Apparently, managing the locations of the Max SDK include-files is not as straight forward.

The easiest way to compile the petra external objects on Windows is the following (cm.buffercloud~ object serving as an example):

1. Go to \~/yourdirectory/petra/max-sdk/source/audio
2. Duplicate any one of the project folders (e.g. simplemsp~) and rename this folder into cm.buffercloud~ (do not move)
3. Enter the cm.buffercloud~ folder and rename the \~.vcxproj file and the \~.c source file into cm.buffercloud~
4. Open the \~.vcxproj file with Visual Studio
5. Replace the contents of the \~.c source file with the contents of the cm.buffercloud\~.c source file contained in \~/yourdirectory/petra/source/cm.buffercloud~
6. Build both Release|Win32 and Release|x64 build configurations

After building, the compiled externals can be found in ~/yourdirectory/petra/max-sdk/externals

## Manual Installation
The latest stable release can be installed with the Max Package Manager. However, if you wish to experiment with the source code, you can also install manually.

#### Installing for Max 6.1 - Mac OS X and Windows
Copy the entire petra repository either into the "packages" folder in your Max installation or into "Max/Packages" in your ~/Documents folder.

#### Installing for Max 7 - Mac OS X and Windows
Copy the entire petra repository into “Max 7/Packages" in your ~/Documents folder.

#### Installing for Max 8 - Mac OS X and Windows
Copy the entire petra repository into “Max 8/Packages" in your ~/Documents folder.

## How to use petra
After installing, open the [petra_overview.maxpat](https://github.com/CircuitMusicLabs/petra/blob/master/extras/petra_overview.maxpat) file. It contains the entire documentation and links to all object helpfiles.
