//
//  ofxTitleBar.h
//  GPUBurner
//
//  Created by Patricio Gonzalez Vivo on 3/9/12.
//  Copyright (c) 2012 http://www.PatricioGonzalezVivo.com All rights reserved.
//

#ifndef OFXTITLEBAR
#define OFXTITLEBAR

#include "ofMain.h"
#include "enumerations.h"

enum ButtonType {
    TOGGLE_BUTTON,
    PUSH_BUTTON
};

struct ofxTitleBarButton{
    char letter;
    bool *state;
    ButtonType type;
};

class ofxTitleBar : public ofNode {
public:
    
    ofxTitleBar( ofRectangle* wBox, int* _windowsId );
    ~ofxTitleBar();
    
    void setTitle(string _title){ title = _title; };
    ofRectangle getTittleBox() { return tittleBox; };
    
    void addButton( char letter, bool *variableToControl, ButtonType _type);
    void removeButton( char letter);
    
    void customDraw();
    
    ofEvent<int> close;
    ofEvent<int> reset;
    ofEvent<int> drag;
    
private:
    void _mouseReleased(ofMouseEventArgs &e);
    bool _mousePressed(ofMouseEventArgs &e);
    
    ofRectangle    tittleBox;
    ofRectangle    *windowsBox;
    string         title;
    vector<ofxTitleBarButton> buttons;
    
    int             *windowsId;
    int             letterWidth, letterHeight, offSetWidth;
    int             height;
};


#endif
