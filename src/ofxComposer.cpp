//
//  ofxComposer.cpp
//  GPUBurner
//
//  Created by Patricio Gonzalez Vivo on 3/9/12.
//  Copyright (c) 2012 http://www.PatricioGonzalezVivo.com All rights reserved.
//

#include "ofxComposer.h"
#include "ImageOutput.h"

//  HELP screen -> F1
//
string helpScreen = "\n \
| ofxComposer help\n \
---------------------------------------------------\n \
\n \
- F1:   Turn ON/OFF this help message\n \
- F2:   Surface Edit-Mode on/off\n \
- F3:   Masking-Mode ON/OFF (need Edit-Mode ) \n \
\n \
On mask mode on:\n \
- x: delete mask path point\n \
- r: reset mask path\n \
\n \
- F4:   Reset surface coorners\n \
- F5:   Add ofxGLEditor (temporal!!!) and if have it add ofVideoGrabber (temporal!!!)\n \
- F6:   Add ofShader (temporal!!!)\n \
- F7:   Turn ON/OFF the fullscreen-mode\n \
\n \
Mouse and Coorners: \n \
- Left Button Drag:     coorner proportional scale \n \
- Left Button Drag + R: Rotate Patch\n \
- Middle Button Drag \n \
or \n \
Right Drag + A:       centered proportional scale\n \
- Right Button Drag:    coorner transformation\n ";

ofxComposer::ofxComposer(){
    
    //  Event listeners
    //
    ofAddListener(ofEvents().mouseMoved, this, &ofxComposer::_mouseMoved, COMPOSER_EVENT_PRIORITY);
    ofAddListener(ofEvents().mousePressed, this, &ofxComposer::_mousePressed, COMPOSER_EVENT_PRIORITY);
    ofAddListener(ofEvents().mouseReleased, this, &ofxComposer::_mouseReleased, COMPOSER_EVENT_PRIORITY);
    ofAddListener(ofEvents().keyPressed, this, &ofxComposer::_keyPressed, COMPOSER_EVENT_PRIORITY);
    ofAddListener(ofEvents().windowResized, this, &ofxComposer::_windowResized, COMPOSER_EVENT_PRIORITY);
    
    // scrollbar
    //
    ofAddListener(ofEvents().mouseDragged, this, &ofxComposer::_mouseDragged, COMPOSER_EVENT_PRIORITY);
    
#ifdef USE_OFXGLEDITOR
    editor.setup("menlo.ttf");
    editor.setCurrentEditor(1);
    editorBgColor.set(0,0);
    editorFgColor.set(0,0);
    editorFbo.allocate(ofGetWindowWidth(), ofGetWindowHeight());
    editorFbo.begin();
    ofClear(editorBgColor);
    editorFbo.end();
#endif
    
    //  Default parameters
    //
    configFile = "config.xml";
    selectedDot = -1;
    selectedID = -1;
    bEditMode = true;
    bGLEditorPatch = false;
    bHelp = false;
    
    // zoom & drag
    //
    disabledPatches = false;
    
    // aligned nodes
    //
    verticalAlign1 = 0;
    verticalAlign2 = 0;
    horizontalAlign1 = 0;
    horizontalAlign2 = 0;
    
    // multiple select
    //
    multipleSelectFromX = 0;
    multipleSelectFromY = 0;
}

/* ================================================ */
/*                      LOOPS                       */
/* ================================================ */

void ofxComposer::update(){
    for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
        it->second->update();
    }
    
    if ( (bEditMode) && (selectedID >= 0)){
#ifdef USE_OFXGLEDITOR
        if (patches[selectedID]->getType() == "ofShader"){
            editorBgColor.lerp(ofColor(0,150), 0.01);
            editorFgColor.lerp(ofColor(255,255), 0.1);
        } else {
            editorBgColor.lerp(ofColor(0,0), 0.01);
            editorFgColor.lerp(ofColor(0,0), 0.05);
        }
        
        editorFbo.begin();
        //ofEnableAlphaBlending();
        ofClear(editorBgColor);
        ofDisableBlendMode();
        ofRotate(180, 0, 1, 0);
        ofSetColor(255,255);
        editor.draw();
        editorFbo.end();
#endif
    }
}

//------------------------------------------------------------------
void ofxComposer::customDraw(){
    ofPushView();
    ofPushStyle();
    ofPushMatrix();
    
    ofEnableAlphaBlending();
    
#ifdef USE_OFXGLEDITOR
    //  Draw the GLEditor if it«s not inside a Patch
    //
    if (bEditMode && !bGLEditorPatch){
        ofPushMatrix();
        ofRotate(180, 1, 0, 0);
        ofTranslate(0, -ofGetWindowHeight());
        ofSetColor(editorFgColor);
        editorFbo.draw(0, 0);
        ofPopMatrix();
    }
#endif
    
    //  Draw Patches
    //
    for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
        it->second->customDraw();
    }
    
    if (bEditMode) {
        
        //  Draw active line
        //
        ofVec3f mouse = ofVec3f(ofGetMouseX(), ofGetMouseY(), 0.0)*this->getGlobalTransformMatrix();
        if (selectedDot >= 0){
            ofLine(patches[selectedDot]->getOutPutPosition(), ofPoint(mouse.x, mouse.y));
        }
        
        // aligned nodes
        //
        if (isAnyPatchSelected) {
            ofVec3f scale = ((ofCamera*)this->getParent())->getScale();
            if (verticalAlign1) {
                ofSetColor(255, 208, 111);
                ofLine(verticalAlign1, 0, verticalAlign1, ofGetHeight()*scale.y);
            }
            if (verticalAlign2) {
                ofSetColor(255, 208, 111);
                ofLine(verticalAlign2, 0, verticalAlign2, ofGetHeight()*scale.y);
            }
            if (verticalAlign3) {
                ofSetColor(255, 208, 111);
                ofLine(verticalAlign3, 0, verticalAlign3, ofGetHeight()*scale.y);
            }
            if (horizontalAlign1) {
                ofSetColor(255, 208, 111);
                ofLine(0, horizontalAlign1, ofGetWidth()*scale.x, horizontalAlign1);
            }
            if (horizontalAlign2) {
                ofSetColor(255, 208, 111);
                ofLine(0, horizontalAlign2, ofGetWidth()*scale.x, horizontalAlign2);
            }
            if (horizontalAlign3) {
                ofSetColor(255, 208, 111);
                ofLine(0, horizontalAlign3, ofGetWidth()*scale.x, horizontalAlign3);
            }
        }
        
        //  Draw Help screen
        //
        if (bHelp){
            ofSetColor(255);
            ofDrawBitmapString(helpScreen, 20, ofGetWindowHeight()*0.5- 11.0*15.0);
        }
        
        // multiple select
        //
        ofNoFill();
        ofRect(multipleSelectRectangle);
        
    }
    
    ofDisableBlendMode();
    ofEnableAlphaBlending();
    
    ofPopMatrix();
    ofPopStyle();
    ofPopView();
}

//------------------------------------------------------------------
void ofxComposer::drawInspectorGUIs() {
    
    if (bEditMode) {

        for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
            it->second->drawInspectorGUI();
        }
    }
}

/* ================================================ */
/* ================================================ */



/* ================================================ */
/*                      EVENTS                      */
/* ================================================ */

void ofxComposer::_keyPressed(ofKeyEventArgs &e){
    if (e.key == OF_KEY_F1 ){
        bHelp = !bHelp;
    } else if (e.key == OF_KEY_F2 ){
        bEditMode = !bEditMode;
    } else if ((e.key == OF_KEY_F3 ) || (e.key == OF_KEY_F4 ) ){
        //  Special keys reserved for Patch Events
        //        
    } else if (e.key == OF_KEY_F7){
        ofToggleFullscreen();
        
#ifdef USE_OFXGLEDITOR
        editor.reShape();
        editorFbo.allocate(ofGetWindowWidth(),ofGetWindowHeight());
        editorFbo.begin();
        ofClear(editorBgColor);
        editorFbo.end();
#endif
    } else {
        //  If no special key was pressed and the GLEditor is present pass the key
        //
#ifdef USE_OFXGLEDITOR
        editor.keyPressed(e.key);
        
        if (selectedID >= 0){
            if (patches[selectedID]->getType() == "ofShader"){
                patches[selectedID]->setFrag(editor.getText(1));
                patches[selectedID]->saveSettings();
            }
        }
#endif
        
    }
}

//------------------------------------------------------------------
void ofxComposer::_mouseMoved(ofMouseEventArgs &e){
    
}

//------------------------------------------------------------------
void ofxComposer::activePatch( int _nID ){
    if ( (_nID != -1) && (patches[_nID] != NULL) ){
        selectedID = _nID;
        
        for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
            if (it->first == _nID)
                it->second->bActive = true;
            else
                it->second->bActive = false;
        }
    }
}

//------------------------------------------------------------------
void ofxComposer::_mousePressed(ofMouseEventArgs &e){
    ofVec3f mouse = ofVec3f(e.x, e.y, 0.0)*this->getGlobalTransformMatrix();
    
    // si no estoy clickeando sobre ninguna de las 2 scrollbars, veo que hago
    // si estoy clickeando una de las scrollbars, no tengo que hacer nada aca
    if(!draggingGrip && !draggingHGrip && !canvas->getOtherSelected() &&
       (mouse.x - this->getParent()->getPosition().x > RIGHT_MENU_WIDTH) &&
       (mouse.y - this->getParent()->getPosition().y > MENU_HEIGHT)) {
        
        int idPatchHit = isAnyPatchHit(mouse.x, mouse.y, mouse.z);
        
        // zoom & drag
        //
        if(idPatchHit == -1){
            disabledPatches = true;
            isAnyPatchSelected = false;
            deactivateAllPatches();
        } else {
            disabledPatches = false;
            for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
                if(!patches.find(idPatchHit)->second->bActive){
                    activePatch(idPatchHit);
                    isAnyPatchSelected = true;
                    break;
                }
            }
        }
        
        selectedDot = -1;
        for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
            if ( (it->second->getOutPutPosition().distance(ofPoint(mouse.x, mouse.y)) < 5)
                && (it->second->bEditMode) && !(it->second->bEditMask) ){
                
                selectedDot = it->first;
                it->second->bActive = false;
                selectedID = -1;
            }
        }
        
        if (selectedDot == -1){
            for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
                if ((it->second->bActive) && (it->second->bEditMode) && !(it->second->bEditMask)){
                    selectedID = it->first;
#ifdef USE_OFXGLEDITOR
                    //if (bGLEditorPatch
                    if ((it->second->getType() == "ofShader")){
                        editor.setText(it->second->getFrag(), 1);
                    }
#endif
                }
            }
        }
        
        // multiple select
        if(disabledPatches && e.button == 0 && !canvas->getOtherSelected()){
            multipleSelectFromX = mouse.x;
            multipleSelectFromY = mouse.y;
            multipleSelectRectangle.x = mouse.x;
            multipleSelectRectangle.y = mouse.y;
        }
    }
    
}

//------------------------------------------------------------------
void ofxComposer::_mouseDragged(ofMouseEventArgs &e){
    
    ofVec3f mouse = ofVec3f(e.x, e.y, 0.0)*this->getGlobalTransformMatrix();
    
    // mouse is being drag, and the mouse is not over any patch
    if ( disabledPatches && !draggingGrip && !draggingHGrip && (!isAnyLinkHit())
        && (!canvas->getOtherSelected() && (mouse.x > RIGHT_MENU_WIDTH) && (mouse.y > MENU_HEIGHT)) ) {
        
        // left button -> multiple select
        if(e.button == 0){
            multipleSelectRectangle.width = mouse.x - multipleSelectFromX;
            multipleSelectRectangle.height = mouse.y - multipleSelectFromY;
        }
        
        // right mouse -> zoom in / out
        if(e.button == 2){}
        
    } else {
        int activePatch = isAnyPatchHit(mouse.x, mouse.y, mouse.z);
        
        // If i'm dragging any patch, see if it is aligned with other patches
        if (activePatch != -1) {
            
            ofxPatch* p = patches[activePatch];
            verticalAlign1 = 0;
            verticalAlign2 = 0;
            verticalAlign3 = 0;
            horizontalAlign1 = 0;
            horizontalAlign2 = 0;
            horizontalAlign3 = 0;
            
            for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
                
                if (it->second != p) {
                    if ((int)it->second->getTextureCoorners()[0].x == (int)p->getTextureCoorners()[0].x or
                        (int)it->second->getTextureCoorners()[1].x == (int)p->getTextureCoorners()[0].x) {
                        verticalAlign1 = p->getTextureCoorners()[0].x ;
                    }
                    if ((int)(it->second->getTextureCoorners()[0].x + it->second->getBox().width/2) == (int)(p->getTextureCoorners()[0].x + p->getBox().width/2)) {
                        verticalAlign2 = (p->getTextureCoorners()[0].x + p->getBox().width/2);
                    }
                    if ((int)it->second->getTextureCoorners()[0].x == (int)p->getTextureCoorners()[1].x or
                        (int)it->second->getTextureCoorners()[1].x == (int)p->getTextureCoorners()[1].x ) {
                        verticalAlign3 = p->getTextureCoorners()[1].x;
                    }
                    
                    if ((int)it->second->getTextureCoorners()[1].y == (int)p->getTextureCoorners()[1].y or
                        (int)it->second->getTextureCoorners()[3].y == (int)p->getTextureCoorners()[1].y) {
                        horizontalAlign1 = p->getTextureCoorners()[1].y ;
                    }
                    if ((int)(it->second->getTextureCoorners()[1].y + it->second->getBox().height/2) == (int)(p->getTextureCoorners()[1].y + p->getBox().height/2)) {
                        horizontalAlign2 = (p->getTextureCoorners()[1].y + p->getBox().height/2);
                    }
                    if ((int)it->second->getTextureCoorners()[1].y == (int)p->getTextureCoorners()[3].y or
                        (int)it->second->getTextureCoorners()[3].y == (int)p->getTextureCoorners()[3].y ) {
                        horizontalAlign3 = p->getTextureCoorners()[3].y;
                    }
                }
            }
        }
    }
}

//------------------------------------------------------------------
void ofxComposer::_mouseReleased(ofMouseEventArgs &e){
    ofVec3f mouse = ofVec3f(e.x, e.y, 0.0)*this->getGlobalTransformMatrix();
    
    if (selectedDot != -1){
        
        for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
            
            if ((selectedDot != it->first) &&              // If not him self
                //(it->second->getType() == "ofShader") && // The target it's a shader
                (it->second->bEditMode) &&                 // And we are in editMode and not on maskMode
                !(it->second->bEditMask) ){
                
                for (int j = 0; j < it->second->inPut.size(); j++){
                    
                    // And after checking in each dot of each shader...
                    // ... fin the one where the mouse it«s over
                    //
                    if ( it->second->inPut[j].pos.distance(ofPoint(mouse.x, mouse.y)) < 5){
                        
                        // Once he founds it
                        // make the link and forget the selection
                        //
                        connect( selectedDot , it->first, j, true);
                        selectedDot = -1;
                    }
                }
            }
            // zoom & drag
            //
            it->second->setDisablePatch(false);
        }
        
        // If he release the mouse over nothing it will clear all
        // the connections of that dot.
        //
        if (selectedDot != -1){
            
            for(int i = 0; i < patches[selectedDot]->outPut.size(); i++) {
                patches[patches[selectedDot]->outPut[i].toId]->removeInput(((ImageOutput*)patches[selectedDot])->getId());
                this->updateConnectionsSize(patches[patches[selectedDot]->outPut[i].toId]);
            }
            patches[selectedDot]->outPut.clear();
            selectedDot = -1;
            
        }
    }
    
    // aligned nodes
    //
    verticalAlign1 = 0;
    verticalAlign2 = 0;
    verticalAlign3 = 0;
    horizontalAlign1 = 0;
    horizontalAlign2 = 0;
    horizontalAlign3 = 0;
    isAnyPatchSelected = false;
    
    // multipleSelect
    //
    multipleSelectAndReset();
    
    // zoom & drag
    //
    disabledPatches = false;
}

//------------------------------------------------------------------
void ofxComposer::_windowResized(ofResizeEventArgs &e){
#ifdef USE_OFXGLEDITOR
    editor.reShape();
    editorFbo.allocate(e.width, e.height);
    editorFbo.begin();
    ofClear(editorBgColor);
    editorFbo.end();
#endif
}

/* ================================================ */
/* ================================================ */


/* ================================================ */
/*                     SETTERS                      */
/* ================================================ */

void ofxComposer::setEdit(bool _state){
    bEditMode = _state;
    for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
        it->second->bEditMode = bEditMode;
    }
}

//------------------------------------------------------------------
void ofxComposer::setLinkType (enum nodeLinkType type) {
    for(map<int,ofxPatch*>::iterator it = this->patches.begin(); it != this->patches.end(); it++ ){
        it->second->setLinkType(type);
    }
    this->nodeLinkType = type;
}

//------------------------------------------------------------------
void ofxComposer::setNodesCount(int count) {
    this->nodesCount = count;
}

//------------------------------------------------------------------
void ofxComposer::setDraggingGrip(bool dragging){
    draggingGrip = dragging;
}

//------------------------------------------------------------------
bool ofxComposer::isDraggingHGrip(){
    return draggingHGrip;
}

//------------------------------------------------------------------
void ofxComposer::setDraggingHGrip(bool dragging){
    draggingHGrip = dragging;
}

//------------------------------------------------------------------
void ofxComposer::setMainCanvas(ofxUISuperCanvas* _canvas) {
    this->canvas = _canvas;
}

/* ================================================ */
/* ================================================ */


/* ================================================ */
/*                     GETTERS                      */
/* ================================================ */

bool ofxComposer::getEdit(){
    return this->bEditMode;
}

//------------------------------------------------------------------
map<int,ofxPatch*> ofxComposer::getPatches() {
    return patches;
}

//------------------------------------------------------------------
map<int,ofxPatch*> ofxComposer::getActivePatches() {
    map<int,ofxPatch*> actives;
    for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
        if(it->second->bActive) {
            actives[it->second->getId()] = it->second;
        }
    }
    return actives;
}

//------------------------------------------------------------------
nodeLinkType ofxComposer::getLinkType(){
    return this->nodeLinkType;
}

//------------------------------------------------------------------
int ofxComposer::getNodesCount() {
    return this->nodesCount;
}

//------------------------------------------------------------------
int ofxComposer::getPatchesLowestCoord(){
    int coordMasBaja = 10000;
    for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
        if(coordMasBaja > it->second->getLowestYCoord()){
            coordMasBaja = it->second->getLowestYCoord();
        }
    }
    return coordMasBaja - 20;
}

//------------------------------------------------------------------
int ofxComposer::getPatchesHighestCoord(){
    int coordMasAlta = -1;
    for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
        if(coordMasAlta < it->second->getHighestYCoord()){
            coordMasAlta = it->second->getHighestYCoord();
        }
    }
    return coordMasAlta;
}

//------------------------------------------------------------------
int ofxComposer::getPatchesLeftMostCoord(){
    int coordMasIzq = 10000;
    for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
        if(coordMasIzq > it->second->getLowestXCoord()){
            coordMasIzq = it->second->getLowestXCoord();
        }
    }
    return coordMasIzq;
}

//------------------------------------------------------------------
int ofxComposer::getPatchesRightMostCoord(){
    int coordMasDer = -1;
    for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
        if(coordMasDer < it->second->getHighestXCoord()){
            coordMasDer = it->second->getHighestXCoord();
        }
    }
    return coordMasDer;
}

//------------------------------------------------------------------
bool ofxComposer::isDraggingGrip(){
    return draggingGrip;
}

/* ================================================ */
/* ================================================ */


/* ================================================ */
/*                  OTHER FUNCTIONS                 */
/* ================================================ */

//------------------------------------------------------------------
void ofxComposer::addPatch(ofxPatch *p, ofPoint _position){
    
    if (p->getId() == -1) {
        nodesCount++;
        p->setId(nodesCount);
        p->scale(SCALE_RATIO);
        p->move( _position );
        p->setLinkType(nodeLinkType);
    }
    patches[p->getId()] = p;
    
    p->setMainCanvas(this->canvas);
    p->setParent(*this->getParent());
}

//------------------------------------------------------------------
bool ofxComposer::connect( int _fromID, int _toID, int nTexture, bool addInput_){
    bool connected = false;
    
    if ((_fromID != -1) && (patches[_fromID] != NULL) &&
        (_toID != -1) && (patches[_toID] != NULL) /*&&
        (patches[ _toID ]->getType() == "ofShader") */) {
        
        bool exists = false;
        int i = 0;
            
        while (!exists && i < patches[ _fromID ]->outPut.size()) {
            
            if (patches[ _fromID ]->outPut[i].toId == _toID) {
                
                exists = true;
                patches[ _fromID ]->outPut[i].pos = patches[ _fromID ]->getOutPutPosition();
                patches[ _fromID ]->outPut[i].to = &(patches[ _toID ]->inPut[ nTexture ]);
                patches[ _fromID ]->outPut[i].toShader = patches[ _toID ]->getShader();
                patches[ _fromID ]->outPut[i].nTex = nTexture;
            }
            i++;
        }
            
        if (!exists) {
            LinkDot newDot;
            newDot.pos = patches[ _fromID ]->getOutPutPosition();
            newDot.toId = patches[ _toID ]->getId();
            newDot.to = &(patches[ _toID ]->inPut[ nTexture ]);
            newDot.toShader = patches[ _toID ]->getShader();
            newDot.nTex = nTexture;
            
            patches[ _fromID ]->outPut.push_back( newDot );
        }
            
        if (addInput_) {
            patches[ _toID ]->addInput(patches[ _fromID ]);
            
            this->updateConnectionsSize(patches[ _toID ]);
        }
            
        patches[ _toID ]->setup();
            
        connected = true;
    }
    
    return connected;
}

void ofxComposer::updateConnectionsSize(ofxPatch* patch){
    
    for(int j = 0; j < patch->outPut.size(); j++){
        patches[ patch->outPut[j].toId ]->resetSizeBasedOnInput(patch);
        updateConnectionsSize(patches[ patch->outPut[j].toId ]);
    }
}

// -----------------------------------------------------------
// ------------------------------------------- MULTIPLE SELECT
// -----------------------------------------------------------
void ofxComposer::multipleSelectAndReset(){
    if(disabledPatches){
        for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
            ofRectangle aux = multipleSelectRectangle.getIntersection(it->second->getBox());
            if(aux.getArea() > 0){
                it->second->bActive = true;
            } else{
                it->second->bActive = false;
            }
        }
    }
    
    // rectangle reset
    multipleSelectFromX = 0;
    multipleSelectFromY = 0;
    multipleSelectRectangle.x = 0;
    multipleSelectRectangle.y = 0;
    multipleSelectRectangle.height = 0;
    multipleSelectRectangle.width = 0;
}

//------------------------------------------------------------------
void ofxComposer::deactivateAllPatches(){
    for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
        it->second->bActive = false;
    }
}

//------------------------------------------------------------------
bool ofxComposer::arePatchesDeactivated(){
    return disabledPatches;
}

//------------------------------------------------------------------
void ofxComposer::movePatches(ofVec3f diff){
    for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
        it->second->moveDiff(diff);
    }
}

//------------------------------------------------------------------
void ofxComposer::scalePatches(float yDiff){
    float scale = ZOOM_UNIT + yDiff*ZOOM_SENSITIVITY;
    for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
        it->second->scale(scale);
    }
}

//------------------------------------------------------------------
bool ofxComposer::isAnyLinkHit(){
    for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
        if(it->second->isLinkHit()){
            return true;
        }
    }
    return false;
}

//------------------------------------------------------------------
int ofxComposer::isAnyPatchHit(float x, float y, float z){
    ofPoint *point = new ofPoint(x,y,z);
    int isAnyHit = -1;
    for(map<int,ofxPatch*>::reverse_iterator rit = patches.rbegin(); rit != patches.rend(); rit++ ){
        if (rit->second->isOver(*point)){
            isAnyHit = rit->first;
            break;
        }
    }
    delete point;
    return isAnyHit;
}
