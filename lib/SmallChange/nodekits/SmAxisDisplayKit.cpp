/**************************************************************************\
 *
 *  This file is part of the SmallChange extension library for Coin.
 *  Copyright (C) 1998-2003 by Systems in Motion.  All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  ("GPL") version 2 as published by the Free Software Foundation.
 *  See the file LICENSE.GPL at the root directory of this source
 *  distribution for additional information about the GNU GPL.
 *
 *  For using SmallChange with software that can not be combined with the
 *  GNU GPL, and for taking advantage of the additional benefits of our
 *  support services, please contact Systems in Motion about acquiring
 *  a Coin Professional Edition License.
 *
 *  See <URL:http://www.coin3d.org> for  more information.
 *
 *  Systems in Motion, Teknobyen, Abels Gate 5, 7030 Trondheim, NORWAY.
 *  <URL:http://www.sim.no>.
 *
\**************************************************************************/

/*!
  \class SmAxisDisplayKit SmAxisDisplayKit.h
  \brief ...
  \ingroup nodekits

*/

#include "SmAxisDisplayKit.h"

#include <Inventor/SbRotation.h>
#include <Inventor/SbViewVolume.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoBaseColor.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoMaterialBinding.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoRotation.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/nodes/SoCone.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoCallback.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/sensors/SoOneShotSensor.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <SmallChange/nodes/DepthBuffer.h>
#include <SmallChange/nodes/ViewportRegion.h>

// used to store private (hidden) data members
class SmAxisDisplayKitP {
public:
  SmAxisDisplayKitP(SmAxisDisplayKit * master) : master(master) { }

  SmAxisDisplayKit * master;

  SbBool processingoneshot;
  SoOneShotSensor * oneshot;
  static void oneshot_cb(void * closure, SoSensor * s);
  static void callback_cb(void * userdata, SoAction * action);
};

SO_KIT_SOURCE(SmAxisDisplayKit);

#define PRIVATE(obj) (obj)->pimpl

/*!
  Constructor.
*/
SmAxisDisplayKit::SmAxisDisplayKit(void) 
{
  PRIVATE(this) = new SmAxisDisplayKitP(this);
  PRIVATE(this)->oneshot = new SoOneShotSensor(SmAxisDisplayKitP::oneshot_cb, 
                                               PRIVATE(this));
  PRIVATE(this)->processingoneshot = FALSE;

  SO_KIT_CONSTRUCTOR(SmAxisDisplayKit);

  SO_KIT_ADD_FIELD(orientation, (SbRotation(SbVec3f(0.0f, 0.0f, 1.0f), 0.0f)));
  SO_KIT_ADD_FIELD(axes, (0.0f, 0.0f, 0.0f));
  SO_KIT_ADD_FIELD(colors, (0.0f, 0.0f, 0.0f));
  SO_KIT_ADD_FIELD(enableArrows, (FALSE));
  SO_KIT_ADD_FIELD(annotations, (""));

  this->axes.setNum(0);
  this->axes.setDefault(TRUE);
  this->colors.setNum(0);
  this->colors.setDefault(TRUE);
  this->enableArrows.setNum(0);
  this->enableArrows.setDefault(TRUE);
  this->annotations.setNum(0);
  this->annotations.setDefault(TRUE);

  SO_KIT_ADD_CATALOG_ENTRY(topSeparator, SoSeparator, FALSE, this, "", FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(callback, SoCallback, FALSE, topSeparator, camera, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(camera, SoPerspectiveCamera, FALSE, topSeparator, depthBuffer, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(depthBuffer, DepthBuffer, FALSE, topSeparator, viewportRegion, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(viewportRegion, ViewportRegion, FALSE, topSeparator, drawstyle, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(drawstyle, SoDrawStyle, FALSE, topSeparator, lightmodel, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(lightmodel, SoLightModel, FALSE, topSeparator, matbinding, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(matbinding, SoMaterialBinding, FALSE, topSeparator, axessep, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(axessep, SoSeparator, FALSE, topSeparator, "", FALSE);

  SO_KIT_INIT_INSTANCE();

  // Setup default values
  SoCallback * cb = (SoCallback *)this->getAnyPart("callback", TRUE);
  cb->setCallback(SmAxisDisplayKitP::callback_cb, PRIVATE(this));

  DepthBuffer * db = (DepthBuffer *)this->getAnyPart("depthBuffer", TRUE);
  db->enable = FALSE;

  ViewportRegion * vpr = (ViewportRegion *)this->getAnyPart("viewportRegion", TRUE);
  vpr->origin.setValue(1.0f, 0.0f);
  vpr->size.setValue(0.3f, 0.3f);

  SoDrawStyle *drawstyle = (SoDrawStyle *)this->getAnyPart("drawstyle", TRUE);
  drawstyle->lineWidth = 2;

  SoLightModel *lightmodel = (SoLightModel *)this->getAnyPart("lightmodel", TRUE);
  lightmodel->model = SoLightModel::BASE_COLOR;

  SoMaterialBinding *matbinding = 
    (SoMaterialBinding *)this->getAnyPart("matbinding", TRUE);
  matbinding->value = SoMaterialBinding::PER_PART;
}

/*!
  Destructor.
*/
SmAxisDisplayKit::~SmAxisDisplayKit()
{
  delete PRIVATE(this)->oneshot;
  delete PRIVATE(this);
}

/*!
  Initializes this class. Call before using it.
*/
void
SmAxisDisplayKit::initClass(void)
{
  static int first = 1;
  if (first) {
    first = 0;
    SO_KIT_INIT_CLASS(SmAxisDisplayKit, SoBaseKit, "BaseKit");
  }
}

void 
SmAxisDisplayKit::getBoundingBox(SoGetBoundingBoxAction * action)
{
  SoCacheElement::invalidate(action->getState());
  SoNode::getBoundingBox(action);
}

// overloader to test when stuff changes in the API fields
void 
SmAxisDisplayKit::notify(SoNotList * l)
{
  if (!PRIVATE(this)->oneshot->isScheduled() && 
      !PRIVATE(this)->processingoneshot) {
    SoField * f = l->getLastField();
    if (f) {
      SoFieldContainer * c = f->getContainer();
      // test if a field in this nodekit changed, but only trigger on
      // normal fields, not on part fields.
      if (c == this && f->getTypeId() != SoSFNode::getClassTypeId()) {
        // trigger a sensor that will recalculate all internal data
        PRIVATE(this)->oneshot->schedule();
      }
    }
  }
  inherited::notify(l);
}

/*!
  Overloaded to set most node fields to default. The parts in this
  nodekit are mostly internal.
*/
void 
SmAxisDisplayKit::setDefaultOnNonWritingFields(void)
{
  SoFieldList fl;
  (void) this->getFields(fl);
  for (int i = 0; i < fl.getLength(); i++) {
    SoField * f = fl[i];
    if (f->isOfType(SoSFNode::getClassTypeId())) f->setDefault(TRUE);
  }  
  inherited::setDefaultOnNonWritingFields();
}

#undef PRIVATE
#define PUBLIC(obj) (obj)->master

// called when something has changed and the internal list needs to be
// regenerated.
void 
SmAxisDisplayKitP::oneshot_cb(void * closure, SoSensor * s)
{
  SmAxisDisplayKitP * thisp = (SmAxisDisplayKitP*) closure;
  thisp->processingoneshot = TRUE;

  // Update private scene graph
  
  SoSeparator *axessep = 
    (SoSeparator *)PUBLIC(thisp)->getAnyPart("axessep", TRUE);
  axessep->removeAllChildren();

  SoSeparator *geomsep = NULL;
  for (int i=0;i<PUBLIC(thisp)->axes.getNum();i++) {
    SoSeparator *axissep = new SoSeparator;
    SoBaseColor *axiscol = new SoBaseColor;
    if (PUBLIC(thisp)->colors.getNum() <= i)
      axiscol->rgb.setValue(PUBLIC(thisp)->colors[PUBLIC(thisp)->colors.getNum()-1]);
    else
      axiscol->rgb.setValue(PUBLIC(thisp)->colors[i]);

    SbRotation rot(SbVec3f(0.0f, 1.0f, 0.0f), PUBLIC(thisp)->axes[i]);
    SoRotation *axistrans = new SoRotation;
    axistrans->rotation = rot;

    if (!geomsep) {
      geomsep = new SoSeparator;
      SoCoordinate3 *geomcoord = new SoCoordinate3;
      SoLineSet *geomls = new SoLineSet;
      SoTranslation *geomtrans = new SoTranslation;
      SoCone *geomcone = new SoCone;

      geomcoord->point.set1Value(0, 0.0f, 0.0f, 0.0f);
      geomcoord->point.set1Value(1, 0.0f, 1.0f, 0.0f);
      geomls->numVertices.setValue(2);
      geomtrans->translation.setValue(0.0f, 1.125f, 0.0f);
      geomcone->height = 0.25f;
      geomcone->bottomRadius = 0.0833f;

      geomsep->addChild(geomcoord);
      geomsep->addChild(geomls);
      geomsep->addChild(geomtrans);
      geomsep->addChild(geomcone);
    }

    axissep->addChild(axiscol);
    axissep->addChild(axistrans);
    axissep->addChild(geomsep);
    axessep->addChild(axissep);
  }

  SoPerspectiveCamera *camera = 
    (SoPerspectiveCamera *)PUBLIC(thisp)->getAnyPart("camera", TRUE);
  camera->orientation.connectFrom(&PUBLIC(thisp)->orientation);
  camera->nearDistance = 0.01f;
  camera->farDistance = 10.0f;
  camera->heightAngle = 22.5*M_PI/180; // Reduce perspective

  if (thisp->oneshot->isScheduled()) thisp->oneshot->unschedule();
  thisp->processingoneshot = FALSE;;
}

void 
SmAxisDisplayKitP::callback_cb(void * userdata, SoAction * action)
{
  SmAxisDisplayKitP * thisp = (SmAxisDisplayKitP*) userdata;
  if (action->isOfType(SoGLRenderAction::getClassTypeId())) {
    SoPerspectiveCamera *camera = 
      (SoPerspectiveCamera *)PUBLIC(thisp)->getAnyPart("camera", TRUE);

    SbRotation rot = PUBLIC(thisp)->orientation.getValue();
    SbVec3f dir;
    rot.multVec(SbVec3f(0.0f, 0.0f, 1.0f), dir);
    camera->position.setValue(6.0f*dir); // Just a good guess
  }
}
