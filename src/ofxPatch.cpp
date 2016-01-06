//
//  ofxPatch.cpp
//  emptyExample
//
//  Created by Patricio Gonzalez Vivo on 3/9/12.
//  Copyright (c) 2012 http://www.PatricioGonzalezVivo.com All rights reserved.
//

#include "ofxPatch.h"
#include <GLUT/glut.h>

ofxPatch::ofxPatch(){
    nId                 = -1;
    type                = "none";
    filePath            = "none";
    configFile          = "appSettings.xml";
    
    selectedMaskCorner  = -1;
    selectedTextureCorner = -1;
    
    bEditMode           = true;
    bEditMask           = false;
    bActive             = false;
    bVisible            = true;
    
    bMasking            = false;
    
    bUpdateMask         = true;
    bUpdateCoord        = true;
    
    image               = NULL;
    shader              = NULL;
    videoPlayer         = NULL;
    videoGrabber        = NULL;
    //texture             = NULL;
    
    drawImage           = false;
    drawVideo           = false;
    drawCamera          = false;
    drawTexture         = false;
    drawFbo             = false;
    
    width               = NODE_WIDTH;
    height              = NODE_HEIGHT;
    
    texOpacity          = 1.0;
    maskOpacity         = 1.0;
    
    disabledPatch       = false;
    
    selectedLink        = -1;
    selectedLinkVertex  = -1;
    
    bInspector          = false;
    
    string shaderProgram = "#version 120\n\
    #extension GL_ARB_texture_rectangle : enable\n\
    \n\
    uniform sampler2DRect tex0;\n\
    uniform sampler2DRect maskTex;\n\
    uniform float texOpacity;\n\
    uniform float maskOpacity;\n\
    \n\
    void main (void){\n\
    vec2 pos = gl_TexCoord[0].st;\n\
    \n\
    vec4 src = texture2DRect(tex0, pos);\n\
    float mask = texture2DRect(maskTex, pos).r;\n\
    \n\
    gl_FragColor = vec4( src.rgb * texOpacity , clamp( min(src.a,mask) , maskOpacity, 1.0));\n\
    }\n";
    maskShader.setupShaderFromSource(GL_FRAGMENT_SHADER, shaderProgram);
    maskShader.linkProgram();
    maskFbo.allocate(width, height);
    
    color.set(200,255);
    
    maskCorners.addVertex(0.0,0.0);
    maskCorners.addVertex(1.0,0.0);
    maskCorners.addVertex(1.0,1.0);
    maskCorners.addVertex(0.0,1.0);
    
    textureCorners.addVertex(0.0,0.0);
    textureCorners.addVertex(width,0.0);
    textureCorners.addVertex(width,height);
    textureCorners.addVertex(0.0,height);
    
    title = new ofxTitleBar(&box, &nId);
    title->addButton('m', &bEditMask, TOGGLE_BUTTON);
    title->addButton('v', &bVisible, TOGGLE_BUTTON);
    title->addButton('i', &bInspector, TOGGLE_BUTTON);
    title->setParent(*this);
    ofAddListener( title->reset , this, &ofxPatch::_reMakeFrame);
    
    ofAddListener(ofEvents().mousePressed, this, &ofxPatch::_mousePressed, PATCH_EVENT_PRIORITY);
    ofAddListener(ofEvents().mouseDragged, this, &ofxPatch::_mouseDragged, PATCH_EVENT_PRIORITY);
    ofAddListener(ofEvents().mouseReleased, this, &ofxPatch::_mouseReleased, PATCH_EVENT_PRIORITY);
    ofAddListener(ofEvents().keyPressed, this, &ofxPatch::_keyPressed, PATCH_EVENT_PRIORITY);
    
};

ofxPatch::~ofxPatch(){
    if ( image != NULL )
        delete image;
    
    if ( shader != NULL )
        delete shader;
    
    if ( videoPlayer != NULL )
        delete videoPlayer;
    
    if ( videoGrabber != NULL )
        delete videoGrabber;
    
//    if ( texture != NULL )
//        delete texture;
    
    inPut.clear();
    outPut.clear();
    textureCorners.clear();
    maskCorners.clear();
}

/* ================================================ */
/*                      LOOPS                       */
/* ================================================ */
void ofxPatch::update(){
    
    if ((width != getSrcTexture().getWidth()) ||
        (height != getSrcTexture().getHeight()) ){
        width = getSrcTexture().getWidth();
        height = getSrcTexture().getHeight();
        
        bUpdateCoord = true;
        bUpdateMask = true;
    }
    
    if (bUpdateMask){
        // If the texture change or it's new it will update some parameters
        // like the size of the FBO, the mask and the matrix
        //
        if ((maskFbo.src->getWidth() != getSrcTexture().getWidth()) ||
            (maskFbo.src->getHeight() != getSrcTexture().getHeight()) ){
            width = getSrcTexture().getWidth();
            height = getSrcTexture().getHeight();
            
            maskFbo.allocate(width,height);
        }
        
        // Generate masking contour
        //
        maskFbo.src->begin();
        ofClear(0,0);
        ofBeginShape();
        ofSetColor(255, 255, 255);
        for(int i = 0; i < maskCorners.size(); i++ ){
            ofVertex(maskCorners[i].x*width,maskCorners[i].y*height);
        }
        ofEndShape(true);
        maskFbo.src->end();
        
        bUpdateMask = false;
    }
    
    if (bMasking){
        //  Masking it«s no allways set. Because it takes some GPU speed off it«s only activated when user edit it.
        //
        
        if ( textureCorners.inside(ofGetMouseX(), ofGetMouseY()) && (bEditMode)){
            texOpacity = ofLerp(texOpacity,1.0, 0.01);
            maskOpacity = ofLerp(maskOpacity,0.8, 0.01);
        } else if (!bEditMode){
            texOpacity = ofLerp(texOpacity, 1.0, 0.01);
            maskOpacity = ofLerp(maskOpacity, 0.0, 0.01);
        } else {
            texOpacity = ofLerp(texOpacity, 0.8, 0.01);
            maskOpacity = ofLerp(maskOpacity, 0.0, 0.01);
        }
        
        maskFbo.dst->begin();
        ofClear(0, 0);
        maskShader.begin();
        maskShader.setUniformTexture("maskTex", maskFbo.src->getTextureReference(), 1 );
        maskShader.setUniform1f("texOpacity", texOpacity);
        maskShader.setUniform1f("maskOpacity", maskOpacity);
        
        getSrcTexture().draw(0,0);
        
        maskShader.end();
        maskFbo.dst->end();
    }
    
    if (bUpdateCoord){
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
    
    // Update shader or video content
    //
    if (videoPlayer != NULL){
        videoPlayer->update();
    } else if (videoGrabber != NULL){
        //videoGrabber->update();
    } else if (shader != NULL){
        shader->update();
    }
    
    // Send the texture to the linked Patches
    //
//    for ( int i = 0; i < outPut.size(); i++ ){
//        if (outPut[i].toShader != NULL){
//            outPut[i].toShader->setTexture( getTextureReference(), outPut[i].nTex );
//        }
//    }

}

//------------------------------------------------------------------
void ofxPatch::customDraw(){
    
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
    
    if (bEditMode) {
        
        ofPushStyle();
        
        // Draw de title
        //
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
                if(bActive){
                    ofSetLineWidth(3.f);
                    ofSetColor(150,150,250);
                }
                ofLine(textureCorners[i].x, textureCorners[i].y, textureCorners[(i+1)%4].x, textureCorners[(i+1)%4].y);
                ofSetLineWidth(1.f);
            }
        } else {
            // Draw dragables mask corners
            //
            for(int i = 0; i < maskCorners.size(); i++){
                ofVec3f pos = ofVec3f( maskCorners[i].x * width, maskCorners[i].y * height, 0.0);
                pos = surfaceToScreenMatrix * pos;
                
                ofVec3f mouse = ofVec3f(ofGetMouseX(), ofGetMouseY(), 0.0)*this->getGlobalTransformMatrix();
                if ( (selectedMaskCorner == i) || ( ofDist(mouse.x, mouse.y, pos.x, pos.y) <= 4 ) ) {
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
            }
        }

        // Draw the input linking dots
        //
        for(int i = 0; i < inPut.size(); i++){
            ofSetColor(255, 150);
            ofNoFill();
            ofCircle(inPut[i].pos, 5);
        }
        
        // Draw the output linking dot
        //
        ofNoFill();
        ofSetColor(255, 150);
        ofCircle(getOutPutPosition(), 5);
        ofPopStyle();
        
        // Draw the links between nodes
        //
        for (int i = 0; i < outPut.size(); i++){
            if (outPut[i].to != NULL){
                ofFill();
                ofCircle(outPut[i].pos, 3);
                if (linkType == STRAIGHT_LINKS)
                    ofLine(outPut[i].pos, outPut[i].to->pos);
                else if (linkType == CURVE_LINKS) {
                    ofNoFill();
                    ofBezier(outPut[i].pos.x, outPut[i].pos.y, outPut[i].pos.x+55, outPut[i].pos.y, outPut[i].to->pos.x-55, outPut[i].to->pos.y, outPut[i].to->pos.x, outPut[i].to->pos.y);
                    ofFill();
                }
                else {
                    if (outPut[i].link_vertices.size() > 0) {
                        ofNoFill();
                        for(int j = 0; j < outPut[i].link_vertices.size(); j++){
                            ofCircle( outPut[i].link_vertices[j], 4);
                        }
                    }
                    
                    outPut[i].link_line.clear();
                    outPut[i].link_line.addVertex(outPut[i].pos);
                    if (outPut[i].link_vertices.size() > 0)
                        outPut[i].link_line.addVertices(outPut[i].link_vertices);
                    outPut[i].link_line.addVertex(outPut[i].to->pos);
                    outPut[i].link_line.draw();
                    
                    ofFill();
                }
                
                ofCircle(outPut[i].to->pos, 3);
            }
        }
    }
}

//------------------------------------------------------------------
void ofxPatch::drawInspectorGUI() {
    
    // Draw the inspector
    //
    if (bInspector) {
        ofVec3f scale = ((ofCamera*)this->getParent())->getScale();
        panel.setPosition(ofVec3f((textureCorners[1].x/scale.x + 2), (textureCorners[1].y/scale.y - 42), scale.z));
        panel.draw();
    }
}

/* ================================================ */
/* ================================================ */


/* ================================================ */
/*                     EVENTS                       */
/* ================================================ */

void ofxPatch::_reMakeFrame( int &_nId ){
    float offSet = 0.0;
    
    if (title != NULL)
        offSet = 15;
    
    if (type == "ofxGLEditor"){
        textureCorners[0].set(0.0, height + offSet);
        textureCorners[1].set(width, height + offSet);
        textureCorners[2].set(width, offSet);
        textureCorners[3].set(0.0, offSet);
    } else {
        textureCorners[0].set(0.0, offSet);
        textureCorners[1].set(width, offSet);
        textureCorners[2].set(width, height + offSet);
        textureCorners[3].set(0.0, height + offSet);
    }
    
    bUpdateCoord = true;
    saveSettings();
}

//------------------------------------------------------------------
void ofxPatch::_mousePressed(ofMouseEventArgs &e){
    ofVec3f mouse = ofVec3f(e.x, e.y, 0.0)*this->getGlobalTransformMatrix();
    
    if (bEditMode){
        
        // check is mouse is pressing over the inspector
//        if (inspector != NULL and inspector->isVisible() and inspector->isHit(mouse.x, mouse.y)) {
//            canvas->setOtherSelected(true);
//            return;
//        }
    }
    
    if (bEditMode && bActive ){
        if (!bEditMask){
            // Editing the texture corners
            //
            for(int i = 0; i < 4; i++){
                if ( ofDist(mouse.x, mouse.y, textureCorners[i].x, textureCorners[i].y) <= 10 )
                    selectedTextureCorner = i;
            }
        } else {
            // Editing the mask corners
            //
            bool overDot = false;
            for(int i = 0; i < maskCorners.size(); i++){
                ofVec3f pos = getSurfaceToScreen( ofPoint(maskCorners[i].x * width, maskCorners[i].y * height));
                
                if ( ofDist(mouse.x, mouse.y, pos.x, pos.y) <= 10 ){
                    selectedMaskCorner = i;
                    overDot = true;
                }
            }
            
            // Add new Dot if it's over the line
            //
            if (!overDot){
                doScreenToSurfaceMatrix();
                mouse = screenToSurfaceMatrix * mouse;
                mouse.x = mouse.x / width;
                mouse.y = mouse.y / height;
                
                int addNew = -1;
                
                // Search for the right placer to incert the point in the array
                //
                for (int i = 0; i < maskCorners.size(); i++){
                    int next = (i+1)%maskCorners.size();
                    
                    ofVec2f AtoM = mouse - maskCorners[i];
                    ofVec2f AtoB = maskCorners[next] - maskCorners[i];
                    
                    float a = atan2f(AtoM.x, AtoM.y);
                    float b = atan2f(AtoB.x, AtoB.y);
                    
                    if ( abs(a - b) < 0.05){
                        addNew = next;
                    }
                }
                
                if (addNew >= 0 ){
                    maskCorners.getVertices().insert( maskCorners.getVertices().begin()+addNew, mouse);
                    selectedMaskCorner = addNew;
                }
                
            }
        }
    }
    // Is mouse pressing over link dot ?
    if (bEditMode and linkType == PATH_LINKS){
        
        bool overDot = false;
        for (int i = 0; i < outPut.size() and !overDot; i++){
            
            for (int j = 0; j < outPut[i].link_vertices.size(); j++){
                
                if ( ofDist(mouse.x, mouse.y, outPut[i].link_vertices[j].x, outPut[i].link_vertices[j].y) <= 10 ){
                    if ((e.button == 2) || (glutGetModifiers() == GLUT_ACTIVE_CTRL)) {
                        outPut[i].link_vertices.erase(outPut[i].link_vertices.begin()+j);
                    }
                    else {
                        selectedLinkVertex = j;
                        selectedLink = i;
                    }
                    overDot = true;
                    setLinkHit(true);
                }
            }
            
            if (!overDot and outPut.size() > 0){
                vector<ofPoint> link_vertices = outPut[i].link_line.getVertices();
                
                if (link_vertices.size()){
                    int addNew = -1;
                    int tolerance = 3;
                    
                    for (int j = 0; j < link_vertices.size()-1; j++){
                        int next = (j+1)%link_vertices.size();
                        
                        if (is_between (mouse.x, link_vertices[j].x, link_vertices[j+1].x, tolerance) && is_between (mouse.y, link_vertices[j].y, link_vertices[j+1].y, tolerance))
                        {
                            if (std::abs(link_vertices[j+1].y - link_vertices[j].y) <= tolerance) // Horizontal line.
                            {
                                addNew = j;
                            }
                            
                            const float M = (link_vertices[j+1].y - link_vertices[j].y) / (link_vertices[j+1].x - link_vertices[j].x); // Slope
                            const float C = -(M * link_vertices[j].x) + link_vertices[j].y; // Y intercept
                            
                            // Checking if (x, y) is on the line passing through the end points.
                            if(std::fabs (mouse.y - (M * mouse.x + C)) <= tolerance) {
                                addNew = j;
                            }
                        }
                    }
                    
                    if (addNew >= 0) {
                        
                        setLinkHit(true);
                        overDot = true;
                        selectedLinkVertex = addNew;
                        selectedLink = i;
                        
                        if (outPut[i].link_vertices.size() == 0)
                            outPut[i].link_vertices.push_back(ofVec3f(mouse.x, mouse.y, 0.0));
                        else if (addNew == 0)
                            outPut[i].link_vertices.insert(outPut[i].link_vertices.begin(), ofVec3f(mouse.x, mouse.y, 0.0));
                        else
                            outPut[i].link_vertices.insert(outPut[i].link_vertices.begin()+addNew, ofVec3f(mouse.x, mouse.y, 0.0));
                    }
                }
            }
        }
    }
}

//------------------------------------------------------------------
void ofxPatch::_mouseDragged(ofMouseEventArgs &e){
    
    if((disabledPatch and !isLinkHit()) or canvas->getOtherSelected()){
        return;
    }
    
    ofVec3f mouse = ofVec3f(e.x, e.y,0);
    ofVec3f mouseLast = ofVec3f(ofGetPreviousMouseX(),ofGetPreviousMouseY(),0);
    ofVec3f mouse_transformed = ofVec3f(ofGetMouseX(), ofGetMouseY(), 0.0)*this->getGlobalTransformMatrix();
    
    if (bEditMode){
        if (!bEditMask){
            // Drag texture corners
            //
            if (( selectedTextureCorner >= 0) && ( selectedTextureCorner < 4) ){
                
                if (e.button == 2){
                    // Deformation
                    //
                    textureCorners[selectedTextureCorner].x = mouse_transformed.x;
                    textureCorners[selectedTextureCorner].y = mouse_transformed.y;
                    
                    bUpdateCoord = true;
                    
                } else if ( ofGetKeyPressed('r') ){
                    // Rotation
                    //
                    ofVec2f center = getPos();
                    
                    ofVec2f fromCenterTo = mouseLast - center;
                    float prevAngle = -1.0*atan2f(fromCenterTo.x,fromCenterTo.y)+(PI/2);
                    
                    fromCenterTo = mouse - center;
                    float actualAngle = -1.0*atan2f(fromCenterTo.x,fromCenterTo.y)+(PI/2);
                    
                    float dif = actualAngle-prevAngle;
                    
                    rotate(dif);
                } else if (( e.button == 1 ) || ofGetKeyPressed('a') ){
                    // Centered Scale
                    //
                    float prevDist = mouseLast.distance(getPos());
                    float actualDist = mouse.distance(getPos());
                    
                    float dif = actualDist/prevDist;
                    
                    scale(dif);
                } else {
                    // Corner Scale
                    //
                    ofVec2f center = getPos();
                    
                    int  opositCorner = (selectedTextureCorner - 2 < 0)? (4+selectedTextureCorner-2) : (selectedTextureCorner-2);
                    ofVec2f toOpositCorner = center - textureCorners[opositCorner];
                    
                    float prevDist = mouseLast.distance( textureCorners[opositCorner] );
                    float actualDist = mouse.distance( textureCorners[opositCorner] );
                    
                    float dif = actualDist/prevDist;
                    
                    move( textureCorners[opositCorner] + toOpositCorner * dif );
                    scale(dif);
                }
                
                // Drag all the surface
                //
            } else if ( bActive && !isLinkHit() ){
                for (int i = 0; i < 4; i++){
                    textureCorners[i] += mouse-mouseLast;
                }
                
                bUpdateCoord = true;
                mouseLast = mouse;
            }
        } else {
            
            // Drag mask points
            //
            for(int i = 0; i < maskCorners.size(); i++){
                ofVec3f pos = ofVec3f( maskCorners[i].x * width, maskCorners[i].y * height, 0.0);
                pos = surfaceToScreenMatrix * pos;
                
                if ((selectedMaskCorner >= 0) && (selectedMaskCorner < maskCorners.size() )){
                    ofVec3f newPos = ofVec3f(mouse_transformed.x, mouse_transformed.y, 0.0);
                    doScreenToSurfaceMatrix();
                    
                    newPos = screenToSurfaceMatrix * mouse_transformed;
                    newPos.x = ofClamp(newPos.x / width, 0.0, 1.0);
                    newPos.y = ofClamp(newPos.y / height, 0.0, 1.0);
                    
                    maskCorners[selectedMaskCorner] = newPos;
                    bMasking = true;
                    
                    bUpdateMask = true;
                }
            }
        }
        
        // Drag link vertices
        //
        if (selectedLink >= 0 and selectedLinkVertex >= 0) {
            outPut[selectedLink].link_vertices[selectedLinkVertex] = ofVec3f(mouse_transformed.x, mouse_transformed.y, 0.0);
        }

    }
}

//------------------------------------------------------------------
void ofxPatch::_mouseReleased(ofMouseEventArgs &e){
    
    // mouse is not longer pressing the inspector or link
    canvas->setOtherSelected(false);
    selectedLinkVertex = -1;
    selectedLink     = -1;
    setLinkHit(false);
    
    ofVec3f mouse = ofVec3f(e.x, e.y,0);
    if (bEditMode || isOver(mouse)){
        if (!bEditMask){
            if (( selectedTextureCorner >= 0) && ( selectedTextureCorner < 4) ){
                selectedTextureCorner = -1;
            }
            saveSettings();
        } else {
            bUpdateMask = true;
            saveSettings();
            selectedMaskCorner = -1;
        }
    }
}

//------------------------------------------------------------------
void ofxPatch::_keyPressed(ofKeyEventArgs &e){
    
    if (e.key == OF_KEY_F2){
        bEditMode = !bEditMode;
    } else if (e.key == OF_KEY_F3){
        if ( bActive ){
            bEditMask = !bEditMask;
        }
    } else if (e.key == OF_KEY_F4){
        if ( bActive ){
            float offSet = 0.0;
            
            if (title != NULL)
                offSet = 15;
            
            if (type == "ofxGLEditor"){
                textureCorners[0].set(0.0, height + offSet);
                textureCorners[1].set(width, height + offSet);
                textureCorners[2].set(width, offSet);
                textureCorners[3].set(0.0, offSet);
            } else {
                textureCorners[0].set(0.0, offSet);
                textureCorners[1].set(width, offSet);
                textureCorners[2].set(width, height + offSet);
                textureCorners[3].set(0.0, height + offSet);
            }
        }
    }
    
    if (bActive && bEditMode & bEditMask) {
        
        // Delete the selected mask point
        //
        if ( (e.key == 'x') &&
            (selectedMaskCorner >= 0) &&
            (selectedMaskCorner < maskCorners.size() ) ){
            maskCorners.getVertices().erase(maskCorners.getVertices().begin()+ selectedMaskCorner );
            selectedMaskCorner = -1;
            
            bUpdateMask = true;
            saveSettings();
            
            bMasking = true;
        }
        
        // Reset all the mask or the texture
        //
        if ( (e.key == 'r') ){
            maskCorners.clear();
            selectedMaskCorner = -1;
            maskCorners.addVertex(0.0,0.0);
            maskCorners.addVertex(1.0,0.0);
            maskCorners.addVertex(1.0,1.0);
            maskCorners.addVertex(0.0,1.0);
            
            bUpdateMask = true;
            saveSettings();
            
            bMasking = false;
        }
    }
}

//------------------------------------------------------------------
void ofxPatch::guiEvent(ofxUIEventArgs &e)
{
    string name = e.widget->getName();
    
    if (name == "Image src btn" && ((ofxUIButton*)e.widget)->getValue()) {
        
        ofFileDialogResult openFileResult = ofSystemLoadDialog("Select an image (.jpg, .jpeg, .png or .bmp)");
        
        if (openFileResult.bSuccess){
            
            ofFile file (openFileResult.getPath());
            
            if (file.exists()){
                
                string fileExtension = ofToUpper(file.getExtension());
                
                //We only want images
                if (fileExtension == "JPG"  ||
                    fileExtension == "PNG"  ||
                    fileExtension == "JPEG" ||
                    fileExtension == "GIF"  ||
                    fileExtension == "BMP"  ) {
                    imageSrc = openFileResult.getPath();
                    //((ofxUITextInput*)inspector->getWidget("Image src"))->setTextString(imageSrc);
                }
                else return;
            }
            file.close();
        }
    }
}

/* ================================================ */
/* ================================================ */


/* ================================================ */
/*                     SETTERS                      */
/* ================================================ */

void ofxPatch::setFrag( string _code){
    if ( shader != NULL ){
        if (shader->setFragmentShader( _code )){
            saveSettings();
            // Calculate the need dots.
            //
            if (shader != NULL){
                if (shader->getNumberOfTextures() > inPut.size()){
                    LinkDot p;
                    bUpdateCoord = true;
                    inPut.push_back(p);
                } else if (shader->getNumberOfTextures() < inPut.size()) {
                    inPut.erase( inPut.end() );
                    bUpdateCoord = true;
                }
            }
        }
    }
}

//------------------------------------------------------------------
void ofxPatch::setTexture(ofTexture& _tex, int _texNum){
    if ( shader != NULL ){
        shader->setTexture(tex, _texNum);
    } else {
        tex = _tex;
    }
}

//------------------------------------------------------------------
void ofxPatch::setCoorners(ofPoint _coorners[4]){
    for (int i = 0; i < 4; i++){
        textureCorners[i].set(_coorners[i]);
    }
    bUpdateCoord = true;
    bUpdateMask = true;
}

//------------------------------------------------------------------
void ofxPatch::setCoorners(vector<ofPoint> _coorners){
    for (int i = 0; i < 4; i++){
        textureCorners[i].set(_coorners[i]);
    }
    bUpdateCoord = true;
    bUpdateMask = true;
}

//------------------------------------------------------------------
void ofxPatch::setLinkType(nodeLinkType type) {
    linkType = type;
}

//------------------------------------------------------------------
void ofxPatch::setMainCanvas(ofxUISuperCanvas* _canvas) {
    this->canvas = _canvas;
}

//------------------------------------------------------------------
void ofxPatch::setDisablePatch(bool disable){
    disabledPatch = disable;
}

//------------------------------------------------------------------
void ofxPatch::setLinkHit(bool linkHit){
    this->linkHit = linkHit;
}

//------------------------------------------------------------------
void ofxPatch::setDrawInspector(bool draw_){
    this->bInspector = draw_;
}

/* ================================================ */
/* ================================================ */


/* ================================================ */
/*                     GETTERS                      */
/* ================================================ */


string ofxPatch::getFrag(){
    if ( shader != NULL ){
        return shader->getFragmentShader();
    }
}

//------------------------------------------------------------------
ofTexture& ofxPatch::getSrcTexture(){
    if (drawImage)
        return image->getTextureReference();
    else if (drawVideo)
        return videoPlayer->getTextureReference();
    else if (drawCamera)
        return videoGrabber->getTextureReference();
    else if (drawFbo)
        return fbo.getTextureReference();
    else if (shader != NULL)
        return shader->getTextureReference();
    else if (drawTexture)
        return tex;
    else
        return maskFbo.dst->getTextureReference();
}

//------------------------------------------------------------------
ofTexture& ofxPatch::getTextureReference(){
    
    // If masking available, draw the mask. If not, draw texture reference (image, video, camera, fbo, texture)
    //
    if (bMasking)
        return maskFbo.dst->getTextureReference();
    else
        return getSrcTexture();
}

//------------------------------------------------------------------
ofPolyline ofxPatch::getTextureCoorners() {
    return textureCorners;
}

//------------------------------------------------------------------
float ofxPatch::getHeight() {
    return height;
}

//------------------------------------------------------------------
float ofxPatch::getWidth(){
    return width;
}

//------------------------------------------------------------------
float ofxPatch::getHighestYCoord(){
    int highestCoord = 0;
    float offSet = 0.0;
    
    if (title != NULL)
        offSet = 15;
    
    for(int i = 0; i < 4; i++){
        if(highestCoord < textureCorners[i].y+offSet){
            highestCoord = textureCorners[i].y+offSet;
        }
    }
    return highestCoord;
}

//------------------------------------------------------------------
float ofxPatch::getLowestYCoord(){
    int lowestCoord = 10000;
    for(int i = 0; i < 4; i++){
        
        if(lowestCoord > textureCorners[i].y){
            lowestCoord = textureCorners[i].y;
        }
    }
    return lowestCoord;
}

//------------------------------------------------------------------
float ofxPatch::getHighestXCoord(){
    int highestCoord = 0;
    float offSet = 0.0;
    
    if (title != NULL)
        offSet = 15;
    
    for(int i = 0; i < 4; i++){
        if(highestCoord < textureCorners[i].x+offSet){
            highestCoord = textureCorners[i].x+offSet;
        }
    }
    return highestCoord;
}

//------------------------------------------------------------------
float ofxPatch::getLowestXCoord(){
    int lowestCoord = 10000;
    for(int i = 0; i < 4; i++){
        
        if(lowestCoord > textureCorners[i].x){
            lowestCoord = textureCorners[i].x;
        }
    }
    return lowestCoord;
}

//------------------------------------------------------------------
bool ofxPatch::isLinkHit(){
    return linkHit;
}

//------------------------------------------------------------------
bool ofxPatch::drawInspector(){
    return bInspector;
}

/* ================================================ */
/* ================================================ */


/* ================================================ */
/*                 OTHER FUNCTIONS                  */
/* ================================================ */

void ofxPatch::move(ofPoint _pos){
    ofVec2f diff = _pos - getPos();
    
    for(int i = 0; i < 4; i++){
        textureCorners[i] += diff;
    }
    
    //setFromCenter(getPos().x, getPos().y, width, height);
    
    bUpdateCoord = true;
}

//------------------------------------------------------------------
void ofxPatch::scale(float _scale){
    for(int i = 0; i < 4; i++){
        ofVec2f center = getPos();
        ofVec2f fromCenterToCorner = textureCorners[i] - center;
        
        float radio = fromCenterToCorner.length();
        float angle = -1.0*atan2f(fromCenterToCorner.x,fromCenterToCorner.y)+(PI/2);
        
        radio *= _scale;
        
        textureCorners[i] = center + ofPoint(radio * cos(angle),
                                             radio * sin(angle),
                                             0.0);
    }
    
    bUpdateCoord = true;
}

//------------------------------------------------------------------
void ofxPatch::rotate(float _rotAngle){
    for(int i = 0; i < 4; i++){
        ofVec2f center = getPos();
        ofVec2f fromCenterToCorner = textureCorners[i] - center;
        
        float radio = fromCenterToCorner.length();
        float angle = -1.0*atan2f(fromCenterToCorner.x,fromCenterToCorner.y)+(PI/2);
        
        angle += _rotAngle;
        
        textureCorners[i] = center + ofPoint(radio * cos(angle),
                                             radio * sin(angle),
                                             0.0);
    }
    
    bUpdateCoord = true;
}

//------------------------------------------------------------------
bool ofxPatch::isOver(ofPoint _pos){
    ofRectangle biggerBox = textureCorners.getBoundingBox();
    biggerBox.setFromCenter(biggerBox.getCenter().x, biggerBox.getCenter().y, biggerBox.width+20, biggerBox.height+20);
    
    return biggerBox.inside(_pos);
};

//------------------------------------------------------------------
void ofxPatch::moveDiff(ofVec2f diff){
    for(int i = 0; i < 4; i++){
        textureCorners[i] += diff;
    }
    bUpdateCoord = true;
}

//------------------------------------------------------------------
bool ofxPatch::is_between (float x, float bound1, float bound2, float tolerance) {
    // Handles cases when 'bound1' is greater than 'bound2' and when
    // 'bound2' is greater than 'bound1'.
    return (((x >= (bound1 - tolerance)) && (x <= (bound2 + tolerance))) ||
            ((x >= (bound2 - tolerance)) && (x <= (bound1 + tolerance))));
}

//------------------------------------------------------------------
void ofxPatch::resetSize(int _width, int _height) {
    
    width = _width;
    height = _height;
    
    x = textureCorners[0].x*((ofCamera*)getParent())->getScale().x;
    y = textureCorners[0].y*((ofCamera*)getParent())->getScale().y;
    
    textureCorners[0].set(x, y);
    textureCorners[1].set(x + (width*SCALE_RATIO), y);
    textureCorners[2].set(x + (width*SCALE_RATIO), y + (height*SCALE_RATIO));
    textureCorners[3].set(x, y + (height*SCALE_RATIO));
}

void ofxPatch::addInputDot() {
    LinkDot p;
    inPut.push_back(p);
}


// -------------------------------------------------------
// --------------------------------------- TRANSFORMATIONS
// -------------------------------------------------------
void ofxPatch::doSurfaceToScreenMatrix(){
    ofPoint src[4];
    
    src[0].set(0, 0);
    src[1].set(width,0.0);
    src[2].set(width,height);
    src[3].set(0, height);
    
    ofPoint dst[4];
    for(int i = 0; i < 4; i++){
        dst[i] = textureCorners[i];
    }
    
    x = textureCorners.getCentroid2D().x;
    y = textureCorners.getCentroid2D().y;
    
    // create the equation system to be solved
    //
    // from: Multiple View Geometry in Computer Vision 2ed
    //       Hartley R. and Zisserman A.
    //
    // x' = xH
    // where H is the homography: a 3 by 3 matrix
    // that transformed to inhomogeneous coordinates for each point
    // gives the following equations for each point:
    //
    // x' * (h31*x + h32*y + h33) = h11*x + h12*y + h13
    // y' * (h31*x + h32*y + h33) = h21*x + h22*y + h23
    //
    // as the homography is scale independent we can let h33 be 1 (indeed any of the terms)
    // so for 4 points we have 8 equations for 8 terms to solve: h11 - h32
    // after ordering the terms it gives the following matrix
    // that can be solved with gaussian elimination:
    
    float P[8][9]=
    {
        {-src[0].x, -src[0].y, -1,   0,   0,  0, src[0].x*dst[0].x, src[0].y*dst[0].x, -dst[0].x }, // h11
        {  0,   0,  0, -src[0].x, -src[0].y, -1, src[0].x*dst[0].y, src[0].y*dst[0].y, -dst[0].y }, // h12
        
        {-src[1].x, -src[1].y, -1,   0,   0,  0, src[1].x*dst[1].x, src[1].y*dst[1].x, -dst[1].x }, // h13
        {  0,   0,  0, -src[1].x, -src[1].y, -1, src[1].x*dst[1].y, src[1].y*dst[1].y, -dst[1].y }, // h21
        
        {-src[2].x, -src[2].y, -1,   0,   0,  0, src[2].x*dst[2].x, src[2].y*dst[2].x, -dst[2].x }, // h22
        {  0,   0,  0, -src[2].x, -src[2].y, -1, src[2].x*dst[2].y, src[2].y*dst[2].y, -dst[2].y }, // h23
        
        {-src[3].x, -src[3].y, -1,   0,   0,  0, src[3].x*dst[3].x, src[3].y*dst[3].x, -dst[3].x }, // h31
        {  0,   0,  0, -src[3].x, -src[3].y, -1, src[3].x*dst[3].y, src[3].y*dst[3].y, -dst[3].y }, // h32
    };
    
    doGaussianElimination(&P[0][0],9);
    
    // gaussian elimination gives the results of the equation system
    // in the last column of the original matrix.
    // opengl needs the transposed 4x4 matrix:
    float aux_H[]= {P[0][8],P[3][8],0,P[6][8], // h11  h21 0 h31
        P[1][8],P[4][8],0,P[7][8], // h12  h22 0 h32
        0      ,      0,0,0,       // 0    0   0 0
        P[2][8],P[5][8],0,1        // h13  h23 0 h33
    };
    
    for(int i=0; i<16; i++)
        glMatrix[i] = aux_H[i];
    
    surfaceToScreenMatrix(0,0)=P[0][8];
    surfaceToScreenMatrix(0,1)=P[1][8];
    surfaceToScreenMatrix(0,2)=0;
    surfaceToScreenMatrix(0,3)=P[2][8];
    
    surfaceToScreenMatrix(1,0)=P[3][8];
    surfaceToScreenMatrix(1,1)=P[4][8];
    surfaceToScreenMatrix(1,2)=0;
    surfaceToScreenMatrix(1,3)=P[5][8];
    
    surfaceToScreenMatrix(2,0)=0;
    surfaceToScreenMatrix(2,1)=0;
    surfaceToScreenMatrix(2,2)=0;
    surfaceToScreenMatrix(2,3)=0;
    
    surfaceToScreenMatrix(3,0)=P[6][8];
    surfaceToScreenMatrix(3,1)=P[7][8];
    surfaceToScreenMatrix(3,2)=0;
    surfaceToScreenMatrix(3,3)=1;
}

//------------------------------------------------------------------
void ofxPatch::doScreenToSurfaceMatrix(){
    ofPoint dst[4];
    
    dst[0].set(0, 0);
    dst[1].set(width,0.0);
    dst[2].set(width,height);
    dst[3].set(0, height);
    
    ofPoint src[4];
    for(int i = 0; i < 4; i++){
        src[i] = textureCorners[i];
    }
    
    // create the equation system to be solved
    //
    // from: Multiple View Geometry in Computer Vision 2ed
    //       Hartley R. and Zisserman A.
    //
    // x' = xH
    // where H is the homography: a 3 by 3 matrix
    // that transformed to inhomogeneous coordinates for each point
    // gives the following equations for each point:
    //
    // x' * (h31*x + h32*y + h33) = h11*x + h12*y + h13
    // y' * (h31*x + h32*y + h33) = h21*x + h22*y + h23
    //
    // as the homography is scale independent we can let h33 be 1 (indeed any of the terms)
    // so for 4 points we have 8 equations for 8 terms to solve: h11 - h32
    // after ordering the terms it gives the following matrix
    // that can be solved with gaussian elimination:
    
    float P[8][9]=
    {
        {-src[0].x, -src[0].y, -1,   0,   0,  0, src[0].x*dst[0].x, src[0].y*dst[0].x, -dst[0].x }, // h11
        {  0,   0,  0, -src[0].x, -src[0].y, -1, src[0].x*dst[0].y, src[0].y*dst[0].y, -dst[0].y }, // h12
        
        {-src[1].x, -src[1].y, -1,   0,   0,  0, src[1].x*dst[1].x, src[1].y*dst[1].x, -dst[1].x }, // h13
        {  0,   0,  0, -src[1].x, -src[1].y, -1, src[1].x*dst[1].y, src[1].y*dst[1].y, -dst[1].y }, // h21
        
        {-src[2].x, -src[2].y, -1,   0,   0,  0, src[2].x*dst[2].x, src[2].y*dst[2].x, -dst[2].x }, // h22
        {  0,   0,  0, -src[2].x, -src[2].y, -1, src[2].x*dst[2].y, src[2].y*dst[2].y, -dst[2].y }, // h23
        
        {-src[3].x, -src[3].y, -1,   0,   0,  0, src[3].x*dst[3].x, src[3].y*dst[3].x, -dst[3].x }, // h31
        {  0,   0,  0, -src[3].x, -src[3].y, -1, src[3].x*dst[3].y, src[3].y*dst[3].y, -dst[3].y }, // h32
    };
    
    doGaussianElimination(&P[0][0],9);
    
    screenToSurfaceMatrix(0,0)=P[0][8];
    screenToSurfaceMatrix(0,1)=P[1][8];
    screenToSurfaceMatrix(0,2)=0;
    screenToSurfaceMatrix(0,3)=P[2][8];
    
    screenToSurfaceMatrix(1,0)=P[3][8];
    screenToSurfaceMatrix(1,1)=P[4][8];
    screenToSurfaceMatrix(1,2)=0;
    screenToSurfaceMatrix(1,3)=P[5][8];
    
    screenToSurfaceMatrix(2,0)=0;
    screenToSurfaceMatrix(2,1)=0;
    screenToSurfaceMatrix(2,2)=0;
    screenToSurfaceMatrix(2,3)=0;
    
    screenToSurfaceMatrix(3,0)=P[6][8];
    screenToSurfaceMatrix(3,1)=P[7][8];
    screenToSurfaceMatrix(3,2)=0;
    screenToSurfaceMatrix(3,3)=1;
    
}

//------------------------------------------------------------------
void ofxPatch::doGaussianElimination(float *input, int n){
    // ported to c from pseudocode in
    // http://en.wikipedia.org/wiki/Gaussian_elimination
    
    float * A = input;
    int i = 0;
    int j = 0;
    int m = n-1;
    while (i < m && j < n)
    {
        // Find pivot in column j, starting in row i:
        int maxi = i;
        for(int k = i+1; k<m; k++)
        {
            if(fabs(A[k*n+j]) > fabs(A[maxi*n+j]))
            {
                maxi = k;
            }
        }
        if (A[maxi*n+j] != 0)
        {
            //swap rows i and maxi, but do not change the value of i
            if(i!=maxi)
                for(int k=0; k<n; k++)
                {
                    float aux = A[i*n+k];
                    A[i*n+k]=A[maxi*n+k];
                    A[maxi*n+k]=aux;
                }
            //Now A[i,j] will contain the old value of A[maxi,j].
            //divide each entry in row i by A[i,j]
            float A_ij=A[i*n+j];
            for(int k=0; k<n; k++)
            {
                A[i*n+k]/=A_ij;
            }
            //Now A[i,j] will have the value 1.
            for(int u = i+1; u< m; u++)
            {
                //subtract A[u,j] * row i from row u
                float A_uj = A[u*n+j];
                for(int k=0; k<n; k++)
                {
                    A[u*n+k]-=A_uj*A[i*n+k];
                }
                //Now A[u,j] will be 0, since A[u,j] - A[i,j] * A[u,j] = A[u,j] - 1 * A[u,j] = 0.
            }
            
            i++;
        }
        j++;
    }
    
    //back substitution
    for(int i=m-2; i>=0; i--)
    {
        for(int j=i+1; j<n-1; j++)
        {
            A[i*n+m]-=A[i*n+j]*A[j*n+m];
            //A[i*n+j]=0;
        }
    }
}

// -----------------------------------------------------------
// ----------------------------------------------- LOAD & SAVE
// -----------------------------------------------------------
bool ofxPatch::loadFile(string _filePath, string _configFile){
    bool loaded = false;
    
    if (_configFile != "none")
        configFile = _configFile;
    
    ofFile file = ofFile(_filePath);
    
    filePath = file.getAbsolutePath();
    string ext = file.getExtension();
    
    // Load General setup variables
    //
    ofxXmlSettings XML;
    if (XML.loadFile(configFile)){
        if (XML.getValue("general:fullscreen", false ) == true){
            width = ofGetScreenWidth();
            height = ofGetScreenHeight();
        } else {
            width = XML.getValue("general:width", 640 );
            height = XML.getValue("general:height", 480 );
        }
    }
    
    if ((ext == "jpg") || (ext == "JPG") ||
        (ext == "jpeg")|| (ext == "JPEG")||
        (ext == "png") || (ext == "PNG") ||
        (ext == "gif") || (ext == "GIF") ||
        (ext == "bmp") || (ext == "BMP") ){
        
        type    = "ofImage";
        
        if ( image != NULL )
            delete image;
        
        image   = new ofImage();
        loaded  = image->loadImage( filePath );
        width   = image->getWidth();
        height  = image->getHeight();
        
        
        // Setting Inspector
        //
//        imageSrc = _filePath;
//        inspector = new ofxUICanvas();
//        inspector->addLabel("INSPECTOR");
//        inspector->addSpacer();
//        inspector->addLabel("Image src:");
//        ofxUITextInput* ti = inspector->addTextInput("Image src", imageSrc);
//        ti->setDraggable(false);
//        inspector->setWidgetPosition(OFX_UI_WIDGET_POSITION_RIGHT);
//        inspector->addImageButton("Image src btn", "assets/edit.png", false);
//        inspector->autoSizeToFitWidgets();
//        ofAddListener(inspector->newGUIEvent,this,&ofxPatch::guiEvent);
//        inspector->setVisible(false);
        
    } else if ((ext == "mov") || (ext == "MOV") ||
               (ext == "mpg") || (ext == "MPG") ||
               (ext == "mp4") || (ext == "MP4") ||
               (ext == "m4v") || (ext == "M4V") ){
        
        type    = "ofVideoPlayer";
        
        if ( videoPlayer != NULL )
            delete videoPlayer;
        
        videoPlayer   = new ofVideoPlayer();
        loaded  = videoPlayer->loadMovie( filePath );
        videoPlayer->play();
        width   = videoPlayer->getWidth();
        height  = videoPlayer->getHeight();
        
    } else if ((ext == "frag") || (ext == "FRAG") ||
               (ext == "fs") || (ext == "FS") ){
        
        ofBuffer content = ofBufferFromFile( filePath );
        
        type    = "ofShader";
        
        if ( shader != NULL )
            delete shader;
        
        shader  = new ofxShaderObj();
        shader->allocate(width,height);
        loaded  = shader->setFragmentShader(content.getText());
        
        inPut.clear();
        for ( int i = 0; i < shader->getNumberOfTextures();  i++){
            LinkDot p;
            inPut.push_back(p);
        }
        
    } else if ((ext == "lut") || (ext == "LUT") ||
               (ext == "cube") || (ext == "CUBE")){
        
        type = "ofTexture";
        
//        if ( texture != NULL )
//            delete texture;
        
        ofBuffer buffer = ofBufferFromFile( filePath );
        
        int mapSize = 32;
        width = mapSize * mapSize;
        height = mapSize;
        
        float * pixels = new float[mapSize * mapSize * mapSize * 3];
        
//        texture = new ofTexture();
        tex.allocate( width, height, GL_RGB32F);
        for(int z=0; z<mapSize ; z++){
            for(int y=0; y<mapSize; y++){
                for(int x=0; x<mapSize; x++){
                    string content = buffer.getNextLine();
                    int pos = x + y*height + z*width;
                    
                    vector <string> splitString = ofSplitString(content, " ", true, true);
                    
                    if (splitString.size() >=3) {
                        pixels[pos*3 + 0] = ofToFloat(splitString[0]);
                        pixels[pos*3 + 1] = ofToFloat(splitString[1]);
                        pixels[pos*3 + 2] = ofToFloat(splitString[2]);
                    }
                }
            }
        }
        tex.loadData( pixels, width, height, GL_RGB);
        
        loaded = (buffer.size() > 0)? true: false;
        
    } else
        ofLog(OF_LOG_ERROR, "ofxComposer: ofxPatch: can«t load file " + filePath + " Extention " + ext + " not know" );
    
    if ( loaded ){
        
        float offSet = 0.0;
        
        if (title != NULL)
            offSet = 15;
        
        if (type == "ofxGLEditor"){
            textureCorners[0].set(0.0, height + offSet);
            textureCorners[1].set(width, height + offSet);
            textureCorners[2].set(width, offSet);
            textureCorners[3].set(0.0, offSet);
        } else {
            textureCorners[0].set(0.0, offSet);
            textureCorners[1].set(width, offSet);
            textureCorners[2].set(width, height + offSet);
            textureCorners[3].set(0.0, height + offSet);
        }
        
        title->setTitle( file.getFileName() );
        
        bUpdateMask = true;
        bUpdateCoord = true;
    }
    
    return loaded;
}

//------------------------------------------------------------------
bool ofxPatch::loadType(string _type, string _configFile){
    bool loaded = false;
    
    if (_configFile != "none")
        configFile = _configFile;
    
    // Load General setup variables
    //
    ofxXmlSettings XML;
    if (XML.loadFile(configFile)){
        if (XML.getValue("general:fullscreen", false ) == true){
            width = ofGetScreenWidth();
            height = ofGetScreenHeight();
        } else {
            width = XML.getValue("general:width", 640 );
            height = XML.getValue("general:height", 480 );
        }
    }
    
    if ( _type == "ofxGLEditor"){
        type = _type;
        title->setTitle( type );
        loaded = true;
    } else if ( _type == "ofVideoGrabber"){
        type = _type;
        videoGrabber = new ofVideoGrabber();
        videoGrabber->setDeviceID( 0 );
        
        title->setTitle(ofToString(nId) + ":" + type );
        loaded = videoGrabber->initGrabber(width, height);
        
        if (loaded){
            width = videoGrabber->getWidth();
            height = videoGrabber->getHeight();
        }
    } else if (_type == "ofShader"){
        type  = _type;
        
        if ( shader != NULL )
            delete shader;
        
        shader  = new ofxShaderObj();
        shader->allocate(width,height);
        loaded = true;
        inPut.clear();
        for ( int i = 0; i < shader->getNumberOfTextures();  i++){
            LinkDot p;
            inPut.push_back(p);
        }
    }
    
    if ( loaded ){
        
        float offSet = 0.0;
        
        if (title != NULL)
            offSet = 15;
        
        if (type == "ofxGLEditor"){
            textureCorners[0].set(0.0, height + offSet);
            textureCorners[1].set(width, height + offSet);
            textureCorners[2].set(width, offSet);
            textureCorners[3].set(0.0, offSet);
        } else {
            textureCorners[0].set(0.0, offSet);
            textureCorners[1].set(width, offSet);
            textureCorners[2].set(width, height + offSet);
            textureCorners[3].set(0.0, height + offSet);
        }
        
        title->setTitle( type );
        
        bUpdateMask = true;
        bUpdateCoord = true;
    }
    
    return loaded;
}

//------------------------------------------------------------------
bool ofxPatch::loadSettings( int _nTag, string _configFile){
    bool loaded = false;
    
    ofxXmlSettings XML;
    
    if (_configFile != "none")
        configFile = _configFile;
    
    if (XML.loadFile(configFile)){
        maskCorners.clear();
        
        if (XML.getValue("general:fullscreen", false ) == true){
            width = ofGetScreenWidth();
            height = ofGetScreenHeight();
        } else {
            width = XML.getValue("general:width", 640 );
            height = XML.getValue("general:height", 480 );
        }
        
        if (XML.pushTag("surface", _nTag)){
            
            // Load the type and do what it have to
            //
            nId = XML.getValue("id", 0);
            type = XML.getValue("type","none");
            bVisible = XML.getValue("visible", true);
            string path = XML.getValue("path", "none" );
            filePath = path;
            
            //  Except the ofVideoGrabber that it«s a device instead of a file
            //  and ofShader that it will read the data stored on the XML
            //  the rest it been load by the loadFile function
            //
            if ( type == "ofVideoGrabber" ){
                videoGrabber = new ofVideoGrabber();
                videoGrabber->setDeviceID( XML.getValue("path", 0 ) );
                
                width = XML.getValue("width", width);
                height = XML.getValue("height", height);
                
                title->setTitle(ofToString(nId) + ":" + type );
                loaded = videoGrabber->initGrabber(width, height);
                
                if (loaded){
                    width = videoGrabber->getWidth();
                    height = videoGrabber->getHeight();
                }
            } else if ( type == "ofShader" ){
                shader = new ofxShaderObj();
                width = XML.getValue("width", width);
                height = XML.getValue("height", height);
                shader->allocate(width, height, XML.getValue("format", GL_RGBA));
                shader->setPasses( XML.getValue("passes", 1) );
                
                loaded = shader->setFragmentShader( XML.getValue("frag", "Error: data not found...") );
                
                if ( loaded ){
                    inPut.clear();
                    for ( int i = 0; i < shader->getNumberOfTextures();  i++){
                        LinkDot p;
                        inPut.push_back(p);
                    }
                    
                    ofFile file;
                    file.open(path);
                    title->setTitle(ofToString(nId) + ":" +  file.getFileName() );
                }
            } else if( type == "ofxGLEditor" ){
                title->setTitle(ofToString(nId) + ":" + type );
                loaded = true;
            } else if (( type == "ofImage") || ( type == "ofTexture") || ( type == "ofVideoPlayer")){
                loaded = loadFile( path );
            } else {
                title->setTitle(ofToString(nId) + ":" + type );
                loaded = true;
            }
            
            if (loaded){
                // Load the texture coorners
                //
                if (XML.pushTag("texture")){
                    for(int i = 0; i < 4; i++){
                        if (XML.pushTag("point",i)){
                            textureCorners[i].set(XML.getValue("x", 0.0),XML.getValue("y", 0.0));
                            XML.popTag();
                        }
                    }
                    XML.popTag();
                }
                
                // Load the mask path
                if ( XML.pushTag("mask") ){
                    int totalMaskCorners = XML.getNumTags("point");
                    if (totalMaskCorners > 0){
                        maskCorners.clear();
                    }
                    
                    for(int i = 0; i < totalMaskCorners; i++){
                        XML.pushTag("point",i);
                        maskCorners.addVertex( XML.getValue("x", 0.0),XML.getValue("y", 0.0));
                        XML.popTag(); // Pop "point"
                    }
                    XML.popTag(); // Pop "mask"
                    
                    if ( maskCorners.getVertices().size() == 4 ){
                        if ((maskCorners.getVertices()[0] == ofPoint(0.0,0.0)) &&
                            (maskCorners.getVertices()[1] == ofPoint(1.0,0.0)) &&
                            (maskCorners.getVertices()[2] == ofPoint(1.0,1.0)) &&
                            (maskCorners.getVertices()[3] == ofPoint(0.0,1.0)) )
                            bMasking = false;
                        else
                            bMasking = true;
                    } else {
                        bMasking = true;
                    }
                }
                
                bUpdateMask = true;
                bUpdateCoord = true;
                
                XML.popTag(); // Pop Surface
            }
        }
    } else
        ofLog(OF_LOG_ERROR,"ofxComposer: loading patch n¼ " + ofToString(nId) + " on " + configFile );
    
    return loaded;
}

//------------------------------------------------------------------
bool ofxPatch::saveSettings(string _configFile){
    bool saved = false;
    
    ofxXmlSettings XML;
    
    if (_configFile != "none")
        configFile = _configFile;
    
    // Open the configfile
    //
    if (XML.loadFile(configFile)){
        
        // If it«s the first time it«s saving the information
        // the nId it«s going to be -1 and it«s not going to be
        // a place that holds the information. It«s that the case:
        //
        //  1- Search for the first free ID abailable number
        //
        //  2- Make the structure of the path that hold the information
        //
        
        if (nId == -1){
            // Find a free id number for this object
            //
            int totalSurfaces = XML.getNumTags("surface");
            int freeId = 0;
            for (int i = 0; i < totalSurfaces; i++){
                if (XML.pushTag("surface", i)){
                    if ( XML.getValue("id",-1) == freeId ){
                        freeId++;
                        i = 0;
                    }
                    XML.popTag();
                }
            }
            
            if ( freeId >= 0){
                nId = freeId;
                
                // Insert a new surface tag at the end of the xml
                // and fill it with the proper structure
                //
                int lastPlace = XML.addTag("surface");
                if (XML.pushTag("surface", lastPlace)){
                    
                    XML.addTag("id");
                    XML.setValue("id", nId);
                    XML.addTag("type");
                    XML.setValue("type", type);
                    XML.addTag("path");
                    XML.setValue("path", filePath);
                    XML.addTag("visible");
                    XML.setValue("visible", bVisible);
                    
                    // For the moment the shader string it's the only one
                    // with an extra parametter.
                    //
                    if ( shader != NULL) {
                        XML.addTag("frag");
                        XML.setValue("frag", shader->getFragmentShader() );
                        XML.addTag("format");
                        XML.setValue("format", shader->getInternalFormat() );
                        XML.addTag("passes");
                        XML.setValue("passes", shader->getPasses() );
                    }
                    
                    // Texture Corners
                    //
                    XML.addTag("texture");
                    if (XML.pushTag("texture")){
                        for(int i = 0; i < 4; i++){
                            
                            XML.addTag("point");
                            
                            XML.setValue("point:x",textureCorners[i].x, i);
                            XML.setValue("point:y",textureCorners[i].y, i);
                        }
                        XML.popTag();// Pop "texture"
                    }
                    
                    // Mask Path
                    //
                    XML.addTag("mask");
                    if (XML.pushTag("mask")){
                        for(int i = 0; i < 4; i++){
                            XML.addTag("point");
                            
                            XML.setValue("point:x",maskCorners[i].x, i);
                            XML.setValue("point:y",maskCorners[i].y, i);
                        }
                        XML.popTag();// Pop "mask"
                    }
                    
                    // Save out linked dots
                    //
                    XML.addTag("out");
                    XML.setValue("out:active",1);
                    if(XML.pushTag("out")){
                        int totalSavedLinks = XML.getNumTags("dot");
                        
                        for(int j = 0; j < outPut.size(); j++){
                            int tagNum = j;
                            
                            // If need more tags
                            // add them
                            //
                            if (j >= totalSavedLinks)
                                tagNum = XML.addTag("dot");
                            
                            XML.setValue("dot:to", outPut[j].toId , tagNum);
                            XML.setValue("dot:tex", outPut[j].nTex, tagNum);
                        }
                        XML.popTag();// pop "out"
                        saved = XML.saveFile();
                    }
                    
                    XML.popTag();// pop "surface"
                }
            }
            
            // This is necesary for making the initial matrix, mask FBO, mask path and texture corners.
            //
            if (saved){
                ofLog(OF_LOG_NOTICE, "The patch have been asigned with ID " + ofToString(nId) + " and save the information" );
            }
            
        } else {
            
            // If the ID it's different from -1 means that it was already
            // find an ID and a place to hold the information so it'll
            // proceed with a normma save process
            //
            
            // Get the total number of surfaces...
            //
            int totalSurfaces = XML.getNumTags("surface");
            
            // ... and search for the right id for loading
            //
            for (int i = 0; i < totalSurfaces; i++){
                if (XML.pushTag("surface", i)){
                    
                    // Once it found the right surface that match the id ...
                    //
                    if ( XML.getValue("id", -1) == nId){
                        
                        // General information
                        //
                        XML.setValue("path", filePath );
                        XML.setValue("visible", bVisible);
                        
                        if (  shader != NULL ){
                            // Shader specific
                            //
                            XML.setValue("frag", shader->getFragmentShader() );
                            XML.setValue("format", shader->getInternalFormat() );
                            XML.setValue("passes", shader->getPasses() );
                        }
                        
                        // Position of the texture coorners
                        //
                        if (XML.pushTag("texture")){
                            for(int i = 0; i < 4; i++){
                                XML.setValue("point:x",textureCorners[i].x, i);
                                XML.setValue("point:y",textureCorners[i].y, i);
                            }
                            XML.popTag(); // pop "texture"
                        }
                        
                        // Mask path
                        //
                        if (XML.pushTag("mask")){
                            int totalSavedPoints = XML.getNumTags("point");
                            
                            for(int j = 0; j < maskCorners.size(); j++){
                                int tagNum = j;
                                
                                if (i >= totalSavedPoints)
                                    tagNum = XML.addTag("point");
                                
                                XML.setValue("point:x",maskCorners[j].x, tagNum);
                                XML.setValue("point:y",maskCorners[j].y, tagNum);
                            }
                            
                            int totalCorners = maskCorners.size();
                            totalSavedPoints = XML.getNumTags("point");
                            
                            if ( totalCorners < totalSavedPoints){
                                for(int j = totalSavedPoints; j > totalCorners; j--){
                                    XML.removeTag("point",j-1);
                                }
                            }
                            XML.popTag(); // pop "mask"
                        }
                        
                        // Save out linked dots
                        //
                        if(XML.pushTag("out")){
                            int totalSavedLinks = XML.getNumTags("dot");
                            
                            for(int j = 0; j < outPut.size(); j++){
                                int tagNum = j;
                                
                                // If need more tags
                                // add them
                                //
                                if (j >= totalSavedLinks)
                                    tagNum = XML.addTag("dot");
                                
                                XML.setValue("dot:to", outPut[j].toId , tagNum);
                                XML.setValue("dot:tex", outPut[j].nTex, tagNum);
                            }
                            
                            // If there are too much tags
                            // delete them
                            //
                            int totalOutPuts = outPut.size();
                            if ( totalOutPuts < totalSavedLinks){
                                for(int j = totalSavedLinks; j > totalOutPuts; j--){
                                    XML.removeTag("dot",j-1);
                                }
                            }
                            XML.popTag(); // pop "out"
                        }
                        
                        // Once it finish save
                        //
                        saved = XML.saveFile();
                    }
                    XML.popTag(); // pop "surface"
                }
            }
        }
    } else
        ofLog(OF_LOG_ERROR, "Couldn't save the patch ID " + ofToString(nId) + ", the file " + configFile + " was not found");
    
    return saved;

}

//------------------------------------------------------------------
bool ofxPatch::saveSettings(ofxXmlSettings &XML, bool _new, int _nTag){
    bool saved = false;
                
    // If is not new node ...
    // I need to update my information in the .xml
    //
    if (!_new){
        
        // General information
        //
        XML.setValue("path", filePath );
        XML.setValue("visible", bVisible);
        
        if (  shader != NULL ){
            // Shader specific
            //
            XML.setValue("frag", shader->getFragmentShader() );
            XML.setValue("format", shader->getInternalFormat() );
            XML.setValue("passes", shader->getPasses() );
        }
        
        // Position of the texture coorners
        //
        if (XML.pushTag("texture")){
            for(int i = 0; i < 4; i++){
                XML.setValue("point:x",textureCorners[i].x, i);
                XML.setValue("point:y",textureCorners[i].y, i);
            }
            XML.popTag(); // pop "texture"
        }
        
        // Mask path
        //
        if (XML.pushTag("mask")){
            int totalSavedPoints = XML.getNumTags("point");
            
            for(int j = 0; j < maskCorners.size(); j++){
                int tagNum = j;
                
                if (j >= totalSavedPoints)
                    tagNum = XML.addTag("point");
                
                XML.setValue("point:x",maskCorners[j].x, tagNum);
                XML.setValue("point:y",maskCorners[j].y, tagNum);
            }
            
            int totalCorners = maskCorners.size();
            totalSavedPoints = XML.getNumTags("point");
            
            if ( totalCorners < totalSavedPoints){
                for(int j = totalSavedPoints; j > totalCorners; j--){
                    XML.removeTag("point",j-1);
                }
            }
            XML.popTag(); // pop "mask"
        }
        
        // Save out linked dots
        //
        if(XML.pushTag("out")){
            int totalSavedLinks = XML.getNumTags("dot");
            
            for(int j = 0; j < outPut.size(); j++){
                int tagNum = j;
                
                // If need more tags
                // add them
                //
                if (j >= totalSavedLinks)
                    tagNum = XML.addTag("dot");
                
                XML.setValue("dot:to", outPut[j].toId , tagNum);
                XML.setValue("dot:tex", outPut[j].nTex, tagNum);
            }
            
            // If there are too much tags
            // delete them
            //
            int totalOutPuts = outPut.size();
            if ( totalOutPuts < totalSavedLinks){
                for(int j = totalSavedLinks; j > totalOutPuts; j--){
                    XML.removeTag("dot",j-1);
                }
            }
            XML.popTag(); // pop "out"
        }
        
        // Once it finish save
        //
        saved = XML.saveFile();
    }

    // If it was the last node in the XML and it wasn't me..
    // I need to add myself in the .xml file
    //
    else {
        
        // Insert a new NODE tag at the end
        // and fill it with the proper structure
        //
            
        XML.addTag("type");
        XML.setValue("type", type);
        XML.addTag("path");
        XML.setValue("path", filePath);
        XML.addTag("visible");
        XML.setValue("visible", bVisible);
        
        // For the moment the shader string it's the only one
        // with an extra parametter.
        //
        if ( shader != NULL) {
            XML.addTag("frag");
            XML.setValue("frag", shader->getFragmentShader() );
            XML.addTag("format");
            XML.setValue("format", shader->getInternalFormat() );
            XML.addTag("passes");
            XML.setValue("passes", shader->getPasses() );
        }
        
        // Texture Corners
        //
        XML.addTag("texture");
        if (XML.pushTag("texture")){
            for(int i = 0; i < 4; i++){
                
                XML.addTag("point");
                
                XML.setValue("point:x",textureCorners[i].x, i);
                XML.setValue("point:y",textureCorners[i].y, i);
            }
            XML.popTag();// Pop "texture"
        }
        
        // Mask Path
        //
        XML.addTag("mask");
        if (XML.pushTag("mask")){
            for(int i = 0; i < 4; i++){
                XML.addTag("point");
                
                XML.setValue("point:x",maskCorners[i].x, i);
                XML.setValue("point:y",maskCorners[i].y, i);
            }
            XML.popTag();// Pop "mask"
        }
        
        // Save out linked dots
        //
        XML.addTag("out");
        XML.setValue("out:active",1);
        if(XML.pushTag("out")){
            int totalSavedLinks = XML.getNumTags("dot");
            
            for(int j = 0; j < outPut.size(); j++){
                int tagNum = j;
                
                // If need more tags
                // add them
                //
                if (j >= totalSavedLinks)
                    tagNum = XML.addTag("dot");
                
                XML.setValue("dot:to", outPut[j].toId , tagNum);
                XML.setValue("dot:tex", outPut[j].nTex, tagNum);
            }
            XML.popTag();// pop "out"
            saved = XML.saveFile();
        }
    }
    
    // This is necesary for making the initial matrix, mask FBO, mask path and texture corners.
    //
    if (saved){
        ofLog(OF_LOG_NOTICE, "The patch have been asigned with ID " + ofToString(nId) + " and save the information" );
    }
    
    return saved;
}

// -----------------------------------------------------------
// ------------------------------------------------- SNNIPPETS
// -----------------------------------------------------------
bool ofxPatch::loadSnippetPatch(string snippetName, int relativeId, int previousPatchesSize) {
    bool loaded = false;
    
    ofxXmlSettings XML;
    
    if (XML.loadFile(snippetName)){
        maskCorners.clear();
        width = XML.getValue("general:width", 640 );
        height = XML.getValue("general:height", 480 );
        
        if (XML.pushTag("surface", relativeId)){
            
            // Load the type and do what it have to
            //
            nId = XML.getValue("id", 0) + previousPatchesSize;
            type = XML.getValue("type","none");
            bVisible = XML.getValue("visible", true);
            string path = XML.getValue("path", "none" );
            filePath = path;
            
            //  Except the ofVideoGrabber that it«s a device instead of a file
            //  and ofShader that it will read the data stored on the XML
            //  the rest it been load by the loadFile function
            //
            if ( type == "ofVideoGrabber" ){
                videoGrabber = new ofVideoGrabber();
                videoGrabber->setDeviceID( XML.getValue("path", 0 ) );
                
                title->setTitle(ofToString(nId) + ":" + type );
                loaded = videoGrabber->initGrabber(width, height);
                
                if (loaded){
                    width = videoGrabber->getWidth();
                    height = videoGrabber->getHeight();
                }
            } else if ( type == "ofShader" ){
                shader = new ofxShaderObj();
                shader->allocate(width, height, XML.getValue("format", GL_RGBA));
                shader->setPasses( XML.getValue("passes", 1) );
                
                loaded = shader->setFragmentShader( XML.getValue("frag", "Error: data not found...") );
                
                if ( loaded ){
                    inPut.clear();
                    for ( int i = 0; i < shader->getNumberOfTextures();  i++){
                        LinkDot p;
                        inPut.push_back(p);
                    }
                    
                    ofFile file;
                    file.open(path);
                    title->setTitle(ofToString(nId) + ":" +  file.getFileName() );
                }
            } else if( type == "ofxGLEditor" ){
                title->setTitle(ofToString(nId) + ":" + type );
                loaded = true;
            } else if (( type == "ofImage") || ( type == "ofTexture") || (type == "ofVideoPlayer")){
                loaded = loadFile( path );
            } else {
                title->setTitle(ofToString(nId) + ":" + type );
                loaded = true;
            }
            
            if (loaded){
                // Load the texture coorners
                //
                if (XML.pushTag("texture")){
                    for(int i = 0; i < 4; i++){
                        if (XML.pushTag("point",i)){
                            textureCorners[i].set(XML.getValue("x", 0.0),XML.getValue("y", 0.0));
                            XML.popTag();
                        }
                    }
                    XML.popTag();
                }
                
                // Load the mask path
                if ( XML.pushTag("mask") ){
                    int totalMaskCorners = XML.getNumTags("point");
                    if (totalMaskCorners > 0){
                        maskCorners.clear();
                    }
                    
                    for(int i = 0; i < totalMaskCorners; i++){
                        XML.pushTag("point",i);
                        maskCorners.addVertex( XML.getValue("x", 0.0),XML.getValue("y", 0.0));
                        XML.popTag(); // Pop "point"
                    }
                    XML.popTag(); // Pop "mask"
                    
                    if ( maskCorners.getVertices().size() == 4 ){
                        if ((maskCorners.getVertices()[0] == ofPoint(0.0,0.0)) &&
                            (maskCorners.getVertices()[1] == ofPoint(1.0,0.0)) &&
                            (maskCorners.getVertices()[2] == ofPoint(1.0,1.0)) &&
                            (maskCorners.getVertices()[3] == ofPoint(0.0,1.0)) )
                            bMasking = false;
                        else
                            bMasking = true;
                    } else {
                        bMasking = true;
                    }
                }
                
                bUpdateMask = true;
                bUpdateCoord = true;
                
                XML.popTag(); // Pop Surface
            }
        }
    } else
        ofLog(OF_LOG_ERROR,"ofxComposer: loading patch n¼ " + ofToString(nId) + " on " + snippetName );
    
    return loaded;
}

//------------------------------------------------------------------
bool ofxPatch::saveSnippetPatch(string snippetName, map<int, int> idMap, ofxXmlSettings XML) {
    bool saved = false;
    
    if (!bActive) {
        return;
    }

    int lastPlace = XML.addTag("surface");
    if (XML.pushTag("surface", lastPlace)){
        
        XML.addTag("id");
        XML.setValue("id", idMap[nId]);
        XML.addTag("type");
        XML.setValue("type", type);
        XML.addTag("path");
        XML.setValue("path", filePath);
        XML.addTag("visible");
        XML.setValue("visible", bVisible);
        
        // For the moment the shader string it's the only one
        // with an extra parametter.
        //
        if ( shader != NULL) {
            XML.addTag("frag");
            XML.setValue("frag", shader->getFragmentShader() );
            XML.addTag("format");
            XML.setValue("format", shader->getInternalFormat() );
            XML.addTag("passes");
            XML.setValue("passes", shader->getPasses() );
        }
        
        // Texture Corners
        //
        XML.addTag("texture");
        if (XML.pushTag("texture")){
            for(int i = 0; i < 4; i++){
                
                XML.addTag("point");
                
                XML.setValue("point:x",textureCorners[i].x, i);
                XML.setValue("point:y",textureCorners[i].y, i);
            }
            XML.popTag();// Pop "texture"
        }
        
        // Mask Path
        //
        XML.addTag("mask");
        if (XML.pushTag("mask")){
            for(int i = 0; i < 4; i++){
                XML.addTag("point");
                
                XML.setValue("point:x",maskCorners[i].x, i);
                XML.setValue("point:y",maskCorners[i].y, i);
            }
            XML.popTag();// Pop "mask"
        }
        
        // Save out linked dots
        //
        XML.addTag("out");
        XML.setValue("out:active",1);
        if(XML.pushTag("out")){
            int totalSavedLinks = XML.getNumTags("dot");
            
            for(int j = 0; j < outPut.size(); j++){
                int tagNum = j;
                
                // If need more tags
                // add them
                //
                if (j >= totalSavedLinks)
                    tagNum = XML.addTag("dot");
                
                XML.setValue("dot:to", idMap[outPut[j].toId] , tagNum);
                XML.setValue("dot:tex", idMap[outPut[j].nTex], tagNum);
            }
            XML.popTag(); // pop "out"
            saved = XML.saveFile();
        }
        
        XML.popTag(); // pop "surface"
    }
    
    // This is necesary for making the initial matrix, mask FBO, mask path and texture corners.
    //
    if (saved){
        ofLog(OF_LOG_NOTICE, "The patch have been asigned with ID " + ofToString(idMap[nId]) + " and save the information" );
    }
    
    return saved;
}

/* ================================================ */
/* ================================================ */





