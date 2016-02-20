//
//  ofxPatch.h
//  emptyExample
//
//  Created by Patricio Gonzalez Vivo on 3/9/12.
//  Copyright (c) 2012 http://www.PatricioGonzalezVivo.com All rights reserved.
//

#ifndef OFXPATCH
#define OFXPATCH

#include "ofMain.h"

#include "ofxXmlSettings.h"

#include "ofxTitleBar.h"
#include "ofxShaderObj.h"
#include "ofxPingPong.h"
#include "enumerations.h"
#include "ofxPanel.h"

struct LinkDot{
    LinkDot(){
        to = NULL;
        toShader = NULL;
        nTex = 0;
    }
    
    ofPoint     pos;
    LinkDot     *to;
    int         nTex;
    int         toId;
    ofxShaderObj *toShader;
    vector<ofPoint> link_vertices;   // vertices in the nodes links
    ofPolyline  link_line;
    LinkDot     *toEncapsulated;
    int         toEncapsulatedId;
};

class ofxPatch : public ofNode {
    
public:
    
    ofxPatch();
    ~ofxPatch();
    
    virtual void    setup(){};
    
    //** LOOPS **//
    //
    void            update();
    virtual void    customDraw();
    void            drawInspectorGUI();
    
    
    //** EVENTS **//
    //
    //void            guiEvent(ofxUIEventArgs &e);
    
    
    //** SETTERS **//
    //
    void            setId(int id_) { nId = id_; };
    void            setFrag(string _code);
    //void          setVert(string _code);
    void            setMask(ofPolyline& _polyLine){ maskCorners = _polyLine; bMasking = true; bUpdateMask = true; };
    void            setCoorners(ofPoint _coorners[4]);
    void            setCoorners(vector<ofPoint> _coorners);
    void            setTexture(ofTexture& tex, int _texNum = 0);
    void            setDisablePatch(bool disable);
    void            setLinkHit(bool linkHit);
    void            setLinkType(nodeLinkType type);
    void            setDrawInspector(bool draw_);
    
    
    //** GETTERS **//
    //
    int             getId() const { return nId; };
    ofPoint         getPos() const { return ofPoint(x,y); };
    string          getType() const { return (shader != NULL)? "ofShader" : type; };
    ofPoint         getSurfaceToScreen(ofPoint _pos){ return surfaceToScreenMatrix * _pos; };
    ofPoint         getScreenToSurface(ofPoint _pos){ return screenToSurfaceMatrix * _pos; };
    GLfloat*        getGlMatrix() { return glMatrix; };
    string          getFrag();
    //string        getVert();
    ofTexture&      getTextureReference();
    ofxShaderObj*   getShader(){ if (getType() == "ofShader") return shader; else return NULL; };
    ofPoint&        getOutPutPosition(){ return outPutPos; };
    ofPolyline      getTextureCoorners();
    ofRectangle     getBox() { return box; };
    float           getHeight();
    float           getWidth();
    bool            drawInspector();
    float           getHighestInspectorYCoord(int winId = MAIN_WINDOW);
    float           getHighestInspectorXCoord(int winId = MAIN_WINDOW);
    bool            getIsAudio(){ return isAudio; };
    
    // when dragging nodes
    //
    float           getHighestYCoord(int winId = MAIN_WINDOW);
    float           getLowestYCoord(int winId = MAIN_WINDOW);
    float           getHighestXCoord(int winId = MAIN_WINDOW);
    float           getLowestXCoord(int winId = MAIN_WINDOW);
    
    
    //** OTHER FUNCTIONS **//
    //
    void            move(ofPoint _pos);
    void            scale(float _scale);
    void            rotate(float _angle);

    bool            loadSettings(ofxXmlSettings &XML, int nTag_, int nodesCount_ = 0);
    bool            saveSettings(ofxXmlSettings &XML, bool _new, int _nTag);
    bool            saveSettingsToSnippet(ofxXmlSettings &XML, int _nTag, map<int,int> newIdsMap);
    
    bool            isOver(ofPoint _pos); // is mouse over patch ?
    void            moveDiff(ofVec2f diff); // move [diff] when scrolling
    bool            isLinkHit(); // is node link hit my mouse click ?
    virtual void    resetSize(int _width = 0, int _height = 0);
    virtual void    resetSizeBasedOnInput(ofxPatch* input_);
    void            resetSizeToNoInputs();
    
    // methods for adding input
    //
    void            addInputDot();
    virtual bool    addInput(ofxPatch* layer_){};
    virtual void    removeInput(int inputId_){};
    
    
    //** PUBLIC ATTRIBUTES **//
    //
    vector<LinkDot> outPut;
    vector<LinkDot> inPut;
    
    ofxTitleBar     *title;
    
    bool            bActive;
    bool            bEditMode;
    bool            bEditMask;
    bool            bVisible;
    
    bool            disabledPatch; // disable patches when zooming & scrolling
    
    
    // encapsulated
    int             getWindowId();
    int             getEncapsulatedId();
    int             getToEncapsulatedId();
    bool            isLastEncapsulated();
    void            setWindowId(int winId);
    void            setEncapsulatedId(int encapId);
    void            setLastEncapsulated(bool last);
    void            setToEncapsulatedId(int encapId);
    virtual string  getName() {};
    
    // MIDI learn
    void setMidiLearnActive(bool active_);
    
    // Audio in
    void setEditAudioInActive(bool active_);
    
protected:
    
    // Mouse & Key Events ( it´s not better if is centralized on the composer )
    //
    void            _mousePressed(ofMouseEventArgs &e);
    virtual void    _mouseDragged(ofMouseEventArgs &e);
    void            _mouseReleased(ofMouseEventArgs &e);
    void            _keyPressed(ofKeyEventArgs &e);
    void            _reMakeFrame( int &_nId );
    
    ofVideoGrabber  *videoGrabber;
    ofxShaderObj    *shader;
    ofTexture       tex;
    ofFbo           fbo;
    ofImage         noInputs;
    
    bool            drawNoInputs;
    bool            isAudio;
    
    // Mask variables
    //
    ofxPingPong     maskFbo;
    ofShader        maskShader;
    ofPolyline      maskCorners;
    int             selectedMaskCorner;
    
    // Texture varialbes
    //
    ofPolyline      textureCorners;
    int             selectedTextureCorner;
    int             textureWidth, textureHeight;
    
    // General Variables
    //
    ofRectangle     box;
    ofColor         color;
    ofPoint         outPutPos;
    string          configFile;
    string          filePath;
    string          type;
    float           x, y;
    float           width, height;
    float           texOpacity, maskOpacity;
    int             nId;
    
    bool            bMasking;
    bool            bUpdateMask;
    bool            bUpdateCoord;
    
    GLfloat         glMatrix[16];
    
    nodeType        nodeType;
    
    // Inspector
    //
    ofxPanel        panel;
    ofxGuiGroup     gui;
    bool            bInspector;
    
    // MIDI learn
    //
    bool midiLearnActive;
    
    // Audio In
    bool editAudioInActive;
    
private:
    
    void            doSurfaceToScreenMatrix();      // Update the SurfaceToScreen transformation matrix
    void            doScreenToSurfaceMatrix();      // Update the ScreenToSurface transformation matrix
    void            doGaussianElimination(float *input, int n); // This is used making the matrix
    
    virtual ofTexture* getTexture(){};
    
    bool            is_between(float x, float bound1, float bound2, float tolerance); // Is mouse click between link vertices ?

    ofPoint         src[4];
    ofMatrix4x4     surfaceToScreenMatrix;
    ofMatrix4x4     screenToSurfaceMatrix;
    
    // Node links Variables
    //
    int             selectedLinkVertex;
    int             selectedLink;
    nodeLinkType    linkType;
    bool            linkHit;
    
    // multiple Window - encapsulated
    int             windowId;
    bool            lastEncapsulated;
    int             encapsulatedId;
    
};

#endif
