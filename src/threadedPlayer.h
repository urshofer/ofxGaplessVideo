#ifndef _THREADED_PLAYER
#define _THREADED_PLAYER

#include "ofMain.h"

class threadedPlayer : public ofThread{

private:

    /* Player */
    
    ofVideoPlayer player;
    
    /* Texture */
    
    ofTexture tex;
    
    
    /* Command Queue */
    struct c {
        string command;
        float par_float;
        bool par_bool;
        string par_string;
    };
    deque<c>    queue;

    /* State Struct */
    struct g {
        int	getCurrentFrame;
        int	getTotalNumFrames;
        bool isFrameNew;
        bool isPaused;
        bool isLoaded;
        bool isPlaying;
        bool isMovieDone;
    };
    g           getters;
    
    /* Push Command */
    void push_command(string _name, string _stringParam = "", bool _bparam = false, float _fparam = 0.0f){
        c c;
        c.command = _name;
        c.par_float = _fparam;
        c.par_bool = _bparam;
        c.par_string = _stringParam;
        if (lock()) {
            queue.push_back(c);
            unlock();
        }
        else {
            ofLogVerbose() << "x loadMovie: no lock!";
        }
    }
    
    
public:

   //--------------------------
   // Setters - Piped on Queue

    
    bool load(string name) {
        push_command("load", name);
    }
    bool loadAsync(string name) {
        push_command("load", name);
    }
    void close() {
        push_command("close");
    }
    void play() {
        push_command("play");
    }
    void _stop() {
        push_command("stop");
    }
    void setVolume(float volume) {
        push_command("setVolume",  "", false, volume);
    }
    void setPaused(bool bPause) {
        push_command("setPaused",  "", bPause);
    }
    

    
    // Getters
    int	getCurrentFrame() {
        ofScopedLock lock(mutex);
        return getters.getCurrentFrame;
    }
    int	getTotalNumFrames() {
        ofScopedLock lock(mutex);
        return getters.getTotalNumFrames;
    }
    bool isFrameNew() {
        ofScopedLock lock(mutex);
        return getters.isFrameNew;
    }
    bool isPaused() {
        ofScopedLock lock(mutex);
        return getters.isPaused;
    }
    bool isLoaded() {
        ofScopedLock lock(mutex);
        return getters.isLoaded;
    }
    bool isPlaying() {
        ofScopedLock lock(mutex);
        return getters.isPlaying;
    }
    
    // Update copy pixels into texture
    void update() {
        if (lock()) {
            if (player.isFrameNew()) {
                if (player.getWidth() != tex.getWidth() || player.getHeight() != tex.getHeight()) {
                    tex.allocate(player.getPixels());
                }
                tex.loadData(player.getPixels());
            }
            unlock();
        }
   }

    // Draw texture
    void draw(float x, float y, float w, float h) {
      tex.draw(x,y,w,h);
    }
    
    bool getIsMovieDone()
    {
        ofScopedLock lock(mutex);
        return getters.isMovieDone;
    }
    
    
   //--------------------------
    

   threadedPlayer() {
       player.setUseTexture(false);
       player.close();
   }

   bool start(){
       startThread();
       return true;
   }
   
   void stop(){
    stopThread();
    player.close();
    std::cout  << "Player Stop\n";
   }


   //--------------------------
   void threadedFunction(){
       while (isThreadRunning()) {
           // Working on Command Queue
           if (queue.size()>0) {
               c next;
               if (lock()) {
                   next = queue.front();
                   queue.pop_front();
                   unlock();
               }

               ofLogVerbose() << "******************************************************";
               if (next.command == "load") {
                   ofLogVerbose() << "------------------ COMMAND: " << next.command << " / " << next.par_string;
                   player.close();
                   player.load(next.par_string);
               }
               else if (next.command == "close") {
                   ofLogVerbose() << "------------------ COMMAND: " << next.command;
                   if (player.isLoaded())
                       player.close();
               }
               else if (next.command == "play") {
                   ofLogVerbose() << "------------------ COMMAND: " << next.command;
                   if (player.isLoaded())
                       player.play();
               }
               else if (next.command == "stop") {
                   ofLogVerbose() << "------------------ COMMAND: " << next.command;
                   if (player.isLoaded())
                       player.stop();
               }
               else if (next.command == "setVolume") {
                   ofLogVerbose() << "------------------ COMMAND: " << next.command;
                   player.setVolume(next.par_float);
               }
               else if (next.command == "setPaused") {
                   ofLogVerbose() << "------------------ COMMAND: " << next.command;
                   player.setPaused(next.par_bool);
               }
               ofLogVerbose() << "******************************************************";

           }
           if (lock()) {
               getters.isLoaded          = player.isLoaded();
               if (getters.isLoaded) {
                   player.update();
                   getters.getCurrentFrame   = player.getCurrentFrame();
                   getters.getTotalNumFrames = player.getTotalNumFrames();
                   getters.isFrameNew        = player.isFrameNew();
                   getters.isPaused          = player.isPaused();
                   getters.isPlaying         = player.isPlaying();
                   getters.isMovieDone       = player.getIsMovieDone();
               }
               unlock();
           }
           ofSleepMillis(5);
       }
   }


};

#endif
