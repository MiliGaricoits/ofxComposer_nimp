//
//  ofxBasePatch.cpp
//  ofxComposer-example
//
//  Created by Patricio González Vivo on 5/15/12.
//  Copyright (c) 2012 PatricioGonzalezVivo.com. All rights reserved.
//

#include "ofxBasePatch.h"

ofxBasePatch::ofxBasePatch(){
    nId         = -1;
    color.set(200,255);
    
    //  Content
    //
    image       = NULL;
    shader      = NULL;
    videoPlayer = NULL;
    videoGrabber= NULL;
    texture     = NULL;
    
    //  Post-Process
    //
    mask        = NULL;
    filter      = NULL;
    transform   = NULL;
    
    //  Flags
    //
    bEditMode   = true;
    bEditMask   = false;
    bActive     = false;
    bVisible    = true;
    
    //  Add Events Listeners
    //
	ofAddListener(ofEvents().mousePressed, this, &ofxBasePatch::_mousePressed);
	ofAddListener(ofEvents().mouseDragged, this, &ofxBasePatch::_mouseDragged);
	ofAddListener(ofEvents().mouseReleased, this, &ofxBasePatch::_mouseReleased);
	ofAddListener(ofEvents().keyPressed, this, &ofxBasePatch::_keyPressed);
};

ofxBasePatch::~ofxBasePatch(){
    
    //  Delete Content
    //
    if ( image != NULL )
        delete image;
    
    if ( shader != NULL )
        delete shader;
    
    if ( videoPlayer != NULL )
        delete videoPlayer;
    
    if ( videoGrabber != NULL )
        delete videoGrabber;
    
    if ( texture != NULL )
        delete texture;
    
    //  Delete Post-Process
    //
    if ( mask != NULL )
        delete mask;
    
    if ( filter != NULL )
        delete filter;
    
    if ( transform != NULL )
        delete transform;
    
    //  Delete Links
    //
    inPut.clear();
    outPut.clear();
    
    //  Remove Events Listeners
    //
    ofRemoveListener(ofEvents().mousePressed, this, &ofxBasePatch::_mousePressed);
	ofRemoveListener(ofEvents().mouseDragged, this, &ofxBasePatch::_mouseDragged);
	ofRemoveListener(ofEvents().mouseReleased, this, &ofxBasePatch::_mouseReleased);
	ofRemoveListener(ofEvents().keyPressed, this, &ofxBasePatch::_keyPressed);
}

bool ofxBasePatch::loadType(string _type){
    
}

bool ofxBasePatch::loadFile(string _filePath){
    
}

bool ofxBasePatch::loadXmlSettings(ofxXmlSettings &_XML){
    
}

bool ofxBasePatch::saveXmlSettings(ofxXmlSettings &_XML){
    
}

string ofxBasePatch::getType(){
    if (image != NULL)
        return "ofImage";
    else if (videoPlayer != NULL)
        return "ofVideoPlayer";
    else if (videoGrabber != NULL)
        return "ofVideoGrabber";
    else if (texture != NULL)
        return "ofTexture";
    else if (shader != NULL)
        return "ofShader";
}

ofTexture&  ofxBasePatch::getContentTextureReference(){
    if (image != NULL)
        return image->getTextureReference();
    else if (videoPlayer != NULL)
        return videoPlayer->getTextureReference();
    else if (videoGrabber != NULL)
        return videoGrabber->getTextureReference();
    else if (shader != NULL)
        return shader->getTextureReference();
    else if (texture != NULL)
        return *texture;
}

ofTexture& ofxBasePatch::getTextureReference(){
    if (mask != NULL)
        return mask->getTextureReference();
    else
        return getContentTextureReference();
    
}

void ofxBasePatch::update(){
    if ((width != getContentTextureReference().getWidth()) ||
        (height != getContentTextureReference().getHeight()) ){
        width = getContentTextureReference().getWidth();
        height = getContentTextureReference().getHeight();
        
        bUpdateCoord = true;
        bUpdateMask = true;
    }
    
    if (mask != NULL) {
        
        if (mask->needUpdate){
            mask->allocate(width,height);
            mask->makeMask();
            
            bUpdateMask = false;
        }
            
        if ( inside(ofGetMouseX(), ofGetMouseY()) && (bEditMode)){
            mask->texOpacity = ofLerp(mask->texOpacity,1.0, 0.01);
            mask->maskOpacity = ofLerp(mask->maskOpacity,0.8, 0.01);
        } else if (!bEditMode){
            mask->texOpacity = ofLerp(mask->texOpacity, 1.0, 0.01);
            mask->maskOpacity = ofLerp(mask->maskOpacity, 0.0, 0.01);
        } else {
            mask->texOpacity = ofLerp(mask->texOpacity, 0.8, 0.01);
            mask->maskOpacity = ofLerp(mask->maskOpacity, 0.0, 0.01);
        }
        
        mask->begin();
        getContentTextureReference().draw(0,0);
        mask->end();
    }
    
    if (transformation != NULL){
        if (transformation->needUpdate){
            doSurfaceToScreenMatrix();
            
            box = textureCorners.getBoundingBox();
            outPutPos.set( box.x + box.width + 4, box.y + box.height*0.5 );
            for(int i = 0; i < outPut.size(); i++){
                outPut[i].pos = outPutPos;
            }
            int total = inPut.size();
            for(int i = 0; i < total; i++){
                inPut[i].pos.set( box.x - 4, box.y + (box.height/(total))* (i+0.5) );
            }
            
            bUpdateCoord = false;
        }
    }
    
    // Update shader or video content
    //
    if (videoPlayer != NULL){
        videoPlayer->update();
    } else if (videoGrabber != NULL){
        videoGrabber->update();
    } else if (shader != NULL){
        shader->update();
    }
    
    // Send the texture to the linked Patches
    //
    for ( int i = 0; i < outPut.size(); i++ ){
        if (outPut[i].toShader != NULL){
            outPut[i].toShader->setTexture( getTextureReference(), outPut[i].nTex );
        }
    }
}

void ofxPatch::draw(){
    
    if ( bEditMode || bVisible ) {
        
        if (bActive || !bEditMode || (type == "ofxGLEditor"))
            color.lerp(ofColor(255,255), 0.1);
        else
            color.lerp(ofColor(200,200), 0.1);
        
        
        ofPushMatrix();
        glMultMatrixf(glMatrix);
        ofSetColor(color);
        getTextureReference().draw(0,0);
        ofPopMatrix();
    }
    
    if (bEditMode){
        ofPushStyle();
        
        if (title != NULL)
            title->draw();
        
        if ( !bEditMask ){
            ofFill();
            // Draw dragables texture corners
            //
            for(int i = 0; i < 4; i++){
                if ( ( selectedTextureCorner == i) || ( ofDist(ofGetMouseX(), ofGetMouseY(), textureCorners[i].x, textureCorners[i].y) <= 4 ) ) ofSetColor(200,255);
                else ofSetColor(color,100);
                
                ofRect(textureCorners[i].x-4,textureCorners[i].y-4, 8,8);
                
                // Draw contour Line
                //
                ofLine(textureCorners[i].x, textureCorners[i].y, textureCorners[(i+1)%4].x, textureCorners[(i+1)%4].y);
            }
        } else {
            // Draw dragables mask corners
            //
            for(int i = 0; i < maskCorners.size(); i++){
                ofVec3f pos = ofVec3f( maskCorners[i].x * width, maskCorners[i].y * height, 0.0);
                pos = surfaceToScreenMatrix * pos;
                
                if ( (selectedMaskCorner == i) || ( ofDist(ofGetMouseX(), ofGetMouseY(), pos.x, pos.y) <= 4 ) ) {
                    ofSetColor(255,255);
                    ofCircle( pos, 4);
                    ofSetColor(255,100);
                    ofFill();
                } else {
                    ofNoFill();
                    ofSetColor(255,100);
                }
                
                ofCircle( pos, 4);
                
                // Draw contour mask line
                //
                ofSetColor(255,200);
                ofVec3f nextPos = ofVec3f(maskCorners[(i+1)%maskCorners.size()].x*width, 
                                          maskCorners[(i+1)%maskCorners.size()].y*height, 0.0);
                nextPos = surfaceToScreenMatrix * nextPos;
                ofLine(pos.x,pos.y,nextPos.x,nextPos.y);
            }
        }
        
        if (type == "ofShader"){
            if (shader != NULL){
                if ( !(shader->isOk()) ){
                    ofSetColor(0, 100);
                    ofFill();
                    ofRect(box);
                    
                    ofSetColor(255, 0, 0);
                    ofDrawBitmapString("Error", box.x + 5, box.y + 15);
                    ofNoFill();
                    ofRect(box);
                }
                
                // Draw the linking dots
                //
                for(int i = 0; i < inPut.size(); i++){
                    ofSetColor(255, 150);
                    ofNoFill();
                    ofCircle(inPut[i].pos, 5);
                }
            }
        }
        
        // Draw the output linking dot
        //
        ofNoFill();
        ofSetColor(255, 150);
        ofCircle(getOutPutPosition(), 5);
        ofPopStyle();
        
        // Draw the links
        //
        for (int i = 0; i < outPut.size(); i++){
            if (outPut[i].to != NULL){
                ofSetColor(150);
                ofFill();
                ofCircle(outPut[i].pos, 3);
                ofLine(outPut[i].pos, outPut[i].to->pos);
                ofCircle(outPut[i].to->pos, 3);
            }
        }
    }
}

void ofxBasePatch::_mousePressed(ofMouseEventArgs &e){
}

void ofxBasePatch::_mouseDragged(ofMouseEventArgs &e){
}

void ofxBasePatch::_mouseReleased(ofMouseEventArgs &e){
}

void ofxBasePatch::_keyPressed(ofKeyEventArgs &e){   
}

void ofxBasePatch::_reMakeFrame( int &_nId ){
}