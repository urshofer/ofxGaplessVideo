#include "ofxGaplessVideoplayer.h"

// Constructor
ofxGaplessVideoPlayer::ofxGaplessVideoPlayer() {
    hasPreview      = false;
    currentMovie    = 0;
    pendingMovie    = 1;
	players[0].actionTimeout   = 0;
	players[1].actionTimeout   = 0;
    forcetrigger    = false;
    state           = empty;

	
    
	#ifdef TARGET_LINUX
	players[0].video.getPlayer<ofGstVideoPlayer>()->setAsynchronousLoad(true);
	players[1].video.getPlayer<ofGstVideoPlayer>()->setAsynchronousLoad(true);
	#endif
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
    ofLogError() << "+ ENQUEUE: LoadMovie: " << _name;
    ofxGaplessVideoPlayer::command c;
    c.c = "loadMovie";
    c.n = _name;
    c.i = _in;
    c.o = _out;
    if (lock()) {
        queue.push_back(c);
        unlock();
    }
}


//--------------------------------------------------------------
void ofxGaplessVideoPlayer::appendMovie(string _name, bool _in, bool _out){
    ofLogError() << "+ ENQUEUE: appendMovie: " << _name;
    ofxGaplessVideoPlayer::command c;
    c.c = "appendMovie";
    c.n = _name;
    c.i = _in;
    c.o = _out;
    if (lock()) {
        queue.push_back(c);
        unlock();
    }
}

//--------------------------------------------------------------
void ofxGaplessVideoPlayer::triggerMovie(string _name){
    ofLogError() << "+ ENQUEUE: triggerMovie: " << _name;
    ofxGaplessVideoPlayer::command c;
    c.c = "triggerMovie";
    c.n = _name;
    c.i = false;
    c.o = false;
    if (lock()) {
        queue.push_back(c);
        unlock();
    }
}


//--------------------------------------------------------------
// Dequeue Function: triggered from the update func(main thread)
//--------------------------------------------------------------
void ofxGaplessVideoPlayer::_loadMovie(string _name, bool _in, bool _out){
    ofLogError() << "+ DEQUEUE: loadMovie: " << _name;
    appendMovie(_name,_in,_out);
    triggerMovie(_name);
}


//--------------------------------------------------------------
void ofxGaplessVideoPlayer::_appendMovie(string _name, bool _in, bool _out){
    ofLogError() << "+ DEQUEUE: appendMovie: " << _name;
    players[pendingMovie].actionTimeout = ofGetElapsedTimeMillis();
    players[pendingMovie].loadTime = players[pendingMovie].actionTimeout;
    players[pendingMovie].video.setVolume(0);
    players[pendingMovie].video.close();
    players[pendingMovie].video.loadAsync(_name);
    players[pendingMovie].video.setPaused(true);
    players[pendingMovie].fades.in  = _in;
    players[pendingMovie].fades.out = _out;
}

//--------------------------------------------------------------
void ofxGaplessVideoPlayer::_triggerMovie(string _name){
    ofLogError() << "+ DEQUEUE: triggerMovie: " << _name;

    if(players[pendingMovie].video.isPaused()) {
        players[pendingMovie].video.setPaused(false);
        players[currentMovie].video.setPaused(true);
    }
    else {
        _appendMovie(_name, false, false);
        players[pendingMovie].video.setPaused(false);
    }
    pendingMovie = currentMovie;
    currentMovie = pendingMovie==1?0:1;

}



//--------------------------------------------------------------
void ofxGaplessVideoPlayer::update(){
    
    if (queue.size()>0) {
        ofxGaplessVideoPlayer::command next_command;
        if (lock()) {
            next_command = queue.front();
            queue.pop_front();
            unlock();
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
//    if (players[0].video.isLoaded())
        players[0].video.update();
//    if (players[1].video.isLoaded())
        players[1].video.update();
}


//--------------------------------------------------------------
bool ofxGaplessVideoPlayer::draw(int x, int y, int w, int h){
    static bool ready = false;
	static bool force_hide = false;

    if (players[currentMovie].video.isPlaying() && !force_hide) {
        float fade = 1.0f;
		static float old_fade = 0.0f;
        if (players[currentMovie].fades.in && players[currentMovie].video.getCurrentFrame()<25) {
            fade = 1.0f / 25.0f * CLAMP(players[currentMovie].video.getCurrentFrame()-1, 0, 25);
            ofLogVerbose() << "Fade in " << fade;
        }
        else if (players[currentMovie].fades.out && (players[currentMovie].video.getTotalNumFrames()-players[currentMovie].video.getCurrentFrame())<25) {
            fade = 1.0f / 25.0f * CLAMP(players[currentMovie].video.getTotalNumFrames()-players[currentMovie].video.getCurrentFrame(), 0, 25);
            ofLogVerbose() << "Fade out " << fade;
        }
        ofPushStyle();
        ofEnableBlendMode(OF_BLENDMODE_ALPHA);
        ofSetColor(255 * fade, 255 * fade, 255 * fade, 255 * fade);
		if (fade != old_fade) {
			players[currentMovie].video.setVolume(fade);
            ofLogVerbose() << "Fade Audio: " << fade;
		}
        players[currentMovie].video.draw(x, y, w, h);
      	ofDisableBlendMode();
        ofPopStyle();
        ready = true;
		old_fade = fade;
    }
    
    if (players[pendingMovie].video.isLoaded() && hasPreview) {
        players[pendingMovie].video.draw(w-w/4-2, 2, w/4, h/4);
    }
    
    if (hasPreview) {
        ofPushStyle();
        ofNoFill();
        ofSetColor(255, 255, 255);
        ofRect(w-w/4-2, 2, w/4, h/4);
        ofSetColor(255, 0, 0);
        ostringstream os;
        os << "Activ : " << getCurrentMovie() << endl;
        os << "State : " << getState() << endl;
        os << "Load  : " << getLoadTime() << endl;
        os << "Frame : " << getCurrentFrame() << "/" << getTotalNumFrames() << endl;
        os << "FPS   : " << ofGetFrameRate() << endl;
        ofDrawBitmapString(os.str(), w-w/4+2, 15);
        ofPopStyle();
    }

	force_hide = players[currentMovie].video.getCurrentFrame() == players[currentMovie].video.getTotalNumFrames();

    return ready;
}

void ofxGaplessVideoPlayer::start() {

}

void ofxGaplessVideoPlayer::stop() {
    players[0].video.close();
    players[1].video.close();
}

