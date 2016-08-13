//
//  ofxComposer.cpp
//  GPUBurner
//
//  Created by Patricio Gonzalez Vivo on 3/9/12.
//  Copyright (c) 2012 http://www.PatricioGonzalezVivo.com All rights reserved.
//

#include "ofxComposer.h"
#include "ImageOutput.h"
#include "ConsoleLog.h"
#include "EventHandler.h"
#include "AudioAnalizer.h"

ofxComposer::ofxComposer(){
    
    //  Event listeners
    //
    ofAddListener(ofEvents().mouseMoved, this, &ofxComposer::_mouseMoved, COMPOSER_EVENT_PRIORITY);
    ofAddListener(ofEvents().mousePressed, this, &ofxComposer::_mousePressed, COMPOSER_EVENT_PRIORITY);
    ofAddListener(ofEvents().mouseReleased, this, &ofxComposer::_mouseReleased, COMPOSER_EVENT_PRIORITY);
    ofAddListener(ofEvents().keyPressed, this, &ofxComposer::_keyPressed, COMPOSER_EVENT_PRIORITY);
    ofAddListener(ofEvents().keyReleased, this, &ofxComposer::_keyReleased, COMPOSER_EVENT_PRIORITY);
    ofAddListener(ofEvents().windowResized, this, &ofxComposer::_windowResized, COMPOSER_EVENT_PRIORITY);
    
    // scrollbar
    //
    ofAddListener(ofEvents().mouseDragged, this, &ofxComposer::_mouseDragged, COMPOSER_EVENT_PRIORITY);
    
    //  Default parameters
    //
    configFile = "config.xml";
    selectedDot = -1;
    selectedID = -1;
    bEditMode = true;
    
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
    holdingCommand = false;
    
    // MIDI learn
    //
    midiLearnActive = false;
    
    // Audio In
    //
    editAudioInActive = false;
    
    // OSC
    //
    editOSCActive = false;
}

ofxComposer::~ofxComposer(){
    ofRemoveListener(ofEvents().mouseMoved, this, &ofxComposer::_mouseMoved, COMPOSER_EVENT_PRIORITY);
    ofRemoveListener(ofEvents().mousePressed, this, &ofxComposer::_mousePressed, COMPOSER_EVENT_PRIORITY);
    ofRemoveListener(ofEvents().mouseReleased, this, &ofxComposer::_mouseReleased, COMPOSER_EVENT_PRIORITY);
    ofRemoveListener(ofEvents().keyPressed, this, &ofxComposer::_keyPressed, COMPOSER_EVENT_PRIORITY);
    ofRemoveListener(ofEvents().keyReleased, this, &ofxComposer::_keyReleased, COMPOSER_EVENT_PRIORITY);
    ofRemoveListener(ofEvents().windowResized, this, &ofxComposer::_windowResized, COMPOSER_EVENT_PRIORITY);
    
    // scrollbar
    //
    ofRemoveListener(ofEvents().mouseDragged, this, &ofxComposer::_mouseDragged, COMPOSER_EVENT_PRIORITY);
}

/* ================================================ */
/*                      LOOPS                       */
/* ================================================ */

void ofxComposer::update(){
    for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
        it->second->update();
    }
}

//------------------------------------------------------------------
void ofxComposer::customDraw(){

    ofPushStyle();
    
    ofEnableAlphaBlending();
    
    //  Draw Patches
    //
    for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
        it->second->customDraw();
    }
    
    if (bEditMode) {
        
        ofVec3f scale = ((ofCamera*)this->getParent())->getScale();
        ofVec3f cam_pos = ((ofCamera*)this->getParent())->getPosition();
        
        //  Draw active line
        //
        ofVec3f mouse = ofVec3f(ofGetMouseX(), ofGetMouseY(), 0.0);
        ofVec3f mouse_transformed = mouse*this->getGlobalTransformMatrix();
        if (selectedDot >= 0){
            ofLine((patches[selectedDot]->getOutPutPosition()-ofPoint(cam_pos.x, cam_pos.y))/ofPoint(scale.x, scale.y), ofPoint(mouse.x, mouse.y));
        }
        
        // aligned nodes
        //
        if (isAnyPatchSelected) {

            if (verticalAlign1) {
                ofSetColor(255, 208, 111);
                ofLine(verticalAlign1, 0, verticalAlign1, ofGetHeight());
            }
            if (verticalAlign2) {
                ofSetColor(255, 208, 111);
                ofLine(verticalAlign2, 0, verticalAlign2, ofGetHeight());
            }
            if (verticalAlign3) {
                ofSetColor(255, 208, 111);
                ofLine(verticalAlign3, 0, verticalAlign3, ofGetHeight());
            }
            if (horizontalAlign1) {
                ofSetColor(255, 208, 111);
                ofLine(0, horizontalAlign1, ofGetWidth(), horizontalAlign1);
            }
            if (horizontalAlign2) {
                ofSetColor(255, 208, 111);
                ofLine(0, horizontalAlign2, ofGetWidth(), horizontalAlign2);
            }
            if (horizontalAlign3) {
                ofSetColor(255, 208, 111);
                ofLine(0, horizontalAlign3, ofGetWidth(), horizontalAlign3);
            }
        }
        
        // multiple select
        //
        ofNoFill();
        ofRect(multipleSelectRectangle);
        
    }
    
    ofDisableBlendMode();
    ofEnableAlphaBlending();
    
    ofPopStyle();
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
//    if(!EventHandler::getInstance()->isMainEvent()){
//        return;
//    }
    
    if (e.key == OF_KEY_F2 ){
        bEditMode = !bEditMode;
    } else if ((e.key == OF_KEY_F3 ) || (e.key == OF_KEY_F4 ) ){
        //  Special keys reserved for Patch Events
        //        
    } else if (e.key == OF_KEY_F7){
        ofToggleFullscreen();
    } else if (e.key == OF_KEY_LEFT_COMMAND || e.key == OF_KEY_RIGHT_COMMAND){
        holdingCommand = true;
    }
}

//------------------------------------------------------------------
void ofxComposer::_keyReleased(ofKeyEventArgs &e){
    holdingCommand = false;
}

//------------------------------------------------------------------
void ofxComposer::activePatch( int _nID ){
    if ( (_nID != -1) && (patches[_nID] != NULL) ){
        selectedID = _nID;
        
        for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
            if (it->first == _nID){
                it->second->bActive = true;
            } else if(!holdingCommand){
                it->second->bActive = false;   
            }
        }
    }
}

//------------------------------------------------------------------
void ofxComposer::_mouseMoved(ofMouseEventArgs &e){
    
}

//------------------------------------------------------------------
void ofxComposer::_mousePressed(ofMouseEventArgs &e){
    // TODO: ver si esto aplica para cuando este la consola
//    if(!EventHandler::getInstance()->isMainEvent()){
//        return;
//    }
    
    ofVec3f mouse = ofVec3f(e.x, e.y, 0.0);
    ofVec3f mouse_transformed = mouse*this->getGlobalTransformMatrix();
    
    ofVec3f scale = ((ofCamera*)this->getParent())->getScale();
    
    int idPatchHit = -1;
    ofxPatch* activePatch;
    
    // si no estoy clickeando sobre ninguna de las 2 scrollbars, veo que hago
    // si estoy clickeando una de las scrollbars, no tengo que hacer nada aca
    if(!draggingGrip && !draggingHGrip) {
        
        idPatchHit = isAnyPatchHit(mouse_transformed.x, mouse_transformed.y, mouse_transformed.z);
        
        // zoom & drag
        //
        if(idPatchHit == -1 && !holdingCommand){
            disabledPatches = true;
            isAnyPatchSelected = false;
            deactivateAllPatches();
        } else if (idPatchHit != -1){
            disabledPatches = false;
            if(!patches.find(idPatchHit)->second->bActive){
                this->activePatch(idPatchHit);
                isAnyPatchSelected = true;
                //break;
            } else if(holdingCommand){
                patches.find(idPatchHit)->second->bActive = false;
            }
        }
        
        selectedDot = -1;
        for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
            if ( (it->second->getOutPutPosition().distance(ofPoint(mouse_transformed.x, mouse_transformed.y)) < (5*scale.x))
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
                }
            }
        }
        
        // multiple select
        //
        if(disabledPatches && e.button == 0){
            multipleSelectFromX = mouse.x;
            multipleSelectFromY = mouse.y;
            multipleSelectRectangle.x = mouse.x;
            multipleSelectRectangle.y = mouse.y;
        }
    }
}

//------------------------------------------------------------------
void ofxComposer::_mouseDragged(ofMouseEventArgs &e){
//    if(!EventHandler::getInstance()->isMainEvent()){
//        return;
//    }
    
    ofVec3f mouse = ofVec3f(e.x, e.y, 0.0);
    ofVec3f mouse_transformed = mouse*this->getGlobalTransformMatrix();
    
    // mouse is being drag, and the mouse is not over any patch
    if ( disabledPatches && !draggingGrip && !draggingHGrip && (!isAnyLinkHit()) ) {
        
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
            ofRectangle aux_box;
            ofRectangle p_box = p->getBox();
            verticalAlign1 = 0;
            verticalAlign2 = 0;
            verticalAlign3 = 0;
            horizontalAlign1 = 0;
            horizontalAlign2 = 0;
            horizontalAlign3 = 0;
            
            for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
                
                aux_box = it->second->getBox();
                
                if (it->second != p) {
                    if ((int)aux_box.x == (int)p_box.x or
                        (int)(aux_box.x + aux_box.width) == (int)p_box.x) {
                        verticalAlign1 = p_box.x ;
                    }
                    if ((int)(aux_box.x + aux_box.width/2) == (int)(p_box.x + p_box.width/2)) {
                        verticalAlign2 = (p_box.x + p_box.width/2);
                    }
                    if ((int)aux_box.x == (int)(p_box.x + p_box.width) or
                        (int)(aux_box.x + aux_box.width) == (int)(p_box.x + p_box.width) ) {
                        verticalAlign3 = p_box.x + p_box.width;
                    }
                    
                    if ((int)(aux_box.y + aux_box.height) == (int)(p_box.y + p_box.height) or
                        (int)aux_box.y == (int)(p_box.y + p_box.height)) {
                        horizontalAlign1 = p_box.y + p_box.height;
                    }
                    if ((int)(aux_box.y + aux_box.height/2) == (int)(p_box.y + p_box.height/2)) {
                        horizontalAlign2 = (p_box.y + p->getBox().height/2);
                    }
                    if ((int)(aux_box.y + aux_box.height) == (int)p_box.y or
                        (int)(aux_box.y == (int) p_box.y) ) {
                        horizontalAlign3 =  p_box.y;
                    }
                }
            }
        }
    }
}

//------------------------------------------------------------------
void ofxComposer::_mouseReleased(ofMouseEventArgs &e){
//    if(!EventHandler::getInstance()->isMainEvent()){
//        return;
//    }
    
    ofVec3f mouse = ofVec3f(e.x, e.y, 0.0);
    ofVec3f mouse_transformed = mouse*this->getGlobalTransformMatrix();
    
    if (selectedDot != -1){
        
        for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
            
            if ((selectedDot != it->first) &&              // If not him self
                //(it->second->getType() == "ofShader") && // The target it's a shader
                (it->second->bEditMode) &&                 // And we are in editMode and not on maskMode
                !(it->second->bEditMask)){                 // And the node acepts more inputs
                
                for (int j = 0; j < it->second->inPut.size(); j++){
                    
                    // And after checking in each dot of each shader...
                    // ... fin the one where the mouse it�s over
                    //
                    if ( it->second->inPut[j].pos.distance(ofPoint(mouse_transformed.x, mouse_transformed.y)) < 5 || it->second->isOver(mouse_transformed)){
                        if ((!it->second->isLastEncapsulated() ||
                            (it->second->isLastEncapsulated() && !(EventHandler::getInstance()->getEncapsulatedIdDraw() == MAIN_WINDOW))) &&
                            it->second->aceptsMoreInputs()){
                            
                            // Once he founds it
                            // make the link and forget the selection
                            //
                            connect( selectedDot , it->first, j, true);
                            selectedDot = -1;
                        }
                    }
                }
            }
            // zoom & drag
            //
            it->second->setDisablePatch(false);
        }
        
        // If he release the mouse over nothing
        // release the selected dot.
        //
        selectedDot = -1;
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
        if (it->second->isLastEncapsulated()) {
            for(map<int,ofxPatch*>::iterator it2 = patches.begin(); it2 != patches.end(); it2++ ){
                if(!it2->second->isLastEncapsulated() && it2->second->getEncapsulatedId() == it->second->getEncapsulatedId()) {
                    actives[it2->second->getId()] = it2->second;
                }
            }
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
int ofxComposer::getPatchesLowestCoord(int encapsulatedDrawId){
    int coordMasBaja = 10000;
    for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
        if(!it->second->getIsAudioAnalizer() || (it->second->getIsAudioAnalizer() && it->second->getDrawAudioAnalizer())){
            if(coordMasBaja > it->second->getLowestYCoord(encapsulatedDrawId)){
                coordMasBaja = it->second->getLowestYCoord(encapsulatedDrawId);
            }
        }
    }
    return coordMasBaja - 20;
}

//------------------------------------------------------------------
int ofxComposer::getPatchesHighestCoord(int encapsulatedDrawId){
    int coordMasAlta = -1;
    for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
        if(!it->second->getIsAudioAnalizer() || (it->second->getIsAudioAnalizer() && it->second->getDrawAudioAnalizer())){
            if(coordMasAlta < it->second->getHighestYCoord(encapsulatedDrawId)){
                coordMasAlta = it->second->getHighestYCoord(encapsulatedDrawId);
            }
        }
    }
    return coordMasAlta;
}

//------------------------------------------------------------------
int ofxComposer::getPatchesLeftMostCoord(int encapsulatedDrawId){
    int coordMasIzq = 10000;
    for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
        if(!it->second->getIsAudioAnalizer() || (it->second->getIsAudioAnalizer() && it->second->getDrawAudioAnalizer())){
            if(coordMasIzq > it->second->getLowestXCoord(encapsulatedDrawId)){
                coordMasIzq = it->second->getLowestXCoord(encapsulatedDrawId);
            }
        }
    }
    return coordMasIzq;
}

//------------------------------------------------------------------
int ofxComposer::getPatchesRightMostCoord(int encapsulatedDrawId){
    int coordMasDer = -1;
    for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
        if(!it->second->getIsAudioAnalizer() || (it->second->getIsAudioAnalizer() && it->second->getDrawAudioAnalizer())){
            if(coordMasDer < it->second->getHighestXCoord(encapsulatedDrawId)){
                coordMasDer = it->second->getHighestXCoord(encapsulatedDrawId);
            }
        }
    }
    return coordMasDer;
}

int ofxComposer::getPatchesHighestYInspectorCoord(int encapsulatedDrawId){
    int highestY = -1;
    for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
        if(!it->second->getIsAudioAnalizer() || (it->second->getIsAudioAnalizer() && it->second->getDrawAudioAnalizer())){
            if(highestY < it->second->getHighestInspectorYCoord(encapsulatedDrawId)){
                highestY = it->second->getHighestInspectorYCoord(encapsulatedDrawId);
            }
        }
    }
    return highestY;
}

int ofxComposer::getPatchesHighestXInspectorCoord(int encapsulatedDrawId){
    int highestX = -1;
    for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
        if(!it->second->getIsAudioAnalizer() || (it->second->getIsAudioAnalizer() && it->second->getDrawAudioAnalizer())){
            if(highestX < it->second->getHighestInspectorXCoord(encapsulatedDrawId)){
                highestX = it->second->getHighestInspectorXCoord(encapsulatedDrawId);
            }
        }
    }
    return highestX;
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
    p->setParent(*this->getParent());
    ofAddListener(p->deletePatchConection , this, &ofxComposer::deletePatchConection);
    
    patches[p->getId()] = p;
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
            newDot.toEncapsulatedId = -1;
            
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

//------------------------------------------------------------------
void ofxComposer::updateConnectionsSize(ofxPatch* patch){
    
    for(int j = 0; j < patch->outPut.size(); j++){
        patches[ patch->outPut[j].toId ]->resetSizeBasedOnInput(patch);
        updateConnectionsSize(patches[ patch->outPut[j].toId ]);
    }
}

//------------------------------------------------------------------
void ofxComposer::deletePatchConection(ofxPatchDeleteEvent &ev){

    patches[patches[ev.patchId]->outPut[ev.deleteOutputId].toId]->removeInput(((ImageOutput*)patches[ev.patchId])->getId());
    this->updateConnectionsSize(patches[patches[ev.patchId]->outPut[ev.deleteOutputId].toId]);
    
    patches[ev.patchId]->outPut.erase(patches[ev.patchId]->outPut.begin() + ev.deleteOutputId);
}

//------------------------------------------------------------------
void ofxComposer::setDrawInspectors(bool drawInspectors_){
    
    for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
        it->second->setDrawInspector(drawInspectors_);
    }
}

// -----------------------------------------------------------
// ------------------------------------------- MULTIPLE SELECT
// -----------------------------------------------------------
void ofxComposer::multipleSelectAndReset(){
    if(disabledPatches){
        
        ofVec3f scale = ((ofCamera*)this->getParent())->getScale();
        
        for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
            
            // if it is invisible, don't make it count, it is encapsulated
            if( ((!it->second->bEditMode && it->second->bVisible) || it->second->bEditMode) && (!it->second->getIsAudioAnalizer() || (it->second->getIsAudioAnalizer() && it->second->getDrawAudioAnalizer()))){
                ofRectangle aux = multipleSelectRectangle.getIntersection(it->second->getBox());
                
                if(aux.getArea() > 0){
                    it->second->bActive = true;
                } else{
                    it->second->bActive = false;
                }
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
void ofxComposer::activateAllPatches(){
    for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
        it->second->bActive = true;
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


// -------------------------------------------------------------
// ------------------------------------------------- ENCAPSULATE
// -------------------------------------------------------------

int ofxComposer::encapsulate(){
    vector<int> patchesToEncapsulate;
    patchesToEncapsulate.clear();
    int lastPatch = this->validateEncapsulation(patchesToEncapsulate);
    if(lastPatch > 0) {
        nodesCount++;
        int encapsulatedId = getNodesCount();
        for(vector<int>::iterator it = patchesToEncapsulate.begin(); it != patchesToEncapsulate.end(); it++ ){
            if(patches.at(*it)->getId() != lastPatch){
//                patches.at(*it)->setWindowId(-1);
            }
            patches.at(*it)->setEncapsulatedId(encapsulatedId);
            patches.at(*it)->setLastEncapsulated(patches.at(*it)->getId() == lastPatch);
            patches.at(*it)->setToEncapsulatedId(lastPatch);
        }
        setOutputEncapsulated(lastPatch, patchesToEncapsulate);
        deactivateAllPatches();
        return encapsulatedId;
    } else {
        ConsoleLog::getInstance()->pushError("The selection of nodes to encapsulate is invalid.");
        return -1;
    }
}

//------------------------------------------------------------------
void ofxComposer::uncapsulate(int encapsulatedId){
    for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
        if(it->second->getEncapsulatedId() == encapsulatedId){
//            it->second->setWindowId(MAIN_WINDOW);
            it->second->setEncapsulatedId(MAIN_WINDOW);
            it->second->setLastEncapsulated(false);
            it->second->setToEncapsulatedId(-1);
        }
    }
    nodesCount--;
}

//------------------------------------------------------------------
int ofxComposer::validateEncapsulation(vector<int> &patchesToEncapsulate){
    int patchId = -1;
    int validOutput = 0;
    
    // load vector with all selected patches
    for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
        if(it->second->bActive){
            if(it->second->getEncapsulatedId() > 0){
                ConsoleLog::getInstance()->pushError("There is an encapsulated node selected");
                patchesToEncapsulate.clear();
                return -1;
            }
            
            if(it->second->getIsAudio()){
                if(dynamic_cast<AudioAnalizer*>(it->second) != NULL){
                    if(it->second->getDrawAudioAnalizer()){
                        ConsoleLog::getInstance()->pushWarning("An audio node was selected. It can't be encapsulated.");
                    }
                } else {
                    ConsoleLog::getInstance()->pushWarning("An audio node was selected. It can't be encapsulated.");
                }
            }else{
                patchesToEncapsulate.push_back(it->second->getId());
            }
        }
    }
    

    if(patchesToEncapsulate.size() > 1){
        // validate that every output is goint to an encapsulated node, except one
        for(vector<int>::iterator it = patchesToEncapsulate.begin(); it != patchesToEncapsulate.end(); it++ ){
            vector<LinkDot> output = patches.at(*it)->outPut;
            if(output.size() == 0){
                patchId = *it;
                validOutput++;
            }
            for(vector<LinkDot>::iterator it2 = output.begin(); it2 != output.end(); it2++ ){
                // if the output of a node is not found in the selected nodes
                bool found = false;
                vector<int>::iterator it3 = patchesToEncapsulate.begin();
                while(it3 != patchesToEncapsulate.end() && !found){
                    if(*it3 == it2->toId){
                        found = true;
                    }
                    it3++;
                }
                if(!found){
                    if(validOutput == 0){
                        patchId = *it;
                        validOutput++;
                    } else {
                        if(patchId != *it) {
                            validOutput++;
                            break;
                        }
                    }
                    
                }
            }
            if(validOutput > 1) {
                ConsoleLog::getInstance()->pushError("There is more than one output of a node that it's destiny is not a selected node");
                patchId = -1;
                patchesToEncapsulate.clear();
                break;
            }
        }
        
        // validation ok
        if(patchId != -1) {
            ConsoleLog::getInstance()->pushSuccess("Nodes encapsulated successfully.");
            ConsoleLog::getInstance()->pushSuccess(" \t \t For a quick view click on the encapsulated node and press command key and 'e'.", false);
        }
    }else{
        ConsoleLog::getInstance()->pushError("There are not enough nodes selected to encapsulate");
    }
    
    return patchId;
}

//------------------------------------------------------------------
int ofxComposer::getSelectedEncapsulated(){
    int activeSelected = 0;
    int retId = -1;
    
    for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
        if(it->second->bActive) {
            activeSelected++;
            if(it->second->isLastEncapsulated()){
                retId = it->second->getEncapsulatedId();
            }
        }
    }
    
    if(activeSelected > 1){
        retId = -1;
    }
    
    return retId;
}

//------------------------------------------------------------------
//void ofxComposer::setWindowsIdForEncapsulated(int encapsulatedId, int winId){
//    for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
//        if(it->second->getEncapsulatedId() == encapsulatedId){
//            it->second->setWindowId(winId);
//        }
//    }
//}

//------------------------------------------------------------------
//void ofxComposer::restoreWindowsForEncapsulated(int previousWin){
//    for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
//        if(it->second->getWindowId() == previousWin){
//            if(it->second->isLastEncapsulated()){
//                it->second->setWindowId(MAIN_WINDOW);
//            }else{
//                it->second->setWindowId(-1);
//            }
//        } else if(it->second->getWindowId() > previousWin){
//            it->second->setWindowId(it->second->getWindowId() - 1);
//        }
//    }
//}

//------------------------------------------------------------------
// This function sets the output of every node that is conected to one encapsulated
// to the last encapsulated node
void ofxComposer::setOutputEncapsulated(int patchId, vector<int> encapsulatedPatches){
    // iterate through every output patch
    for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
        vector<LinkDot> output = it->second->outPut;
        for(int i = 0; i < output.size(); i++){
            // if the toId of the output is one of the encapsulated patches, set the values of the new output
            for(int j = 0; j < encapsulatedPatches.size(); j++){
                if(encapsulatedPatches[j] == output[i].toId){
                    output[i].toEncapsulatedId = patchId;
                    output[i].toEncapsulated = &patches.at(patchId)->inPut.at(0);
                    it->second->outPut[i] = output[i];
                }
            }
        }
    }
}

//------------------------------------------------------------------
void ofxComposer::restoreOutputEncapsulated(int lastPatchId){
    for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
        vector<LinkDot> output = it->second->outPut;
        for(int i = 0; i < output.size(); i++){
            if(output[i].toEncapsulatedId == lastPatchId){
                output[i].toEncapsulatedId = -1;
                it->second->outPut[i] = output[i];
            }
        }
    }
}

//------------------------------------------------------------------
int ofxComposer::getLastPatchEncapsulated(int encapsulatedId){
    for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
        if(it->second->bActive && it->second->isLastEncapsulated() && it->second->getEncapsulatedId() == encapsulatedId) {
            return it->second->getId();
        }
    }
}

//------------------------------------------------------------------
//void ofxComposer::setCameraForWindow(int winId, ofEasyCam cam){
//    for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
//        if(it->second->getWindowId() == winId || it->second->isLastEncapsulated()){
//            it->second->setParent(*this->getParent());
//            it->second->title->setParent(*this->getParent());
//        }
//    }
//}

//------------------------------------------------------------------
string  ofxComposer::getLastEncapsulatedName(int encapsulatedId){
    for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
        if(it->second->isLastEncapsulated() && it->second->getEncapsulatedId() == encapsulatedId) {
            return it->second->getName();
        }
    }
    return "";
}

//------------------------------------------------------------------
bool ofxComposer::saveEncapsulatedSettings(ofxXmlSettings &XML, int encapsulatedId){
    bool saved = true;
    int lastPatch = -1;
    
    for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
        if(it->second->isLastEncapsulated() && it->second->getEncapsulatedId() == encapsulatedId) {
            lastPatch = it->second->getId();
        }
    }

    if(lastPatch < 0){
        stringstream ss;
        ss << "Error saving encapsulated with id " << encapsulatedId << ". The node doesn't exists.";
        ofLog(OF_LOG_ERROR, ss.str());
        ConsoleLog::getInstance()->pushError("Error saving encapsulated");
        return false;
    }
    
    int totalEncapsulated = XML.getNumTags("ENCAPSULATED_NODE");
    for (int i = 0; i <= totalEncapsulated; i++){
        if ( XML.getAttribute("ENCAPSULATED_NODE", "id", -1, i) == encapsulatedId){
            XML.setAttribute("ENCAPSULATED_NODE", "id", encapsulatedId, totalEncapsulated);
            XML.setAttribute("ENCAPSULATED_NODE", "lastEncapsulatedId", lastPatch, totalEncapsulated);
            XML.pushTag("ENCAPSULATED_NODE");
            saved = true;
            break;
        } else if (i == totalEncapsulated) {
            // all ENCAPSULATED_NODE tags were already processed and it wasn't found
            int lastPlace = XML.addTag("ENCAPSULATED_NODE");
            XML.addAttribute("ENCAPSULATED_NODE", "id", encapsulatedId, totalEncapsulated);
            XML.addAttribute("ENCAPSULATED_NODE", "lastEncapsulatedId", lastPatch, totalEncapsulated);
            XML.pushTag("ENCAPSULATED_NODE", lastPlace);
            saved = true;
        }
    }

    if(saved){
        int nodeTag = 0;
        for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
            if(!it->second->isLastEncapsulated() && it->second->getEncapsulatedId() == encapsulatedId) {
                nodeTag = XML.addTag("NODE");
                XML.setValue("NODE", it->second->getId(), nodeTag);
            }
        }
    }
    XML.popTag();
    
    stringstream ss;
    if(!saved){
        ss << "Error saving encapsulated with id " << encapsulatedId;
        ofLog(OF_LOG_ERROR, ss.str());
        ConsoleLog::getInstance()->pushError("Error saving encapsulated");
    }
    
    return saved;
}

bool ofxComposer::saveEncapsulatedSettingsToSnippet(ofxXmlSettings &XML, int encapsulatedId, map<int,int> newIdsMap) {
    
    bool saved = true;
    int lastPatch = -1;
    
    for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
        if(it->second->isLastEncapsulated() && it->second->getEncapsulatedId() == encapsulatedId) {
            lastPatch = it->second->getId();
        }
    }
    
    if(lastPatch < 0){
        stringstream ss;
        ss << "Error saving encapsulated with id " << encapsulatedId << ". The node doesn't exists.";
        ofLog(OF_LOG_ERROR, ss.str());
        ConsoleLog::getInstance()->pushError("Error saving encapsulated");
        return false;
    }
    

    int lastPlace = XML.addTag("ENCAPSULATED_NODE");
    XML.addAttribute("ENCAPSULATED_NODE", "id", encapsulatedId, lastPlace);
    XML.addAttribute("ENCAPSULATED_NODE", "lastEncapsulatedId", newIdsMap[lastPatch], lastPlace);
    saved = XML.pushTag("ENCAPSULATED_NODE", lastPlace);

    if(saved){
        int nodeTag = 0;
        for(map<int,ofxPatch*>::iterator it = patches.begin(); it != patches.end(); it++ ){
            if(!it->second->isLastEncapsulated() && it->second->getEncapsulatedId() == encapsulatedId) {
                nodeTag = XML.addTag("NODE");
                XML.setValue("NODE", newIdsMap[it->second->getId()], nodeTag);
            }
        }
    }
    XML.popTag();
    
    stringstream ss;
    if(!saved){
        ss << "Error saving encapsulated with id " << encapsulatedId;
        ofLog(OF_LOG_ERROR, ss.str());
        ConsoleLog::getInstance()->pushError("Error saving encapsulated");
    }
    
    return saved;
}

