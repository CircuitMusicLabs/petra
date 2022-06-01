# Petra

Max package for granular synthesis, originally by Matthias W. Müller -
[@verenasounds](https://twitter.com/verenasounds) -
[www.circuitmusiclabs.com](http://www.circuitmusiclabs.com/). Under maintenance by Cycling '74.

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

## How to use petra
After installing, open the [petra_overview.maxpat](https://github.com/Cycling74/petra/blob/master/extras/petra_overview.maxpat) file. It contains the entire documentation and links to all object helpfiles.


## Bug reports

Please report bugs with the petra package on the GitHub [issues](https://github.com/Cycling74/petra/issues) tracker.
