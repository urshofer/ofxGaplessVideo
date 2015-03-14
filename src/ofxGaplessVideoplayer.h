#ifndef _THREADED_VIDEOPLAYER
#define _THREADED_VIDEOPLAYER

#include "ofMain.h"
#define MAX_VIDEOS 2

class ofxGaplessVideoPlayer : public ofThread{
    
private:
    
//	ofVideoPlayer videos[MAX_VIDEOS];
		
    int currentMovie, pendingMovie;
    
    enum PStatus { empty, ready, appended, switching, switched };
    PStatus state;


    bool hasPreview;
    
    /* Command Queue */
    struct command {
        string c;
        string n;
        bool i;
        bool o;
    };
    deque<command> queue;

    /* Fades */
    struct fade {
        bool in;
        bool out;
    };


    /* Players */
    struct player {
        ofVideoPlayer video;
        fade          fades;
        int           loadTime;
        int           actionTimeout;
    };
    player players[MAX_VIDEOS];

    void _loadMovie(string _name, bool _in, bool _out);
    void _appendMovie(string _name, bool _in, bool _out);
    void _triggerMovie(string _name);
    
public:
    
    ofxGaplessVideoPlayer();
    ~ofxGaplessVideoPlayer();

    void start();
    void stop();
    
    int getCurrentMovie() {return currentMovie;}
    int getState() {return state;}
    int getLoadTime() {return players[currentMovie].video.isLoaded() ? players[currentMovie].loadTime : 0;}
    
    int getCurrentFrame() {return players[currentMovie].video.isLoaded() ? players[currentMovie].video.getCurrentFrame() : 0;}
    int getTotalNumFrames() {return players[currentMovie].video.isLoaded() ? players[currentMovie].video.getTotalNumFrames() : 0;}
    
    void setPreview(bool p);
    void togglePreview();
    
    void loadMovie(string _name, bool _in, bool _out);
    void appendMovie(string _name, bool _in, bool _out);
    void triggerMovie(string _name);

    void update();
    bool draw(int x, int y, int w, int h);
    bool draw() {return draw(0,0,ofGetWidth(),ofGetHeight());}
  	
};

#endif
