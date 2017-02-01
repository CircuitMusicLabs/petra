# petra by Circuit Music Labs
**Matthias W. Müller - [@mwmueller](https://twitter.com/mwmueller) - [www.circuitmusiclabs.com](http://www.circuitmusiclabs.com/)**

[logo]: http://circuitmusiclabs.com/img/projects/petra/petra-logo.svg

A Max Package for experiments in granular synthesis. The petra package will replace the [cm.grainlabs~](https://github.com/CircuitMusicLabs/cm.grainlabs) object, which is no longer in development and likely to cause crashes with all current versions of Max.

petra is now open for beta-testing. Feel free to download and to report back any feedback you may have.

Once beta testing is done, I will try to submit the project as an official Max package for the new [Max Package Manager](https://cycling74.com/2015/12/14/introducing-the-max-package-manager/#.V6BINKJ1Z_B).

##System Requirements for Compiled Externals
* Mac OS 10.9.5 or above or Microsoft Windows 7 or later
* Max 6.1.8 or above (compatible with Max 7)
* Max 32 bit and 64 bit

##Installing the Source
###Cloning the Repository
If you have [Git](http://git-scm.com/) installed, you can install via the Terminal using the following commands:

	cd ~/yourdirectory
	git clone https://github.com/CircuitMusicLabs/petra

After you cloned the repository, you have to pull the contents of the Max SDK submodule with the following commands:

	cd petra
	git submodule update --init --recursive

###Compiling the External Objects (Mac OS)
Go to ~/yourdirectory/source, open the included Xcode projects and compile the externals with via Product > Build (Command-B).

###Compiling the External Objects (Windows)
Currently, the repository does not contain preconfigured project files for Visual Studio on Windows. Apparently, managing the locations of the Max SDK include-files is not as straight forward.

The easiest way to compile the petra external objects on Windows is the following (cm.buffercloud~ object serving as an example):

1. Go to ~/yourdirectory/petra/max-sdk/source/audio
2. Duplicate any one of the project folders (e.g. simplemsp~) and rename this folder into cm.buffercloud~ (do not move)
3. Enter the cm.buffercloud~ folder and rename the ~.vcxproj file and the ~.c source file into cm.buffercloud~
4. Open the ~.vcxproj file with Visual Studio
5. Replace the contents of the ~.c source file with the contents of the cm.buffercloud~ source file contained in ~/yourdirectory/petra/source/cm.buffercloud~
6. Build both Release|Win32 and Release|x64 build configurations

After building, the compiled externals can be found in ~/yourdirectory/petra/max-sdk/externals

##Installation of the Max Package
The petra repository is structured as a Max package, which contains the compiled external objects.

###Installing the Max Package (Max 6 - Mac OS X)
Copy the entire petra repository either into the "packages" folder in your Max installation or into "Max/Packages" in your ~/Documents folder.

###Installing the Max Package (Max 7 - Mac OS X)
Copy the entire petra repository into “Max 7/Packages" in your ~/Documents folder.

###Installing the Max Package (Max 7 - Windows)
Copy the entire petra repository into “Max 7/Packages" in your ~/Documents folder.

##How to use petra
After installing the Max package, open the [petra_overview.maxpat](https://github.com/CircuitMusicLabs/petra/blob/master/extras/petra_overview.maxpat) file. I contains the entire documentation and links to all object helpfiles.
