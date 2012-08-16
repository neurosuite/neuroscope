/***************************************************************************
                          floatrect.h  -  description
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

#ifndef FLOATRECT_H
#define FLOATRECT_H

#include <QRect>
#include <qpoint.h>
#include <math.h>

  /**
  * This class is used to enable zoom feature. It computes the dimensions of the QRect
  * (the window of a Qframe) corresponding to the part of the drawing
  * which will actually be drawn onto the widget.
  * The calculs are made in world coordinates system (the drawing coordinates system).
  * @author Lynn Hazan
  */

class ZoomWindow {

public: 

/**
* Constructor using a QRect
* @param rect initial rectangle.
*/
  ZoomWindow(const QRect& rect);
/**
* Destructor
*/
  ~ZoomWindow();
/**
* Conversion to a QRect
*/
operator QRect() const;

/**
* Zooms the width and the height of the rectangle by @p factor and center it on the specified center (@p centerX, @p centerY).
* @param factor zoom factor
* @param centerX absciss of the center point for the newly zoomed rectangle
* @param centerY ordinate of the center point for the newly zoomed rectangle
* @return boolean indicating if the zoom has been done
*/
bool zoom(float factor, float centerX, float centerY);


/**
* This is an overloaded member function, provided for convenience.
* It behaves essentially like the above function.
* Zoom the rectangle by @p factor and center it on @p center.
* @param factor zoom factor
* @param center center point for the newly zoomed rectangle
* @return boolean indicating if the zoom has been done
*/
inline bool zoom(float factor, const QPoint& center){
  return zoom(factor, center.x(), center.y());
};


/**
* Zooms on the rectangle specified by two diagonal points (firstClickX,firstClickY) and (secondClickX,secondClickY).
* @param firstClickX absciss of the first selected point
* @param firstClickY ordinate of the first selected point
* @param secondClickX absciss of the second selected point
* @param secondClickY ordinate of the first selected point
* @return boolean indicating if the zoom has been done
*/
bool zoom(int firstClickX, int firstClickY, int secondClickX, int secondClickY);

/**
* This is an overloaded member function, provided for convenience.
* It behaves essentially like the above function.
* Zoom on the rectangle specified by two diagonal points @p firstPoint and @p secondPoint.
* @param firstPoint Qpoint of the first selected point
* @param secondPoint Qpoint of the first selected point
* @return boolean indicating if the zoom has been done
*/
inline bool zoom(QPoint firstPoint, QPoint secondPoint) {
   return zoom(firstPoint.x(), firstPoint.y(), secondPoint.x(), secondPoint.y());
};

protected:
/**
* Make sure that the new window remains inside the intial boundaries.
*/
void correctWindow();


private:
/**
* Left coordinate of the rectangle.
*/
float left;
/**
* Right coordinate of the rectangle.
*/
float right;
/**
* Bottom coordinate of the rectangle.
*/
float bottom;
/**
* Top coordinate of the rectangle.
*/
float top;


/**
* Initial left coordinate of the rectangle.
*/
float initialLeft;
/**
* Initial right coordinate of the rectangle.
*/
float initialRight;
/**
* Initial bottom coordinate of the rectangle.
*/
float initialBottom;
/**
* Initial top coordinate of the rectangle.
*/
float initialTop;

/**
* Minimum zoom allowed
*/
static const float MIN_SCALE;

/**
* Maximum zoom allowed
*/
static const float MAX_SCALE;

public:

/**
* Resets the zoomWindow to its initialize state
*/
inline void reset(){
 left   = initialLeft;
 right  = initialRight;
 top    = initialTop;
 bottom = initialBottom;
};

};


#endif
