//
//  ofxTitleBar.cpp
//  GPUBurner
//
//  Created by Patricio Gonzalez Vivo on 3/9/12.
//  Copyright (c) 2012 http://www.PatricioGonzalezVivo.com All rights reserved.
//

#include "ofxTitleBar.h"

ofxTitleBar::ofxTitleBar( ofRectangle* wBox, int* _windowsId ){
    ofAddListener(ofEvents().mousePressed, this, &ofxTitleBar::_mousePressed, PATCH_EVENT_PRIORITY);
    
    windowsBox = wBox;
    windowsId = _windowsId;
    height = 19;
    letterWidth = 10;
    letterHeight = 12;
    offSetWidth = 5;
    
    addButton( 'x', NULL, PUSH_BUTTON);
    addButton( 'r', NULL, PUSH_BUTTON);
};

ofxTitleBar::~ofxTitleBar(){
    
    ofRemoveListener(ofEvents().mousePressed, this, &ofxTitleBar::_mousePressed, PATCH_EVENT_PRIORITY);
}

void ofxTitleBar::addButton( char letter, bool *variableToControl, ButtonType _type){
    ofxTitleBarButton newButton;
    newButton.letter = letter;
    newButton.state = variableToControl;
    newButton.type = _type;
    buttons.push_back( newButton );
}

void ofxTitleBar::removeButton( char letter_){
    
    int i = 0;
    
    while (i < buttons.size()) {
        if(buttons.at(i).letter == letter_) {
            buttons.erase(buttons.begin() + i);
            i = buttons.size();
        }
        i++;
    }
}

void ofxTitleBar::customDraw(){
    
    ofVec3f scale = ((ofCamera*)this->getParent())->getScale();
    ofVec3f cam_pos = ((ofCamera*)this->getParent())->getPosition();
    
    // Update the information of the position
    //
    tittleBox.width = windowsBox->width/scale.z;
    tittleBox.height = height;
//    tittleBox.x = windowsBox->x;
//    tittleBox.y = windowsBox->y - height;
    tittleBox.setPosition(ofVec3f(windowsBox->x/scale.x, (windowsBox->y - height*scale.z)/scale.y, cam_pos.z/scale.z));
    
    ofPushStyle();
    
    // Draw the Bar
    //
    ofFill();
    ofSetColor(0,50);
    ofRect(tittleBox);
    
    // Draw the tittle
    //
    ofFill();
    ofSetColor(100);
    (tittleBox.width - title.size() * 8) > 57
        ? ofDrawBitmapString(title, tittleBox.x -1 + tittleBox.width - title.size() * 8, tittleBox.y + letterHeight)
        : ofDrawBitmapString("...", tittleBox.x -1 + tittleBox.width - 24, tittleBox.y + letterHeight);
    
    // Draw the bottoms
    //
    string buttonString;
    for (int i = 0; i < buttons.size(); i++){
        ofSetColor(100);
        if ( buttons[i].state != NULL){
            if ((*buttons[i].state) == true)
                ofSetColor(255);
        }
        
        ofDrawBitmapString( ofToString(buttons[i].letter) , tittleBox.x + offSetWidth + i*letterWidth, tittleBox.y + letterHeight);
    }
    ofPopStyle();
}

void ofxTitleBar::_mousePressed(ofMouseEventArgs &e){
    ofPoint mouse = ofPoint(e.x, e.y);
    ofVec3f mouse_transformed = mouse*this->getGlobalTransformMatrix();
    
    if ( tittleBox.inside(mouse)){
        bool hit = false;
        for (int i = 0; i < buttons.size() && !hit; i++){
            if (((mouse.x - tittleBox.x - offSetWidth) > i * letterWidth ) &&
                ((mouse.x - tittleBox.x - offSetWidth) < (i+1) * letterWidth ) ){
                if ( buttons[i].letter == 'x' ){
                    ofNotifyEvent(close, *windowsId);
                    hit = true;
                } else if ( buttons[i].letter == 'r' ){
                    ofNotifyEvent(reset, *windowsId);
                    hit = true;
                } else {
                    if ( buttons[i].state != NULL ){
                        (*buttons[i].state) = !(*buttons[i].state);
                        hit = true;
                    }
                }
            }
        }
    }
};

void ofxTitleBar::_mouseReleased(ofMouseEventArgs &e){
    ofPoint mouse = ofPoint(e.x, e.y);
    ofVec3f mouse_transformed = mouse*this->getGlobalTransformMatrix();
    if ( tittleBox.inside(mouse_transformed)){
        ofNotifyEvent(drag, *windowsId);
    }
}