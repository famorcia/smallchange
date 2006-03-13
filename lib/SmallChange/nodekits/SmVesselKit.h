/**************************************************************************\
 *
 *  Copyright (C) 1998-2004 by Systems in Motion. All rights reserved.
 *
\**************************************************************************/

#ifndef VESSELKIT_H
#define VESSELKIT_H

#include "SmDynamicObjectKit.h"

#include <Inventor/nodekits/SoSubKit.h>
#include <Inventor/nodekits/SoBaseKit.h>
#include <Inventor/fields/SoSFVec2f.h>
#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/fields/SoSFNode.h>
#include <Inventor/fields/SoSFTime.h>
#include <Inventor/C/basic.h>
#include <Inventor/SbVec3f.h>
#include <SmallChange/basic.h>


class SmVesselKitP;

class SMALLCHANGE_DLL_API SmVesselKit : public SmDynamicObjectKit {
  typedef SmDynamicObjectKit inherited;
  
  SO_KIT_HEADER(SmVesselKit);
  
public:
  SmVesselKit(void);
  
  SoSFNode oceanKit;
  SoSFVec2f size;
  SoSFFloat speed;
  SoSFTime lastDatumTime;
  SoSFFloat maxExtrapolationTime;
  SoSFFloat pitchInertia;
  SoSFFloat pitchResistance;
  SoSFFloat pitchBalance;
  SoSFFloat rollInertia;
  SoSFFloat rollResistance;
  SoSFFloat rollBalance;

  static void initClass(void);
  
protected:
  virtual ~SmVesselKit();

public:
  
  virtual void GLRender(SoGLRenderAction * action);

private:
  
  SmVesselKitP * pimpl;

};

#endif // !VESSELKIT_H

