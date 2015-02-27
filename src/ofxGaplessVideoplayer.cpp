#include "ofxGaplessVideoplayer.h"

// Constructor
ofxGaplessVideoPlayer::ofxGaplessVideoPlayer() {
    hasPreview      = false;
    currentMovie    = 0;
    actionTimeout        = 0;
    forcetrigger    = false;
    state           = empty;
	
	#ifdef TARGET_LINUX
	videos[0].getPlayer<ofGstVideoPlayer>()->setAsynchronousLoad(true);
	videos[1].getPlayer<ofGstVideoPlayer>()->setAsynchronousLoad(true);
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
void ofxGaplessVideoPlayer::loadMovie(string _name, bool _in, bool _out){
    ofLogVerbose() << "+ ENQUEUE: LoadMovie: " << _name << endl;
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
    ofLogVerbose() << "+ ENQUEUE: appendMovie: " << _name << endl;
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
    ofLogVerbose() << "+ ENQUEUE: triggerMovie: " << _name << endl;
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
void ofxGaplessVideoPlayer::update(){
    
    /* Process Command: Dequeue from Command Stack */

    if (queue.size()>0 && (state == ready || state == empty)) {
        ofxGaplessVideoPlayer::command next_command = queue.front();
        queue.pop_front();
        if (next_command.c == "loadMovie") {
            ofLogVerbose() << "+ DEQUEUE: LoadMovie: " << next_command.n << endl;
            action      = loadpivot;
            name        = next_command.n;
            in          = next_command.i;
            out         = next_command.o;
        }
        if (next_command.c == "appendMovie") {
            ofLogVerbose() << "+ DEQUEUE: AppendMovie: " << next_command.n << endl;
            action      = load;
            name        = next_command.n;
            in          = next_command.i;
            out         = next_command.o;
        }
        if (next_command.c == "triggerMovie") {
            ofLogVerbose() << "+ DEQUEUE: TriggerMovie: " << next_command.n << endl;
            if (state != ready) {
                loadMovie(next_command.n, false, false);
            }
            else {
                action = pivot;
            }
        }
   }

    
    /* Loading */

    if (state == loading) {
        if (videos[currentMovie==0?1:0].isLoaded()) {
            actionTimeout = ofGetElapsedTimeMillis();
            videos[currentMovie==0?1:0].play();
            state = loaded;
        }
        else if (ofGetElapsedTimeMillis() - actionTimeout > 1000) {
            state = empty;
            ofLogError() << "LOAD ERROR: " << name << endl;
        }
    }
    if (action == load || action == loadpivot) {
        loadTime = actionTimeout = ofGetElapsedTimeMillis();
        videos[currentMovie==0?1:0].setVolume(0);
        videos[currentMovie==0?1:0].loadAsync(name);
        loadTime = ofGetElapsedTimeMillis() - loadTime;
        fades[currentMovie==0?1:0].in  = in;
        fades[currentMovie==0?1:0].out = out;
        state = loading;
        forcetrigger = action==loadpivot?true:false;
    }
    
    /* Get Ready */
    
    if (state == loaded) {
        if (videos[currentMovie==0?1:0].isPlaying()) {
            videos[currentMovie==0?1:0].setPaused(true);
            state = ready;
        }
        else  if (ofGetElapsedTimeMillis() - actionTimeout > 1000) {
            state = empty;
            ofLogError() << "READY ERROR: " << name << endl;
        }
    }
    
    /* Pivot aka. Trigger */
    
    if ((action == pivot || forcetrigger) && state == ready) {
        actionTimeout = ofGetElapsedTimeMillis();
        videos[currentMovie==0?1:0].setPaused(false);
        state = playing;
        forcetrigger = false;
        actionTimeout = ofGetElapsedTimeMillis();
    }
    
    if (state == playing) {
        if (videos[currentMovie==0?1:0].isPlaying() && videos[currentMovie==0?1:0].getCurrentFrame() > 1) {
            videos[currentMovie].stop();
            currentMovie = currentMovie==0?1:0;
            state = empty;
            actionTimeout = ofGetElapsedTimeMillis() - actionTimeout;
        }
        else if (ofGetElapsedTimeMillis() - actionTimeout > 1000) {
            state = empty;
            ofLogError() << "TRIGGER ERROR: " << name << endl;
        }
    }
    
    
    videos[0].update();
    videos[1].update();
    action = null;

}


//--------------------------------------------------------------
bool ofxGaplessVideoPlayer::draw(int x, int y, int w, int h){
    static bool ready = false;
	static bool force_hide = false;

    if (videos[currentMovie].isPlaying() && !force_hide) {
        float fade = 1.0f;
		static float old_fade = 0.0f;
        if (fades[currentMovie].in && videos[currentMovie].getCurrentFrame()<25) {
            fade = 1.0f / 25.0f * CLAMP(videos[currentMovie].getCurrentFrame()-1, 0, 25);
            ofLogVerbose() << "Fade in " << fade << endl;
        }
        else if (fades[currentMovie].out && (videos[currentMovie].getTotalNumFrames()-videos[currentMovie].getCurrentFrame())<25) {
            fade = 1.0f / 25.0f * CLAMP(videos[currentMovie].getTotalNumFrames()-videos[currentMovie].getCurrentFrame(), 0, 25);
            ofLogVerbose() << "Fade out " << fade << endl;
        }
        ofPushStyle();
        ofEnableBlendMode(OF_BLENDMODE_ALPHA);
        ofSetColor(255 * fade, 255 * fade, 255 * fade, 255 * fade);
		if (fade != old_fade) {
			videos[currentMovie].setVolume(fade);		
            ofLogVerbose() << "Fade Audio: " << fade << endl;
		}
        videos[currentMovie].draw(x, y, w, h);
      	ofDisableBlendMode();
        ofPopStyle();
        ready = true;
		old_fade = fade;
    }
    
    if (videos[currentMovie==0?1:0].isLoaded() && hasPreview) {
        videos[currentMovie==0?1:0].draw(w-w/4-2, 2, w/4, h/4);
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

	force_hide = videos[currentMovie].getCurrentFrame() == videos[currentMovie].getTotalNumFrames();

    return ready;
}

void ofxGaplessVideoPlayer::start() {

}

void ofxGaplessVideoPlayer::stop() {
    videos[0].close();
    videos[1].close();
}

