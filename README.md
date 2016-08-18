# petra by Circuit Music Labs
**Matthias W. Müller - [@mwmueller](https://twitter.com/mwmueller) - [www.circuitmusiclabs.com](http://www.circuitmusiclabs.com/)**

A Max Package for experiments in granular synthesis. The petra package will replace the [cm.grainlabs~](https://github.com/CircuitMusicLabs/cm.grainlabs) object, which is no longer in development and likely to cause crashes with all current versions of Max.

petra is still in active development, but I'm applying the last touches since all contained objects seem to be stable.

You're welcome to come back here to check for updates. I will try to submit the project as an official Max package for the new [Max Package Manager](https://cycling74.com/2015/12/14/introducing-the-max-package-manager/#.V6BINKJ1Z_B) as soon as all is done and tested.

###System Requirements for Compiled Externals
* Mac OS 10.9.5 or above
* Max 6.1.8 or above (compatible with Max 7)
* Max running in 64 bit mode

###Installing the Source (Mac OS X)
If you have [Git](http://git-scm.com/) installed, you can install via the Terminal using the following commands:

	cd ~/yourdirectory
	mkdir cm.grainlabs~
	cd cm.grainlabs~
	git clone https://github.com/CircuitMusicLabs/cm.grainlabs

After you downloaded the entire repository, open the included Xcode projects and compile the externals with via Product > Build (Command-B).

##Installation
The petra repository is structured as a Max package.

###Installing the Max Package (Max 6 - Mac OS X)
Copy the entire petra repository either into the "packages" folder in your Max installation or into "Max/Packages" in your ~/Documents folder.

###Installing the Max Package (Max 7 - Mac OS X)
Copy the entire cm.grainlabs~ folder inside the "max-package" directory into “Max 7/Packages" in your ~/Documents folder.
