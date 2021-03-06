// Uncomment this to compile against GStreamer
// On Linux, GStreamer is the default anyway.
// #define GSTREAMER_ON_OSX


#include "ofxGaplessVideoplayer.h"


// Constructor
ofxGaplessVideoPlayer::ofxGaplessVideoPlayer() {
    hasPreview      = false;
    receivedVolumeChange = false;
    currentMovie    = 0;
    pendingMovie    = 1;
	players[0].actionTimeout   = 0;
	players[1].actionTimeout   = 0;
	players[0].maxVol = 1.0f;
	players[1].maxVol = 1.0f;
#ifdef GSTREAMER_ON_OSX
    players[0].video.setPlayer(std::shared_ptr<ofGstVideoPlayer>(new ofGstVideoPlayer));
    players[1].video.setPlayer(std::shared_ptr<ofGstVideoPlayer>(new ofGstVideoPlayer));
#endif
    state           = empty;
}

// Desonstructor
ofxGaplessVideoPlayer::~ofxGaplessVideoPlayer() {
    
}

//--------------------------------------------------------------
void ofxGaplessVideoPlayer::setPreview(bool p){
    hasPreview = p;
}

//--------------------------------------------------------------
void ofxGaplessVideoPlayer::togglePreview(){
    hasPreview = !hasPreview;
}

void ofxGaplessVideoPlayer::setVolume(float _volume) {
    players[currentMovie].maxVol = _volume;
    players[pendingMovie].maxVol = _volume;
    receivedVolumeChange = true;
}


//--------------------------------------------------------------
// Enqueue Function: triggered from another thread
//--------------------------------------------------------------
void ofxGaplessVideoPlayer::loadMovie(string _name, bool _in, bool _out){
    if (state == empty) return;
    ofLogVerbose() << "+ [net] loadMovie: " << _name;
    ofxGaplessVideoPlayer::command c;
    c.c = "loadMovie";
    c.n = _name;
    c.i = _in;
    c.o = _out;
    if (lock()) {
        queue.push_back(c);
        unlock();
    }
    else {
        ofLogVerbose() << "x loadMovie: no lock!";
    }
}


//--------------------------------------------------------------
void ofxGaplessVideoPlayer::appendMovie(string _name, bool _in, bool _out){
    if (state == empty) return;
    ofLogVerbose() << "+ [net] appendMovie: " << _name;
    ofxGaplessVideoPlayer::command c;
    c.c = "appendMovie";
    c.n = _name;
    c.i = _in;
    c.o = _out;
    if (lock()) {
        queue.push_back(c);
        unlock();
    }
    else {
        ofLogVerbose() << "x appendMovie: no lock!";
    }
}

//--------------------------------------------------------------
void ofxGaplessVideoPlayer::triggerMovie(string _name){
    if (state == empty) return;
    ofLogVerbose() << "+ [net] triggerMovie: " << _name;
    ofxGaplessVideoPlayer::command c;
    c.c = "triggerMovie";
    c.n = _name;
    c.i = false;
    c.o = false;
    if (lock()) {
        queue.push_back(c);
        unlock();
    }
    else {
        ofLogVerbose() << "x triggerMovie: no lock!";
    }
}


//--------------------------------------------------------------
// Dequeue Function: triggered from the update func(main thread)
//--------------------------------------------------------------
void ofxGaplessVideoPlayer::_loadMovie(string _name, bool _in, bool _out){
    ofLogVerbose() << "        " << _name << " loaded";
    appendMovie(_name,_in,_out);
    triggerMovie(_name);
}


//--------------------------------------------------------------
void ofxGaplessVideoPlayer::_appendMovie(string _name, bool _in, bool _out){
    if (state != ready) return;
    players[pendingMovie].loadTime = ofGetElapsedTimeMillis();
    players[pendingMovie].video.loadAsync(_name);
    players[pendingMovie].loadTime = ofGetElapsedTimeMillis() - players[pendingMovie].loadTime;
    players[pendingMovie].fades.in  = _in;
    players[pendingMovie].fades.out = _out;
    state = appended;
    ofLogError(ofToString(ofGetElapsedTimef(),2)) << "[" << pendingMovie << "] " << _name << " appended";
}

//--------------------------------------------------------------
void ofxGaplessVideoPlayer::_triggerMovie(string _name){
    if(state == waiting) {
        players[pendingMovie].video.setVolume(0.0f);
        players[pendingMovie].video.setPaused(false);
        state = switching;
        ofLogError(ofToString(ofGetElapsedTimef(),2)) << "[" << pendingMovie << "] " << _name << " triggered";
    }
    else {
        players[pendingMovie].video.loadAsync(_name);
        players[pendingMovie].fades.in  = false;
        players[pendingMovie].fades.out = false;
        state = forceappended;
        ofLogError(ofToString(ofGetElapsedTimef(),2)) << "[" << pendingMovie << "] " << _name << " Force triggered";
    }
    
}



//--------------------------------------------------------------
void ofxGaplessVideoPlayer::update(){

    if (state==empty) state = ready;
    
    if (queue.size()>0) {
        ofxGaplessVideoPlayer::command next_command;
        int t = ofGetElapsedTimeMillis();
        if (lock()) {
            next_command = queue.front();
            queue.pop_front();
            unlock();
        }
        else {
            ofLogVerbose() << "x update: no lock!";
        }


        if (next_command.c == "loadMovie") {
            _loadMovie(next_command.n,next_command.i,next_command.o);
        }
        if (next_command.c == "appendMovie") {
            _appendMovie(next_command.n,next_command.i,next_command.o);
        }
        if (next_command.c == "triggerMovie") {
            _triggerMovie(next_command.n);
        }
    }

    /* After Focde Trigger */
    
    if (state == forceappended) {
        if(players[pendingMovie].video.isLoaded()) {
            players[pendingMovie].video.setVolume(0.0f);
            players[pendingMovie].video.setPaused(false);
            state = switching;
        }
    }
    
    
    /* After Append & Loaded: Switch to Pause and prepare for Trigger */
    
    if (state == appended) {
        if(players[pendingMovie].video.isLoaded()) {
            players[pendingMovie].video.setPaused(true);
            state = waiting;
        }
    }

    /* After Switch: Pause other clip & mute */
    
    if (state == switched) {
        players[pendingMovie].video.setVolume(0.0f);
        players[pendingMovie].video.setPaused(true);
        state = ready;
    }
    
    /* During Switch: Wait until not paused and current Frame > 1 */
    
    if (state == switching) {
        if(players[pendingMovie].video.isPlaying() && players[pendingMovie].video.getCurrentFrame() > 1) {
            pendingMovie = currentMovie;
            currentMovie = pendingMovie==1?0:1;
            state = switched;
            players[currentMovie].video.setVolume(!players[currentMovie].fades.in ? players[currentMovie].maxVol : 0);
            ofLogVerbose() << "Set Audio to " << players[currentMovie].maxVol;
        }
    }

    if(players[currentMovie].video.isLoaded())
        players[currentMovie].video.update();
    if(players[pendingMovie].video.isLoaded())
        players[pendingMovie].video.update();
    
    
}


//--------------------------------------------------------------
bool ofxGaplessVideoPlayer::draw(int x, int y, int w, int h){
    ofLogVerbose() << "Start Draw... ";
    
    static bool isDrawing = false;

    int current_pos = players[currentMovie].video.getCurrentFrame();
    int total_pos = players[currentMovie].video.getTotalNumFrames();

    if (!players[currentMovie].video.isPaused() && !players[currentMovie].video.getIsMovieDone()) {
        ofPushStyle();
        float fade = 1.0f;
        static float old_fade = 0.0f;
        if (players[currentMovie].fades.out || players[currentMovie].fades.in) {
            int length = total_pos>50 ? 25 : (total_pos > 2 ? floor(total_pos/2) : 1);
            if (players[currentMovie].fades.in && current_pos<length) {
                fade = 1.0f / (float)length * CLAMP(current_pos-1, 0, length);
                ofLogVerbose() << "Fade in " << fade;
            }
            if (players[currentMovie].fades.out && (total_pos-current_pos)<length) {
                fade = 1.0f / (float)length * CLAMP(total_pos-current_pos, 0, length);
                ofLogVerbose() << "Fade out " << fade;
            }
            ofEnableBlendMode(OF_BLENDMODE_ALPHA);
            ofSetColor(255 * fade, 255 * fade, 255 * fade, 255 * fade);
            if (fade != old_fade) {
                players[currentMovie].video.setVolume(CLAMP(players[currentMovie].maxVol * fade, 0.0f, 1.0f));
                ofLogVerbose() << "Fade Audio: " << fade;
            }
        }
        else {
            ofSetColor(255,255,255,255);
        }

        players[currentMovie].video.draw(x, y, w, h);
      	ofDisableBlendMode();
        ofPopStyle();
        isDrawing = true;
        
        if (receivedVolumeChange && fade == old_fade) {
            players[currentMovie].video.setVolume(CLAMP(players[currentMovie].maxVol, 0.0f, 1.0f));
            receivedVolumeChange = false;
        }
        
        old_fade = fade;
        
        
    }
    
    if (hasPreview) {
        ofPushStyle();
        ofSetColor(0, 0, 0, 150);
        ofDrawRectangle(w-w/4-2, 0, w/4+2, h);
        ofNoFill();
        ofSetColor(255, 0, 0);
        ostringstream os;
        os << "Current"  << endl;
        os << "Load    : " << players[currentMovie].loadTime << endl;
        os << "Frame   : " << current_pos << "/" << total_pos << endl;
        os << "Playing : " << players[currentMovie].video.isPlaying() << endl;
        os << "Paused  : " << players[currentMovie].video.isPaused() << endl;
        os << "Loaded  : " << players[currentMovie].video.isLoaded() << endl << endl;
        
        os << "Pending"  << endl;
        os << "Load    : " << players[pendingMovie].loadTime << endl;
        os << "Frame   : " << players[pendingMovie].video.getCurrentFrame() << "/" << players[pendingMovie].video.getTotalNumFrames() << endl;
        os << "Playing : " << players[pendingMovie].video.isPlaying() << endl;
        os << "Paused  : " << players[pendingMovie].video.isPaused() << endl;
        os << "Loaded  : " << players[pendingMovie].video.isLoaded() << endl;
        
        os << "State   : " << state_string[state] << endl << "------------------" << endl;
#ifdef GSTREAMER_ON_OSX
        os << "GStreamer on OSX" << endl;
#else
        os << "ofVideoPlayer" << endl;
#endif


        ofDrawBitmapString(os.str(), w-w/4+2, 17 + h/4);

        ofDisableAntiAliasing();
        ofSetColor(255, 255, 255);
        if(players[pendingMovie].video.isLoaded()) {
            players[pendingMovie].video.draw(w-w/4, 1, w/4-1, h/4-1);
        }
        ofDrawRectangle(w-w/4, 1, w/4-1, h/4-1);
        
        ofPopStyle();
    }

    ofLogVerbose() << "Stop Draw... ";

    return isDrawing;
}

void ofxGaplessVideoPlayer::start() {

}

void ofxGaplessVideoPlayer::stop() {
#ifdef _THREADED_PLAYER
    players[0].video.stop();
    players[1].video.stop();
#else
    players[0].video.close();
    players[1].video.close();
#endif
}

