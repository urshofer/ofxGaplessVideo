# ofxGaplessVideo
Gapless Video Playback for OpenFrameworks. Runs with avFoundation on OSX and gStreamer on Linux. Use it with openFrameworks master, since both playback systems are loading clips way faster.

- ofxGaplessVideo is used for remote controlled video playback over the network, whereas the control over the player runs in an own thread.
- ofxGaplessVideo provides functions to preload movies flicker free and to trigger a new clip gaplessly, since it uses a queue preloads the clips in a hidden texture.
- ofxGaplessVideo is thread safe, which means, the control over the playback can be done in an own thread.

Basic Functions in the Main Loop:
---

    /* Setup Function */
    MO.start();
    
    /* Update Function */
    MO.update();
    
    /* Draw Function (Fullscreen without Options */
    MO.draw()

Controlling Playback:
---

It's probably the most efficient way to pass the MO Object to a loader thread which is waiting for signals from a controller server.
Please note the MO->... -  I assume that you pass MO to a thread as a reference.

Loads a Movie and starts playback immediately:

    MO->loadMovie(string movieFile, bool fade_in, bool fade_out);

Preloads a Movie:

    MO->appendMovie(string movieFile, bool fade_in, bool fade_out);

Starts a preloaded Movie. The parameter movieFile should be set to the same value as in appendMovie.

    MO->triggerMovie(string movieFile);
