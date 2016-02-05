#include "ofApp.h"
#include <time.h>

/**
 * Video player that secretly takes photos of the viewers,
 * displays the taken pictures afterwords,
 * followed by a photobooth.
 *
 * Saved photos found in the output folder of the application.
 *
 * Created by Andrew Sweet, Jan 2016
 */

// Configurable Values
double secondsBetweenSecretPhotosTaken = 8;
double secondsToShowSecretPhotos = 10;
double secondsBetweenSecretPhotosAndFirstPhotoBoothPhoto = 5;
double secondsBetweenPhotoBoothPhotos = 2;
int numPhotosToTakeInPhotoBooth = 5;

// 1.0 would mean instantaneous camera flash decay in photobooth photos
// 0.0 would mean white screen forever
float flashDecay = 0.07;

// Calculated Values
int videoX;
int videoY;
int videoWidth;
int videoHeight;

int numTakenPhotosInBooth;

double displaySecretPhotosTime;

double photoBoothStartTime;
double displayTimePerSecretPhoto;

bool shouldTakePhoto;
bool shouldDisplayPhotos;
bool shouldDisplayPhotoBooth;
ofVideoPlayer myPlayer;
ofVideoGrabber videoGrabber;
ofSoundPlayer soundPlayer;

ofPixels currentPhoto;
ofImage currentImage;
ofImage overlayImage;
float flash;

vector<ofPixels> recordedVideo;

string basePhotoPath = "output/photo";
int photoNumber = 0;
int numberOfPhotos = 0;
int totalNumberOfPhotos = 0;

double getTime(){
    return myPlayer.getDuration() * myPlayer.getPosition();
}

void onAppResize(){
    int vidW = 16;
    int vidH = 9;
    
    int screenWidth = ofGetScreenWidth();
    int screenHeight = ofGetScreenHeight();
    
    int x = screenWidth * vidH;
    int y = screenHeight * vidW;
    
    if (x < y){
        videoX = 0;
        videoWidth = screenWidth;
        
        videoHeight = (videoWidth / vidW) * vidH;
        videoY = (screenHeight/2) - (videoHeight/2);
    } else {
        videoY = 0;
        videoHeight = screenHeight;
        
        videoWidth = (videoHeight / vidH) * vidW;
        videoX = (screenWidth/2) - (videoWidth/2);
    }
}

double lastPhotoTime = getTime();

bool shouldDisplayNextPhoto(){
    double now = getTime();
    double secondsSinceLastPhoto = now - lastPhotoTime;
    
    if (secondsSinceLastPhoto > displayTimePerSecretPhoto){
        lastPhotoTime = now;
        return true;
    }
    return false;
}

string getNextPhotoPath(){
    string photoNumAsString = std::to_string(photoNumber);
    
    int numZeroes = 3 - photoNumAsString.length();
    
    while (numZeroes > 0){
        photoNumAsString = "0" + photoNumAsString;
        numZeroes--;
    }
    
    photoNumber++;
    
    return basePhotoPath + photoNumAsString + ".jpg";
}

void setupVideo(){
    myPlayer.load("video.mov");
    
    photoBoothStartTime = myPlayer.getDuration() - (numPhotosToTakeInPhotoBooth * secondsBetweenPhotoBoothPhotos) - secondsBetweenSecretPhotosAndFirstPhotoBoothPhoto;
    displaySecretPhotosTime = photoBoothStartTime - secondsToShowSecretPhotos;
}

//--------------------------------------------------------------
void ofApp::setup(){
    ofSetEscapeQuitsApp(false);
    ofSetDataPathRoot("../Resources/data");
    ofBackground(0, 0, 0);
    
    setupVideo();
    
    soundPlayer.load("camera.wav");
    soundPlayer.setLoop(false);
    
    videoGrabber.setDeviceID(0);
    videoGrabber.initGrabber(1440, 900);
    myPlayer.setLoopState(OF_LOOP_NONE);
    
    ofSetFullscreen(true);
    
    ofPixels overlayPixels;
    ofLoadImage(overlayPixels, "overlay.png");
    overlayImage.setFromPixels(overlayPixels);
    onAppResize();
    
    numTakenPhotosInBooth = 0;
    flash = 0;
    shouldTakePhoto = false;
    shouldDisplayPhotos = false;
    shouldDisplayPhotoBooth = false;
}

//--------------------------------------------------------------
void ofApp::update(){
    videoGrabber.update();
    myPlayer.update();
    
    if (shouldDisplayPhotos) {
        
        if (!shouldDisplayPhotoBooth){
            // Setup photoBooth
            if (getTime() > photoBoothStartTime){
                shouldDisplayPhotoBooth = true;
                photoNumber = totalNumberOfPhotos;
                photoBoothStartTime = getTime();
            } else if (shouldDisplayNextPhoto()) {
                ofLoadImage(currentPhoto, getNextPhotoPath());
                currentImage.setFromPixels(currentPhoto);
                
                lastPhotoTime = getTime();
            }
        }
    } else {
        double secondsSinceStart = getTime();
        double secondsSinceLastPhoto = getTime() - lastPhotoTime;
        if (secondsSinceStart > displaySecretPhotosTime && secondsSinceStart < photoBoothStartTime){
            shouldDisplayPhotos = true;
            numberOfPhotos = photoNumber + 1;
            displayTimePerSecretPhoto = (1.0 * secondsToShowSecretPhotos) / numberOfPhotos;
            totalNumberOfPhotos = photoNumber;
            photoNumber = 0;
            
            // Load first image
            ofLoadImage(currentPhoto, getNextPhotoPath());
            currentImage.setFromPixels(currentPhoto);
            
            lastPhotoTime = getTime();
        } else if (secondsSinceLastPhoto > secondsBetweenSecretPhotosTaken){
            lastPhotoTime = getTime();
            shouldTakePhoto = true;
        }
    }
}

void takePhoto(){
    // Take the photo
    currentPhoto = videoGrabber.getPixels();
    ofSaveImage(currentPhoto, getNextPhotoPath());
    
    currentImage.setFromPixels(currentPhoto);
    
    numTakenPhotosInBooth++;
    flash = 1.0;
    soundPlayer.play();
}

void drawPhotoBooth(){
    double secondsSincePhotoBoothStart = getTime() - photoBoothStartTime;
    
    if (secondsSincePhotoBoothStart > secondsBetweenSecretPhotosAndFirstPhotoBoothPhoto &&
        numTakenPhotosInBooth < numPhotosToTakeInPhotoBooth){
        
        double now = getTime();
        
        if (now - lastPhotoTime > secondsBetweenPhotoBoothPhotos){
            takePhoto();
            lastPhotoTime = now;
        }
        
        currentImage.draw(videoX, videoY, videoWidth, videoHeight);
        overlayImage.draw(videoX, videoY, videoWidth, videoHeight);
        
        if (flash > 0.0){
            ofSetColor(255, 255, 255, 255.0 * flash);
            ofFill();
            ofDrawRectangle(videoX, videoY, videoWidth, videoHeight);
            flash -= flashDecay;
            ofSetColor(255, 255, 255, 255);
        }
    } else {
        if (myPlayer.isPlaying()){
            videoGrabber.draw(videoX, videoY, videoWidth, videoHeight);
            ofSaveImage(videoGrabber.getPixels(), getNextPhotoPath());
            overlayImage.draw(videoX, videoY, videoWidth, videoHeight);
        }
    }
}

//--------------------------------------------------------------
void ofApp::draw(){
    ofClear(0, 0, 0);
    
    if (shouldTakePhoto){
        ofSaveImage(videoGrabber.getPixels(), getNextPhotoPath());
        shouldTakePhoto = false;
    }
    
    if (shouldDisplayPhotoBooth){
        drawPhotoBooth();
    } else if (numTakenPhotosInBooth == 0){
        if (shouldDisplayPhotos) {
            currentImage.draw(videoX, videoY, videoWidth, videoHeight);
        } else {
            myPlayer.draw(videoX, videoY, videoWidth, videoHeight);
        }
    }
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    if (key == ' '){
        // Toggle pause
        myPlayer.setPaused(myPlayer.isPlaying());
    } else if (key == 27) {
        // ESCAPE key
        ofSetFullscreen(false);
    }
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
    onAppResize();
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){
    
}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){
    
}
