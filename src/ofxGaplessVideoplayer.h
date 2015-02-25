#ifndef _THREADED_VIDEOPLAYER
#define _THREADED_VIDEOPLAYER

#include "ofMain.h"
#define MAX_VIDEOS 2

class ofxGaplessVideoPlayer : public ofThread{
    
private:
    
	ofVideoPlayer videos[MAX_VIDEOS];
		
    int currentMovie, loadTime, actionTimeout;
    
    enum PStatus { empty, loading, loaded, ready, prerolling, playing, stopping };
    PStatus state;
    
    enum PAction { null, load, pivot, loadpivot };
    PAction action;

    bool hasPreview, forcetrigger;

    string name;

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
    fade fades[MAX_VIDEOS];
    bool in, out;


    
public:
    
    ofxGaplessVideoPlayer();
    ~ofxGaplessVideoPlayer();

    void start();
    void stop();
    
    int getCurrentMovie() {return currentMovie;}
    int getState() {return state;}
    int getLoadTime() {return loadTime;}
    
    int getCurrentFrame() {return videos[currentMovie].getCurrentFrame();}
    int getTotalNumFrames() {return videos[currentMovie].getTotalNumFrames();}
    
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
