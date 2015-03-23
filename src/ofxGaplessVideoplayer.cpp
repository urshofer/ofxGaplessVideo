// Uncomment this to compile against GStreamer
// On Linux, GStreamer is the default anyway.
//#define GSTREAMER_ON_OSX


#include "ofxGaplessVideoplayer.h"
#ifdef GSTREAMER_ON_OSX
#include "ofGstVideoPlayer.h"
#endif

// Constructor
ofxGaplessVideoPlayer::ofxGaplessVideoPlayer() {
    hasPreview      = false;
    currentMovie    = 0;
    pendingMovie    = 1;
	players[0].actionTimeout   = 0;
	players[1].actionTimeout   = 0;
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

    ofLogVerbose() << "        " << _name << " appended";
    players[pendingMovie].actionTimeout = ofGetElapsedTimeMillis();
//    players[pendingMovie].video.close();
    players[pendingMovie].actionTimeout = ofGetElapsedTimeMillis() - players[pendingMovie].actionTimeout;

    players[pendingMovie].loadTime = ofGetElapsedTimeMillis();
    players[pendingMovie].video.loadAsync(_name);
    players[pendingMovie].loadTime = ofGetElapsedTimeMillis() - players[pendingMovie].loadTime;

    players[pendingMovie].fades.in  = _in;
    players[pendingMovie].fades.out = _out;
 
    state = appended;
    
    ofLogError() << "[" << pendingMovie << "] Loading: " << players[pendingMovie].loadTime << " Closing: " << players[pendingMovie].actionTimeout;
}

//--------------------------------------------------------------
void ofxGaplessVideoPlayer::_triggerMovie(string _name){
    if(state == waiting) {
        ofLogVerbose() << "        " << _name << " triggered";
        players[pendingMovie].video.setPaused(false);
        state = switching;
    }
    else {
        ofLogError() << "        " << _name << " forcefully triggered";
//        players[pendingMovie].video.close();
        players[pendingMovie].video.loadAsync(_name);
        players[pendingMovie].fades.in  = false;
        players[pendingMovie].fades.out = false;
//        players[pendingMovie].video.play();
        state = forceappended;
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

    /* After Switch: Pause other clip, mute */
    
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
            if (!players[currentMovie].fades.in) {
                players[currentMovie].video.setVolume(1.0f);
                ofLogVerbose() << "Set Audio to 1";
            }
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
        if (players[currentMovie].fades.out || players[currentMovie].fades.in) {
            float fade = 1.0f;
            static float old_fade = 0.0f;
            if (players[currentMovie].fades.in && current_pos<25) {
                fade = 1.0f / 25.0f * CLAMP(current_pos-1, 0, 25);
                ofLogVerbose() << "Fade in " << fade;
            }
            if (players[currentMovie].fades.out && (total_pos-current_pos)<25) {
                fade = 1.0f / 25.0f * CLAMP(total_pos-current_pos, 0, 25);
                ofLogVerbose() << "Fade out " << fade;
            }
            ofEnableBlendMode(OF_BLENDMODE_ALPHA);
            ofSetColor(255 * fade, 255 * fade, 255 * fade, 255 * fade);
            if (fade != old_fade) {
                players[currentMovie].video.setVolume(fade);
                ofLogVerbose() << "Fade Audio: " << fade;
            }
            old_fade = fade;
        }
        else {
            ofSetColor(255,255,255,255);
        }
        players[currentMovie].video.draw(x, y, w, h);
      	ofDisableBlendMode();
        ofPopStyle();
        isDrawing = true;
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
        
        os << "State  : " << state_string[state] << endl;
        
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
    players[0].video.close();
    players[1].video.close();
}

