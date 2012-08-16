/***************************************************************************
                          ZoomWindow.cpp  -  description
                             -------------------
    begin                : Thu Aug 21 2003
    copyright            : (C) 2003 by 
    email                : 

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "zoomwindow.h"

#include <iostream>
using namespace std;

//Assignment of the min and max scale allowed (starting point 100%)
const float ZoomWindow::MIN_SCALE = 1;//0.51;//51%
const float ZoomWindow::MAX_SCALE = 22.73;//227.%

ZoomWindow::ZoomWindow(const QRect& rect){
  
  initialLeft   = rect.left();
  initialRight  = rect.right();
  initialTop    = rect.top();
  initialBottom = rect.bottom();
  
  left    = rect.left();
  right   = rect.right();
  top     = rect.top();
  bottom  = rect.bottom();
}

ZoomWindow::~ZoomWindow(){
}


/**
* Conversion from a ZoomWindow to a QRect
*/
ZoomWindow::operator QRect() const{
  
  //A QRect has a constructor taking 2 Qpoints (topLeft and bottomRight)
  //Those QPoints are created with an explicit cast
  QPoint topLeft(static_cast<int>(left),static_cast<int>(top));
  QPoint bottomRight(static_cast<int>(right),static_cast<int>(bottom));

  return QRect(topLeft,bottomRight);
}


bool ZoomWindow::zoom(float factor, float centerX, float centerY){
  //Store current values.
  float previousLeft   = left;
  float previousRight  = right;
  float previousTop    = top;
  float previousBottom = bottom;
  
  //To enlarge the drawing, the size of the window as to be smaller and to reduce
  //the drawing, the size of the window as to be bigger. Therefore the zoom factor is 1/factor
  float zoomFactor = 1/factor;

  float width  = fabs(right - left);
  float height = fabs(top - bottom);
  
  left   = centerX - zoomFactor * width/2.0;
  right  = centerX + zoomFactor * width/2.0;
  top    = centerY - zoomFactor * height/2.0;
  bottom = centerY + zoomFactor * height/2.0;
  
  //Make sure that the new window remains inside the intial boundaries.
  correctWindow();

  //Verify if the window has really been modified
  return !(left == previousLeft && right == previousRight && top == previousTop && bottom == previousBottom);

}

bool ZoomWindow::zoom(int firstClickX,int firstClickY, int secondClickX,int secondClickY){
  //Store current values.
  float previousLeft   = left;
  float previousRight  = right;
  float previousTop    = top;
  float previousBottom = bottom;
 
  left   = static_cast<float>(qMin(firstClickX,secondClickX));
  right  = static_cast<float>(qMax(firstClickX,secondClickX));
  top    = static_cast<float>(qMin(firstClickY,secondClickY));
  bottom = static_cast<float>(qMax(firstClickY,secondClickY));
  
  //Make sure that the new window remains inside the intial boundaries.
  correctWindow();
  
  //Verify if the window has really been modified
  return !(left == previousLeft && right == previousRight && top == previousTop && bottom == previousBottom);
   
}

void ZoomWindow::correctWindow(){
  if(left < initialLeft) {
   right += initialLeft - left;
   if(right > initialRight) right = initialRight;
   left = initialLeft;
  }
  if(right > initialRight){
   left += initialRight - right;
   if(left < initialLeft) left = initialLeft;
   right = initialRight;    
  } 
  if(bottom > initialBottom) {
    top += initialBottom - bottom;
    if(top < initialTop) top = initialTop;
    bottom = initialBottom;
  }
  if(top < initialTop){
   bottom += initialTop - top;
   if(bottom > initialBottom) bottom = initialBottom;
   top = initialTop;
  }

  float initialWidth = (initialRight-initialLeft);
  float width = (right-left);
  float scaleX = initialWidth/width;
  if(scaleX > MAX_SCALE){
   //increase window width so scaleX will equal MAX_SCALE.
   float deltaX = initialWidth / MAX_SCALE  - width;
   left -= 0.5 * deltaX;
   right += 0.5 * deltaX;
  }

  float initialHeight = (initialBottom-initialTop);
  float height = (bottom-top);
  float scaleY = initialHeight/height;
  if(scaleY > MAX_SCALE){
   //increase window height so scaleY will equal MAX_SCALE.
   float deltaY = initialHeight / MAX_SCALE  - height;
   top -= 0.5 * deltaY;
   bottom += 0.5 * deltaY;
  }

  //Make sure that the new window is still inside the initial boundaries.
  if(left < initialLeft) {
   right += initialLeft - left;
   if(right > initialRight) right = initialRight;
   left = initialLeft;
  }
  if(right > initialRight){
   left += initialRight - right;
   if(left < initialLeft) left = initialLeft;
   right = initialRight;
  }
  if(bottom > initialBottom) {
    top += initialBottom - bottom;
    if(top < initialTop) top = initialTop;
    bottom = initialBottom;
  }
  if(top < initialTop){
   bottom += initialTop - top;
   if(bottom > initialBottom) bottom = initialBottom;
   top = initialTop;
  }
}












