//
//  ofxComposer.h
//  GPUBurner
//
//  Created by Patricio Gonzalez Vivo on 3/9/12.
//  Copyright (c) 2012 http://www.PatricioGonzalezVivo.com All rights reserved.
//

#ifndef OFXCOMPOSER
#define OFXCOMPOSER

#include "ofMain.h"
#include "ofxPatch.h"
#include "enumerations.h"

//  Comment the "define USE_OFXGLEDITOR" if you don't want to use ofxGLEditor
//
//#define USE_OFXGLEDITOR
#ifdef USE_OFXGLEDITOR
#include "ofxGLEditor.h"
#endif

class ofxComposer : public ofNode {
    
public:
    
    ofxComposer();
    
    //** LOOPS **//
    //
    void    update();
    void    customDraw();
    void    drawInspectorGUIs();
    
    
    //** SETTERS **//
    //
    void    setEdit(bool _state);
    void    setMainCanvas(ofxUISuperCanvas* gui);
    void    setLinkType (nodeLinkType type);
    void    setNodesCount(int count);
    
    
    //** GETTERS **//
    //
    map<int,ofxPatch*> getPatches();
    int     getPatchesLowestCoord();
    int     getPatchesHighestCoord();
    int     getPatchesLeftMostCoord();
    int     getPatchesRightMostCoord();
    bool    getEdit();
    nodeLinkType getLinkType();
    int     getNodesCount();
    
    
    //** OTHER FUNCTIONS **//
    //
    void    save(string _fileConfig = "default");
    void    load(string _fileConfig = "default");
    bool    addPatchFromFile(string _filePath, ofPoint _position);
    bool    addPatchWithOutFile(string _type, ofPoint _position);
    void    addPatch(ofxPatch* p, ofPoint _position);
    
    ofxPatch* operator[](int _nID){ if ( (_nID != -1) && (patches[_nID] != NULL) ) return patches[_nID]; };
    int     size(){return patches.size(); };
    
    void    movePatches(ofVec3f diff);
    void    scalePatches(float yDiff);
    
    // scroll
    //
    void    setDraggingGrip(bool dragging);
    void    setDraggingHGrip(bool dragging);
    bool    isDraggingGrip();
    bool    isDraggingHGrip();
    
    void    deactivateAllPatches();
    bool    arePatchesDeactivated();
    
    // snippet
    //
    void    loadSnippet();
    bool    saveSnippet();
    
protected:
    
    bool    connect( int _fromID, int _toID, int _nTexture, bool addInput );
    
    map<int,ofxPatch*>  patches;
    
private:
    
    //** EVENTS **//
    //
    void    _mouseMoved(ofMouseEventArgs &e);
    void    _keyPressed(ofKeyEventArgs &e);
    void    _mousePressed(ofMouseEventArgs &e);
    void    _mouseReleased(ofMouseEventArgs &e);
    void    _windowResized(ofResizeEventArgs &e);
    void    _mouseDragged(ofMouseEventArgs &e);
    
    void    activePatch( int _nID );
    
#ifdef USE_OFXGLEDITOR
    ofxGLEditor editor;
    ofFbo       editorFbo;
    ofColor     editorBgColor;
    ofColor     editorFgColor;
#endif
    
    int     isAnyPatchHit(float x, float y, float z);
    bool    isAnyLinkHit();
    bool    isAnyPatchSelected;
    
    ofxUISuperCanvas* canvas;
    
    bool    disabledPatches;
    
    string  configFile;
    
    bool    bEditMode;
    bool    bGLEditorPatch, bHelp;
    
    // node select, and link vertex selected
    //
    int     selectedDot;
    int     selectedID;
    
    // nodes count
    //
    int     nodesCount;
    
    // node link type
    //
    nodeLinkType nodeLinkType;
    
    // align nodes
    //
    int     verticalAlign1, verticalAlign2, verticalAlign3, horizontalAlign1, horizontalAlign2, horizontalAlign3;
    
    // snippet
    //
    int     getMaxIdPatch();
    
    // multiple select
    //
    ofRectangle multipleSelectRectangle;
    int     multipleSelectFromX;
    int     multipleSelectFromY;
    void    multipleSelectAndReset();
    
    // scroll
    //
    bool draggingGrip;
    bool draggingHGrip;
};


#endif
