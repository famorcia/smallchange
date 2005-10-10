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
  \class SoLODExtrusion SoLODExtrusion.h
  \brief Extrusion node with auto level of detail per spine segment

  Renders an extrusion of \e crossSection along \e spine, for
  all \e spine segments closer to camera than \e lodDistance1. Between
  \e lodDistance1 and \e lodDistance2 , \e spine segments are rendered
  as simple lines. Beyond \e lodDistance2 nothing is rendered. 
  To render lines to infinity, set \e lodDistance2 to less than zero.
  (the default value for \e lodDistance2 is -1.0).

  The node currently renders the extrusion with per vertex normal
  vectors. Per (spine) vertex material is not yet supported, nor
  are texture coordinates.

  \sa SoVRMLExtrusion
*/

/*! \var SoSFBool SoLODExtrusion::ccw
  Whether or not \e crossSection vertices are ordered counter-clockwise
  or not.
 */

/*! \var SoSFFloat SoLODExtrusion::creaseAngle
  Crease angle for normal vector calculation.
 */

/*! \var SoMFVec2f SoLODExtrusion::crossSection
  Cross section of the extrusion, given as a list of 2D vertices.
 */

/*! \var SoMFVec3f SoLODExtrusion::spine
  The central spine of the extrusion, given as a list of 3D
  vertices.
 */

/*! \var SoMFVec3f SoLODExtrusion::color
  If color.getNum() == spine.getNum(), the extrusion is colored
  per spine point.
 */

/*! \var SoSFFloat SoLODExtrusion::radius
  If radius is set to a value greater than zero, \e crossSection
  will be set to represent a circle of radius \e radius. Any other
  setting of \e crossSection is ignored as long as \e radius is
  greater than zero. Default value for \e radius is -1.0.
 */

/*! \var SoSFInt32 SoLODExtrusion::circleSegmentCount
  How many line segments will be used to represent a circular
  cross section. Relevant only when \e radius is greater than
  zero.
 */

/*! \var SoSFFloat SoLODExtrusion::lodDistance1
  The distance from camera where extrusion rendering is replaced
  by line rendering.
 */

/*! \var SoSFFloat SoLODExtrusion::lodDistance2
  The distance from camera where line rendering will be replaced
  by nothing, if \e lodDistance2 is greater than zero. If 
  \e lodDistance2 is less than zero, lines will be rendered to
  infinity (or to the end of the spine, or to the far clipping
  plane, whichever is closer to camera). Default value for
  \e lodDistance2 is -1.0.
 */

/*!
  \var SoSFVec3f SoLODExtrusion::zAxis
  
  Makes it possible to lock the extrusion Z-axis to always have this 
  value. The extrusion coordinate system will need to be orthonormal,
  of course, so the Z-axis will be transformed to accout for this.
  Default value is (0, 0, 0), which means that the Z-axis is not 
  locked.
*/

#include "SoLODExtrusion.h"
// #include <Inventor/nodes/SoSubNodeP.h>
#include <Inventor/misc/SoNormalGenerator.h>
#include <Inventor/bundles/SoMaterialBundle.h>
#include <Inventor/bundles/SoTextureCoordinateBundle.h>
#include <Inventor/elements/SoGLCoordinateElement.h>
#include <Inventor/elements/SoTextureCoordinateElement.h>
#include <Inventor/elements/SoGLTextureEnabledElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/SbLine.h>
#include <Inventor/details/SoPointDetail.h>
#include <Inventor/details/SoLineDetail.h>
#include <Inventor/details/SoFaceDetail.h>
#include <float.h>
#include <math.h>

#include "../misc/SbList.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#ifdef __COIN__
#include <Inventor/system/gl.h>
#else // SGI/TGS Inventor
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif // HAVE_WINDOWS_H
#include <GL/gl.h>
#endif // SGI/TGS Inventor

//
// needed to avoid warnings generated by SbVec3f::normalize
//
static float
my_normalize(SbVec3f & vec)
{
  float len = vec.length();
  if (len > FLT_EPSILON) {
    vec /= len;
  }
  return len;
}


#ifndef DOXYGEN_SKIP_THIS
class SoLODExtrusionP {
public:
  SoLODExtrusionP(SoLODExtrusion * master)
    :master(master),
     coord(32),
     tcoord(32),
     idx(32),
     color_idx(32),
     segidx(32),
     striplens(32),
     dirty(TRUE)
  {
  }

  SoLODExtrusion * master;
  SbList <SbVec3f> coord;
  SbList <SbVec3f> normals;
  SbList <SbVec2f> tcoord;
  SbList <int> idx;     // vertex index
  SbList <int> color_idx;     // per coord color index (into master->color), matches coord list
  SbList <int> segidx;  // index into idx, for each spine segment
  SbList <int32_t> striplens;  // lengths of tri-strips
  SbList <float> spinelens;    // geometric length of each spine segment, precalculated
  SbBool dirty;

  void generateCoords(void);
  void renderSegidx( const int, const SbBool);
  void makeCircleCrossSection( const float, const int);
};
#endif // DOXYGEN_SKIP_THIS

#undef THIS
#define THIS this->pimpl


SO_NODE_SOURCE(SoLODExtrusion);

void
SoLODExtrusion::initClass(void) // static
{
  SO_NODE_INIT_CLASS(SoLODExtrusion, SoShape, "SoShape");
}

SoLODExtrusion::SoLODExtrusion(void)
{
  THIS = new SoLODExtrusionP(this);

  SO_NODE_CONSTRUCTOR(SoLODExtrusion);

  SO_NODE_ADD_FIELD(ccw, (FALSE));
  SO_NODE_ADD_FIELD(creaseAngle, (0.0f));

  SO_NODE_ADD_FIELD(crossSection, (0.0f, 0.0f));
  this->crossSection.setNum(5);
  SbVec2f * cs = this->crossSection.startEditing();
  cs[0] = SbVec2f(1.0f, 1.0f);
  cs[1] = SbVec2f(1.0f, -1.0f);
  cs[2] = SbVec2f(-1.0f, -1.0f);
  cs[3] = SbVec2f(-1.0f, 1.0f);
  cs[4] = SbVec2f(1.0f, 1.0f);
  this->crossSection.finishEditing();
  this->crossSection.setDefault(TRUE);

  SO_NODE_ADD_FIELD(spine, (0.0f, 0.0f, 0.0f));
  this->spine.setNum(2);
  this->spine.set1Value(1, 0.0f, 1.0f, 0.0f);
  this->spine.setDefault(TRUE);

  SO_NODE_ADD_FIELD(color, (1.0f, 0.0f, 0.0f));

  SO_NODE_ADD_FIELD(radius, (-1.0f));
  SO_NODE_ADD_FIELD(circleSegmentCount, (10));
  SO_NODE_ADD_FIELD(lodDistance1, (1000.0));
  SO_NODE_ADD_FIELD(lodDistance2, (-1.0));   // default lines to infinity
  SO_NODE_ADD_FIELD(zAxis, (0.0f, 0.0f, 0.0f));
  SO_NODE_ADD_FIELD(pickLines, (FALSE));

  THIS->dirty = TRUE;
}

SoLODExtrusion::~SoLODExtrusion()
{
  delete THIS;
}


void
SoLODExtrusion::GLRender(SoGLRenderAction * action)
{
  if (!this->shouldGLRender(action)) return;
  SoState * state = action->getState();

  this->updateCache();

  SoMaterialBundle mb(action);
  mb.sendFirst();

  // Find camera position in local coordinate space
  SbVec3f cameralocal;
  const SbMatrix & tempmat = SoModelMatrixElement::get(state);
  const SbMatrix matrix = tempmat.inverse();
  const SbViewVolume & vv = SoViewVolumeElement::get(state);
  matrix.multVecMatrix(vv.getProjectionPoint(), cameralocal);

  const SbVec3f * sv = this->spine.getValues(0);
  int spinelength = this->spine.getNum();

  SbBool use_color;
  if( spinelength == this->color.getNum())
    use_color = TRUE;
  else
    use_color = FALSE;

  // check for closed spine
  if (sv[0] == sv[spinelength-1]) {
    spinelength--;
  }


  const SbVec3f * colorv = this->color.getValues(0);
  const float * lengths = this->pimpl->spinelens.getArrayPtr();
  int lodmode, i = 0;
  float dist, ld1dist, ld2dist, accdist;


  while (i < spinelength-1) {
    int oldi = i;
    accdist = 0.0;
    dist = (cameralocal - sv[i]).length();
    ld1dist = SbAbs(dist - this->lodDistance1.getValue());
    ld2dist = SbAbs(dist - this->lodDistance2.getValue());
    if (dist < this->lodDistance1.getValue()) {
      lodmode = 0;
    }
    else if (this->lodDistance2.getValue() > 0.0) {
      if (dist < this->lodDistance2.getValue()) {
        lodmode = 1;
      }
      else { 
        lodmode = 2;
      }
    }
    else {
      lodmode = 1;
    }
    switch (lodmode) {
    case 0:    // render extrusion until above lodDistance1
      while (accdist < ld1dist && i < spinelength-1) {
        THIS->renderSegidx(i, use_color);
        accdist += lengths[i];
        i++;
      }
      break;
    case 1:    // render line until crossing a lodDistance
      {
        SbBool wasenabled = glIsEnabled(GL_LIGHTING);
        if (wasenabled) glDisable(GL_LIGHTING);
        glBegin(GL_LINE_STRIP);
        while (accdist < ld1dist && accdist < ld2dist && i < spinelength-1) {
          if( use_color )
            glColor3fv((const GLfloat*)colorv[i].getValue());
          glVertex3fv((const GLfloat*)sv[i].getValue());
          accdist += lengths[i];
          i++;
        }
        if( use_color )
          glColor3fv((const GLfloat*)colorv[i].getValue());
        glVertex3fv((const GLfloat*)sv[i].getValue());
        glEnd();
        if (wasenabled) glEnable(GL_LIGHTING);
      }
      break;
    case 2:    // render nothing until beneath lodDistance2
      while (accdist < ld2dist && i < spinelength-1) {
	accdist += lengths[i];
	i++;
      }
      break;
    default:
      assert(0);
      i = spinelength + 1;  // exit loop on error
      break;
    }
    // to avoid hangs
    if (i == oldi) i++;
  } 
  SoGLCacheContextElement::shouldAutoCache(action->getState(),
                                           SoGLCacheContextElement::DONT_AUTO_CACHE);

}

void 
SoLODExtrusion::rayPick(SoRayPickAction * action)
{
  if (!shouldRayPick(action)) return;

  SoState * state = action->getState();

  // Find camera position in local coordinate space
  SbVec3f cameralocal;
  const SbMatrix & tempmat = SoModelMatrixElement::get(state);
  const SbMatrix matrix = tempmat.inverse();
  const SbViewVolume & vv = SoViewVolumeElement::get(state);
  matrix.multVecMatrix(vv.getProjectionPoint(), cameralocal);
  
  action->setObjectSpace();
  const SbLine & ray = action->getLine();

  const float r = this->radius.getValue();

  int num = this->spine.getNum();
  const SbVec3f * sptr = this->spine.getValues(0);

  if (sptr[0] == sptr[num-1]) {
    num--;
  }

  float d2 = this->lodDistance1.getValue();
  if (this->pickLines.getValue()) {
    d2 = this->lodDistance2.getValue();
  }
  if (d2 < 0.0f) {
    d2 = FLT_MAX;
  }
  else {
    d2 *= d2; // square it
  }
  float r2 = r*r;

  SoPointDetail pd0;
  SoPointDetail pd1;

  for (int i = 0; i < num-1; i++) {
    SbVec3f v0 = sptr[i];
    SbVec3f v1 = sptr[i+1];

    if (v0 != v1) {
      float l1 = (v0-cameralocal).sqrLength();
      float l2 = (v1-cameralocal).sqrLength();
      
      if (l1 < d2 || l2 < d2) {
        SbLine line(v0, v1);
        SbVec3f op0, op1; // object space
        if (ray.getClosestPoints(line, op0, op1)) {
          // clamp op1 between v0 and v1
          if ((op1-v0).dot(line.getDirection()) < 0.0f) op1 = v0;
          else if ((v1-op1).dot(line.getDirection()) < 0.0f) op1 = v1;
          
          if ((op1-op0).sqrLength() <= r2 && action->isBetweenPlanes(op0)) {

            // adjust picked point to account for radius of the extrusion

            // FIXME: for now we just place the picked point on top of the extrusion
            op0[2] += this->radius.getValue();

            SoPickedPoint * pp = action->addIntersection(op0);
            if (pp) {
              pd0.setCoordinateIndex(i);
              pd1.setCoordinateIndex(i+1);
              SoLineDetail * detail = new SoLineDetail;
              detail->setPoint0(&pd0);
              detail->setPoint1(&pd1);
              detail->setLineIndex(i);
              pp->setDetail(detail, this);
            }
          }
        }
      }
    }
  }
}

void
SoLODExtrusion::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  this->updateCache();
  action->addNumTriangles(THIS->idx.getLength() / 4);
}

void
SoLODExtrusion::computeBBox(SoAction * action,
                             SbBox3f & box,
                             SbVec3f & center)
{
  this->updateCache();

  int num = THIS->coord.getLength();
  const SbVec3f * coords = THIS->coord.getArrayPtr();

  box.makeEmpty();
  while (num--) {
    box.extendBy(*coords++);
  }

  num = this->spine.getNum();
  coords = this->spine.getValues(0);
  while(num--) {
    box.extendBy(*coords++);
  }

  if (!box.isEmpty()) center = box.getCenter();
}

void
SoLODExtrusion::generatePrimitives(SoAction * action)
{
  this->updateCache();
  
  // FIXME: generate triangles

  /*
  const SbVec3f * normals = THIS->gen.getNormals();
  const SbVec2f * tcoords = THIS->tcoord.getArrayPtr();
  const SbVec3f * coords = THIS->coord.getArrayPtr();
  const int32_t * iptr = THIS->idx.getArrayPtr();
  const int32_t * endptr = iptr + THIS->idx.getLength();

  SoPrimitiveVertex vertex;
  SoFaceDetail faceDetail;
  SoPointDetail pointDetail;

  vertex.setDetail(&pointDetail);

  int idx;
  int nidx = 0;
  this->beginShape(action, TRIANGLES, &faceDetail);
  while (iptr < endptr) {
    idx = *iptr++;
    while (idx >= 0) {
      pointDetail.setNormalIndex(nidx++);
      pointDetail.setCoordinateIndex(idx);
      vertex.setNormal(*normals++);
      vertex.setTextureCoords(tcoords[idx]);
      vertex.setPoint(coords[idx]);
      this->shapeVertex(&vertex);
      idx = *iptr++;
    }
  }
  this->endShape();
  */
}

void
SoLODExtrusion::updateCache(void)
{
  if (THIS->dirty) {
    THIS->generateCoords();
    THIS->dirty = FALSE;
  }
}

/*!
  Overloaded to disable geometry cache.
*/
void
SoLODExtrusion::notify(SoNotList * list)
{
  THIS->dirty = TRUE;
  inherited::notify(list);
}


SoDetail * 
SoLODExtrusion::createTriangleDetail(SoRayPickAction * action,
                                      const SoPrimitiveVertex * v1,
                                      const SoPrimitiveVertex * v2,
                                      const SoPrimitiveVertex * v3,
                                      SoPickedPoint * pp)
{
  // no triangle detail for Extrusion
  return NULL;
}

#undef THIS
#ifndef DOXYGEN_SKIP_THIS

void
SoLODExtrusionP::generateCoords(void)
{
  this->coord.truncate(0);
  this->color_idx.truncate(0);
  this->tcoord.truncate(0);
  this->idx.truncate(0);
  this->segidx.truncate(0);
  this->striplens.truncate(0);
  this->spinelens.truncate(0);
  this->normals.truncate(0);

  // Create circular cross section if radius > 0.0
  if (this->master->radius.getValue() > 0.0) {
    this->makeCircleCrossSection(this->master->radius.getValue(),
                                 this->master->circleSegmentCount.getValue());
  }
  if (this->master->crossSection.getNum() == 0 ||
      this->master->spine.getNum() == 0) return;
  
  SbMatrix matrix = SbMatrix::identity();

  int i, j, numcross;
  SbBool connected = FALSE;   // is cross section closed
  SbBool closed = FALSE;      // is spine closed
  numcross = this->master->crossSection.getNum();
  const SbVec2f * cross =  master->crossSection.getValues(0);
  if (cross[0] == cross[numcross-1]) {
    connected = TRUE;
    numcross--;
  }

  int numspine = master->spine.getNum();
  const SbVec3f * spine = master->spine.getValues(0);
  if (spine[0] == spine[numspine-1]) {
    closed = TRUE;
    numspine--;
  }

  SbVec3f zaxis = this->master->zAxis.getValue();
  SbBool dolockz = zaxis != SbVec3f(0.0f, 0.0f, 0.0f);
  if (dolockz) zaxis.normalize();

  SbVec3f X, Y, Z;
  SbVec3f prevX(1.0f, 0.0f, 0.0f);
  SbVec3f prevY(0.0f, 1.0f, 0.0f);
  SbVec3f prevZ(0.0f, 0.0f, 1.0f);

  //  int numorient = this->master->orientation.getNum();
  //  const SbRotation * orient = this->master->orientation.getValues(0);
  int numorient = 0;
  const SbRotation * orient = NULL;

  //  int numscale = this->master->scale.getNum();
  //  const SbVec2f * scale = this->master->scale.getValues(0);
  int numscale = 0;
  const SbVec2f * scale = NULL;

  int reversecnt = 0;

  // loop through all spines
  for (i = 0; i < numspine; i++) {
    if (i < numspine-1) {
      this->spinelens.append( (spine[i+1]-spine[i]).length() );
    }
    if (closed) {
      if (i > 0)
        Y = spine[i+1] - spine[i-1];
      else
        Y = spine[1] - spine[numspine-1];
    }
    else {
      if (i == 0) Y = spine[1] - spine[0];
      else if (i == numspine-1) Y = spine[numspine-1] - spine[numspine-2];
      else Y = spine[i+1] - spine[i-1];
    }

    if (my_normalize(Y) <= FLT_EPSILON) {
      if (prevY[1] < 0.0f)
        Y = SbVec3f(0.0f, -1.0f, 0.0f);
      else
        Y = SbVec3f(0.0f, 1.0f, 0.0f);
    }
    
    if (dolockz) {
      Z = zaxis;
      X = Y.cross(Z);
      X.normalize();
      Z = X.cross(Y);
      Z.normalize();
    }
    else {
      SbVec3f z0, z1;
      
      if (closed) {
        if (i > 0) {
          z0 = spine[i+1] - spine[i];
          z1 = spine[i-1] - spine[i];
        }
        else {
          z0 = spine[1] - spine[0];
          z1 = spine[numspine-1] - spine[0];
        }
      }
      else {
        if (numspine == 2) {
          z0 = SbVec3f(1.0f, 0.0f, 0.0f);
          z1 = Y;
          Z = z0.cross(z1);
          if (my_normalize(Z) <= FLT_EPSILON) {
            z0 = SbVec3f(0.0f, 1.0f, 0.0f);
            z1 = Y;
          }
        }
        else if (i == 0) {
          z0 = spine[2] - spine[1];
          z1 = spine[0]-spine[1];
        }
        else if (i == numspine-1) {
          z0 = spine[numspine-1] - spine[numspine-2];
          z1 = spine[numspine-3]-spine[numspine-2];
        }
        else {
          z0 = spine[i+1] - spine[i];
          z1 = spine[i-1] - spine[i];
        }
      }
      
      my_normalize(z0);
      my_normalize(z1);
      
      // test if spine segments are (almost) parallel. If they are, the
      // cross product will not be reliable, and we should just use the
      // previous Z-axis instead.
      if (z0.dot(z1) < -0.999f) {
        Z = prevZ;
      }
      else {
        Z = z0.cross(z1);
      }
      
      if (my_normalize(Z) <= FLT_EPSILON || SbAbs(Y.dot(Z)) > 0.5f) {
        Z = SbVec3f(0.0f, 0.0f, 0.0f);
        
        int bigy = 0;
        float bigyval = Y[0];
        if (SbAbs(Y[1]) > SbAbs(bigyval)) {
          bigyval = Y[1];
          bigy = 1;
        }
        if (SbAbs(Y[2]) > SbAbs(bigyval)) {
          bigy = 2;
          bigyval = Y[2];
        }
        Z[(bigy+1)%3] = bigyval > 0.0f ? 1.0f : -1.0f;
        
        // make Z perpendicular to Y
        X = Y.cross(Z);
        my_normalize(X);
        Z = X.cross(Y);
        my_normalize(Z);
      }
      
      X = Y.cross(Z);
      my_normalize(X);
      
      if (i > 0 && (Z.dot(prevZ) <= 0.5f || X.dot(prevX) <= 0.5f)) {
        // if change is fairly large, try to find the most appropriate
        // axis. This will minimize change from spine-point to
        // spine-point
        SbVec3f v[4];
        v[0] = X;
        v[1] = -X;
        v[2] = Z;
        v[3] = -Z;
        
        float maxdot = v[0].dot(prevZ);
        int maxcnt = 0;
        for (int cnt = 1; cnt < 4; cnt++) {
          float dot = v[cnt].dot(prevZ);
          if (dot > maxdot) {
            maxdot = dot;
            maxcnt = cnt;
          }
        }
        Z = v[maxcnt];
        X = Y.cross(Z);
        my_normalize(X);
      }
    }

    prevX = X;
    prevY = Y;
    prevZ = Z;

    matrix[0][0] = X[0];
    matrix[0][1] = X[1];
    matrix[0][2] = X[2];
    matrix[0][3] = 0.0f;

    matrix[1][0] = Y[0];
    matrix[1][1] = Y[1];
    matrix[1][2] = Y[2];
    matrix[1][3] = 0.0f;

    matrix[2][0] = Z[0];
    matrix[2][1] = Z[1];
    matrix[2][2] = Z[2];
    matrix[2][3] = 0.0f;

    matrix[3][0] = spine[i][0];
    matrix[3][1] = spine[i][1];
    matrix[3][2] = spine[i][2];
    matrix[3][3] = 1.0f;

    int cnt = 0;
    if (X[0] < 0.0f) cnt++;
    if (X[1] < 0.0f) cnt++;
    if (X[2] < 0.0f) cnt++;
    if (Y[0] < 0.0f) cnt++;
    if (Y[1] < 0.0f) cnt++;
    if (Y[2] < 0.0f) cnt++;
    if (Z[0] < 0.0f) cnt++;
    if (Z[1] < 0.0f) cnt++;
    if (Z[2] < 0.0f) cnt++;

    if (cnt & 1) reversecnt--;
    else reversecnt++;

    if (numorient) {
      SbMatrix rmat;
      orient[SbMin(i, numorient-1)].getValue(rmat);
      matrix.multLeft(rmat);
    }

    if (numscale) {
      SbMatrix smat = SbMatrix::identity();
      SbVec2f s = scale[SbMin(i, numscale-1)];
      smat[0][0] = s[0];
      smat[2][2] = s[1];
      matrix.multLeft(smat);
    }

    for (j = 0; j < numcross; j++) {
      SbVec3f c;
      c[0] = cross[j][0];
      c[1] = 0.0f;
      c[2] = cross[j][1];

      matrix.multVecMatrix(c, c);

      // calc vertex normal
      SbVec3f t(0.0f, 0.0f, 0.0f); // assumes cross section is centered in (0,0)
      matrix.multVecMatrix(t, t);
      this->coord.append(c);
      c -= t;
      c.normalize();
      this->normals.append(c);
      this->tcoord.append(SbVec2f(float(j)/float(numcross-1),
                                  float(i)/float(closed ? numspine : numspine-1)));
      this->color_idx.append(i);
    }
  }

  // this macro makes the code below more readable
#define ADD_TRIANGLE(i0, j0, i1, j1, i2, j2) \
  do { \
    if (reversecnt < 0) { \
      this->idx.append((i2)*numcross+(j2)); \
      this->idx.append((i1)*numcross+(j1)); \
      this->idx.append((i0)*numcross+(j0)); \
      this->idx.append(-1); \
    } \
    else { \
      this->idx.append((i0)*numcross+(j0)); \
      this->idx.append((i1)*numcross+(j1)); \
      this->idx.append((i2)*numcross+(j2)); \
      this->idx.append(-1); \
    } \
  } while (0)

#define ADD_VERTEX(i0, j0) \
  do { \
    this->idx.append((i0)*numcross+(j0)); \
  } while (0)

  int cursegidx = 0;
  // create walls
  for (i = 0; i < numspine-1; i++) {
    // Create walls
    assert(cursegidx == this->idx.getLength());
    this->segidx.append( cursegidx );
    for (j = 0; j < numcross; j++) {
      ADD_VERTEX(i, j);
      ADD_VERTEX(i+1, j);
      cursegidx += 2;
    }
    if (connected) {
      ADD_VERTEX(i, 0);
      ADD_VERTEX(i+1, 0);
      cursegidx += 2;
      this->striplens.append( 2 + numcross * 2 );
    } 
    else {
      this->striplens.append( numcross * 2 );
    }
    //  this->idx.append(-1);
    //  cursegidx += 1;
  }
  this->segidx.append(cursegidx);
#undef ADD_TRIANGLE
#undef ADD_VERTEX
  //  printf("generateCoords: done, segidx length = %d \n", this->segidx.getLength() );
}

//
// Render triangles for segidx[index]
// Assumes per vertex normals, no materials or texture coords
//
void 
SoLODExtrusionP::renderSegidx( const int index, const SbBool use_color )
{
  assert( index >=0 && index < this->segidx.getLength() );

  const int * siv = this->segidx.getArrayPtr();
  const int * iv = this->idx.getArrayPtr();
  const SbVec3f * cv = this->coord.getArrayPtr();
  const SbVec3f * nv = this->normals.getArrayPtr();
  const SbColor * colorv = this->master->color.getValues(0);
  const int * coloridx = this->color_idx.getArrayPtr();
  int vcnt = this->coord.getLength();
  int startindex = siv[index];
  int stopindex;
  if (index < segidx.getLength()-1)
    stopindex = siv[index+1];
  else
    stopindex = idx.getLength();

  assert(stopindex > startindex);

  int curidx = startindex;
  int32_t v1, v2, v3, v4, v5 = 0; // v5 init unnecessary, but kills a compiler warning.
  
  if( iv[curidx+3] < 0) {  /* Triangle */
    assert(0 && "should never get here");
    //    printf("renderSegidx: triangles. \n");
    glBegin(GL_TRIANGLES);
    while( curidx < stopindex ) {
      v1 = iv[curidx++];
      v2 = iv[curidx++];
      v3 = iv[curidx++];
      assert(v1 >= 0 && v2 >= 0 && v3 >= 0);
      assert(v1 < vcnt && v2 < vcnt && v3 < vcnt);
      v4 = iv[curidx++];
      assert( v4 < 0);  /* Triangle means every 4th index = -1  */
      
      /* vertex 1 *********************************************************/
      if( use_color )
	glColor3fv((const GLfloat*)colorv[coloridx[v1]].getValue());
      glNormal3fv((const GLfloat*)nv[v1].getValue());
      glVertex3fv((const GLfloat*)cv[v1].getValue());
      
      /* vertex 2 *********************************************************/
      if( use_color )
	glColor3fv((const GLfloat*)colorv[coloridx[v2]].getValue());
      glNormal3fv((const GLfloat*)nv[v2].getValue());
      glVertex3fv((const GLfloat*)cv[v2].getValue());
      
      /* vertex 3 *********************************************************/
      if( use_color )
	glColor3fv((const GLfloat*)colorv[coloridx[v3]].getValue());
      glNormal3fv((const GLfloat*)nv[v3].getValue());
      glVertex3fv((const GLfloat*)cv[v3].getValue());
    }
    glEnd(); /* draw triangles */
  } 
  else {   /* Tristrip(s) */
    glBegin(GL_TRIANGLE_STRIP);
    while( curidx < stopindex ) {
      assert(curidx < this->idx.getLength());
      v1 = iv[curidx];
      assert(v1 >= 0 && v1 < vcnt);
      if( use_color ) {
	glColor3fv((const GLfloat*)colorv[coloridx[v1]].getValue());
      }
      glNormal3fv((const GLfloat*)nv[v1].getValue());
      glVertex3fv((const GLfloat*)cv[v1].getValue());
      curidx++;
    }
    glEnd(); /* draw tristrip*/
  }
}

void
SoLODExtrusionP::makeCircleCrossSection(const float radius, const int segments)
{
  SbList <SbVec2f> templist;
  templist.truncate(0);
  float angle = (float) (2.0 * M_PI / float(segments));
  templist.append( SbVec2f( 0.0, radius ) );
  for( int i=1; i < segments; i++) {
    templist.append( SbVec2f( radius*sin(float(i)*angle), radius*cos(float(i)*angle)) );
  }
  templist.append( SbVec2f( 0.0, radius ) );

  // disable notify so that we don't send notification messages while
  // rendering (not good for multipipe rendering).
  SbBool old = this->master->crossSection.enableNotify(FALSE);
  this->master->crossSection.setValues( 0, templist.getLength(), templist.getArrayPtr() );
  this->master->crossSection.enableNotify(old);
}

#endif // DOXYGEN_SKIP_THIS

