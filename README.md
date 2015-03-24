# ofxGaplessVideo
Gapless Video Playback for OpenFrameworks. Runs with avFoundation on OSX and gStreamer on Linux. 

Compatibility
-------------

ofxGaplessVideo relies on the loadAsync function, which only exists in openFrameworks master. Consider it as experimental.

Usage
-----

ofxGaplessVideo is used for remote controlled video playback over the network. ofxGaplessVideo provides functions to preload movies flicker free and to trigger a new clip instantly, since it uses internally two video players and preloads the clips.

ofxGaplessVideo is thread safe, which means, the playback control can be run in an own thread. Normally, this will be a network listener thread which is listening for trigger and load signals.

Basic Functions in the Main Loop
--------------------------------

    /* Setup Function */
    MO.start();
    
    /* Update Function */
    MO.update();
    
    /* Draw Function (Fullscreen without Options */
    MO.draw()

Controlling Playback
--------------------

It's probably the most efficient way to pass the MO Object as a reference to a loader thread which is waiting for signals from a controller server.

Loads a Movie and starts playback immediately:

    MO->loadMovie(string movieFile, bool fade_in, bool fade_out);

Preloads a Movie:

    MO->appendMovie(string movieFile, bool fade_in, bool fade_out);

Starts a preloaded Movie. The parameter movieFile should be set to the same value as in appendMovie:

    MO->triggerMovie(string movieFile);

Communication
-------------

Either send loadMovie signals over the net if you want to start a clip by a single command. To create a more accurate system, send a preload signal first and a trigger signal in the moment you really want to start the movie.


Compilation & Patches
---------------------

ofxGaplessVideo runs the AVFoundationPlayer in a background thread on OSX. On Linux, GStreamer is used in the main thread. Since the AVFoundationPlayer is not meant to run in the background, a patched version is needed. Replace the files in the openFrameworks/libs/openframeworks/video directory with the ones provided in the patches directory of the ofxGaplessVideo repository. Linux should run out of the box.

GStreamer on OSX
----------------

It's also possible to compile ofxGaplessVideo with GStreamer support on OSX. Uncomment




