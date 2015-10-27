#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    MO.start();
}

//--------------------------------------------------------------
void ofApp::update(){
    static int lastMod = ofGetElapsedTimeMillis();
    static int action = 0;
    static int moviecount = 1;
    static string movieFile = "";
    if (ofGetElapsedTimeMillis() - lastMod > 500) {
        if (action == 0) {
            cout << "Append " << ofToString(moviecount) << ".mp4" << endl;
            movieFile = ofToString(moviecount) + ".mp4";
            MO.appendMovie(movieFile, false, false);
            action++;
            if (moviecount < 3)
                moviecount ++;
            else
                moviecount = 1;
        }
        else if (action == 1) {
            cout << "Trigger" << endl;
            MO.triggerMovie(movieFile);
            action = 0;
        }
        lastMod = ofGetElapsedTimeMillis();
    }
    MO.update();
}

//--------------------------------------------------------------
void ofApp::draw(){
    float w = MO.getWidth();
    float h = MO.getHeight();
    MO.draw(0,0 , w, h);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
