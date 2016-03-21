//
//  ofxPatchDeleteEvent.h
//  nimp
//
//  Created by Mili Garicoits on 3/20/16.
//
//

#ifndef ofxPatchDeleteEvent_h
#define ofxPatchDeleteEvent_h

class ofxPatchDeleteEvent : ofEventArgs {
    
public:
    int   patchId;
    int   deleteOutputId;
};

#endif /* ofxPatchDeleteEvent_h */
