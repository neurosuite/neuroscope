/***************************************************************************
                          baseframe.h  -  description
                             -------------------
    begin                : Wed Feb 18 2004
    copyright            : (C) 2004 by Lynn Hazan
    email                : lynn.hazan@myrealbox.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef BASEFRAME_H
#define BASEFRAME_H

// include files for QT
#include <QWidget>
#include <QFrame>
#include <QCursor>

#include <QResizeEvent>
#include <QMouseEvent>
#include "klusterrubberband.h"

//include files for the application
#include "zoomwindow.h"

/**
  * Frame base class containing the following features:
  * <ul>
  *  <li>zoom in on a zone selected with a rubber band or on a single point.</li>
  *  <li>zoom out to the original size by double clicking.</li>
  *  <li>coordinates conversion between the world's coordinates system and viewport's coordinates system.</li>
  *  <li>coordinates conversion between the viewport's coordinates system and world's coordinates system.</li>
  *  <li>ability to change the background color.</li>
  *  <li>utilities functions.</li>
  * </ul>
  * @attention This class needs the ZoomWindow class (which provides the zooming calculations),
  * and the zoom_cursor.png for the zoom cursor.
  *@author Lynn Hazan
  */

class BaseFrame : public QFrame  {
    Q_OBJECT

public:

    /**
  * typedef indicating in wich mode the user is.
  */
    typedef int Mode;

    /**Enum to be use as a Mode.
  * The only value provided in this class is ZOOM, indicating that the user is in a mode enabling him to zoom.*/
    enum {ZOOM = 1};

    /**
   * Constructs the view.
   * @param Xborder border on the left and right sides inside the window (QRect corresponding
   * to the part of the drawing which will actually be drawn onto the widget).
   * @param Yborder border on the top and bottom sides inside the window (QRect corresponding
   * to the part of the drawing which will actually be drawn onto the widget).
   * @param parent the parent QWidget.
   * @param name name of the widget (can be used for introspection).
   * @param backgroundColor color used as background. Balck is the default.
   * @param minSize minumum size of the view.
   * @param maxSize maximum size of the view.
   * @param windowTopLeft the top-left corner of the window (QRect corresponding
   * to the part of the drawing which will actually be drawn onto the widget).
   * @param windowBottomRight bottom-right corner of the window (QRect corresponding
   * to the part of the drawing which will actually be drawn onto the widget).
   * @param border size of the border between the frame and the contents.
   */
    BaseFrame(int Xborder,int Yborder,QWidget* parent=0, const QString &name=QString(),const QColor& backgroundColor = Qt::black,
              int minSize = 500, int maxSize = 4000, int windowTopLeft = -500,int windowBottomRight = 1001, int border = 0);
    ~BaseFrame();

    /**Signals that the widget is about to be deleted.*/
    virtual void willBeKilled(){}

    /**Sets the borders of the frame.
   * @param x border on the left and right sides inside the window (QRect corresponding
   * to the part of the drawing which will actually be drawn onto the widget).
   * @param y border on the top and bottom sides inside the window (QRect corresponding
   * to the part of the drawing which will actually be drawn onto the widget).
  */
    void setBorders(int x,int y){
        Xborder = x;
        Yborder = y;
    }

public Q_SLOTS:

    /**Update the information presented in the view if need it.*/
    virtual void updateDrawing(){}

    /**If the frame is contained in a dockWidget, this slot can be used
  * when the enclosing dockwidget is being closed.
  * Emits the parentDockBeingClosed signal.
  */
    virtual void dockBeingClosed(){emit parentDockBeingClosed(this);}

    /**Changes the color of the background.*/
    virtual void changeBackgroundColor(const QColor &color);

    /**Change the current mode, call by a selection of a tool.
  * @param selectedMode new mode of drawing.
  */
    virtual void setMode(BaseFrame::Mode selectedMode){mode = selectedMode;}

Q_SIGNALS:
    /*s*Signals that the enclosing dockwidget is being closed
   * @param viewWidget pointer on the the current object.
   */
    void parentDockBeingClosed(QWidget* viewWidget);

private:
    void changeColor(const QColor & color);

protected:

    /**The view responds to a resize event.
  * @param event resize event.
  */
    virtual void resizeEvent(QResizeEvent* event);

    /**The view responds to a mouse click.
  * @param event mouse release event.
  */
    virtual void mousePressEvent(QMouseEvent* event);

    /**The view responds to a mouse release.
  * @param event mouse event.
  */
    virtual void mouseReleaseEvent(QMouseEvent* event);

    /**The view responds to a mouse move.
  * @param event mouse event.
  */
    virtual void mouseMoveEvent(QMouseEvent* event);

    /**The view responds to a double click.
  * @param event mouse event.
  */
    virtual void mouseDoubleClickEvent(QMouseEvent* event);

    /**
  * Translates a point (@p vx, @p vy) on the viewport to a QPoint in the world
  * @param vx x coordinate of the point in the viewport's coordinates system (relative to the widget).
  * @param vy y coordinate of the point in the viewport's coordinates system (relative to the widget).
  * @return the point on the viewport translated to a point in the world.
  */
    QPoint viewportToWorld(int vx, int vy);

    /**
  * This is an overloaded member function, provided for convenience.
  * It behaves essentially like the above function.
  * @param point point with coordinates relative to the widget (viewport).
  * @return the point on the viewport translated to a point in the world.
  */
    QPoint viewportToWorld(const QPoint& point){
        return viewportToWorld(point.x(), point.y());
    }

    /**
  * Translates a point (@p wx, @p wy) in the world to a QPoint on the viewport (relative to the widget).
  * @param wx x coordinate of the point in the world's coordinates system.
  * @param wy y coordinate of the point in the world's coordinates system.
  * @return the point on the world translated to a point in the viewport.
  */
    QPoint worldToViewport(int wx, int wy);

    /**
  * This is an overloaded member function, provided for convenience.
  * It behaves essentially like the above function.
  * @param point point with coordinates in the world.
  * @return the point on the world translated to a point in the viewport.
  */
    QPoint worldToViewport(const QPoint& point){
        return worldToViewport(point.x(), point.y());
    }

    /** Translates the abscissa @p wx in the world to the abscissa on the viewport (relative to the widget).
  * @param wx abscissa in the world's coordinates system.
  * @return abscissa in the viewport's coordinates system.
  */
    long worldToViewportAbscissa(long wx);

    /** Translates the abscissa @p wy in the world to the abscissa on the viewport (relative to the widget).
  * @param wy ordinate in the world's coordinates system.
  * @return ordinate in the viewport's coordinates system.
  */
    long worldToViewportOrdinate(long wy);


    /**
  * Translates the width @p width in the world to a width on the viewport (relative to the widget).
  * @param width width in the world's coordinates system.
  * @return width in the viewport's coordinates system.
  */
    long worldToViewportWidth(long width){
        float widthRatio = (static_cast<float>(viewport.width())/static_cast<float>(((QRect)window).width()));
        return static_cast<long>(width * widthRatio);
    }

    /**
  * Translates the height @p height in the world to a height on the viewport (relative to the widget).
  * @param height height in the world's coordinates system.
  * @return height in the viewport's coordinates system.
  */
    long worldToViewportHeight(long height){
        float heightRatio = (static_cast<float>(viewport.height())/static_cast<float>(((QRect)window).height()));
        return static_cast<long>(height * heightRatio);
    }

    /**
  * Translates the width @p width on the viewport (relative to the widget) to a width in the world.
  * @param width width in the world's coordinates system.
  * @return width in the viewport's coordinates system.
  */
    long viewportToWorldWidth(long width){
        float widthRatio = (static_cast<float>(((QRect)window).width())/static_cast<float>(viewport.width()));
        return static_cast<long>(width * widthRatio);
    }

    /**
  * Translates the height @p height on the viewport (relative to the widget) to a height in the world.
  * @param height height in the world's coordinates system.
  * @return height in the viewport's coordinates system.
  */
    long viewportToWorldHeight(long height){
        float heightRatio = (static_cast<float>(((QRect)window).height())/static_cast<float>(viewport.height()));
        return static_cast<long>(height * heightRatio);
    }

    /** Sets if a rubber band has to be drawn while the mode is not ZOOM, and how it sould be drawn.
  * @param draw true if a rubber band has to be drawn, false otherwise.
  * @param vertical true if the rubber band has to be drawn on whole the height of the window, false otherwise.
  */
    void drawRubberBand(bool draw,bool vertical = false){
        isRubberBandToBeDrawn = draw;
        wholeHeightRectangle = vertical;
    }

    //Members

    /**
  * Enumeration indicating in wich drawing contents mode the widget is:
  * reuse of the double buffer or redraw the contents into the double buffer
  */
    enum DrawContentsMode{REFRESH=1, UPDATE=2,REDRAW=3};

    const int MIN_SIZE; //Default 500
    const int MAX_SIZE; //Default 4000
    const int BORDER; //Default 0
    const int WINDOW_TOP_LEFT; //Default -500
    const int WINDOW_BOTTOM_RIGHT; //Default 1001

    QSize oldSize;
    QRect viewport;

    /**
  * Object enabling to transform the window which represent the part of the world to be drawn
  */
    ZoomWindow window;
    /**
  * Coordinates of the point where the user has first click
  */
    QPoint firstClick;
    /**
  * Boolean indicating that the user has double clicked
  */
    bool isDoubleClick;
    /**Mode giving the way of drawing the contents of the view*/
    DrawContentsMode drawContentsMode;

    /**
   * Draw mode (selected by the user via a menu, a button or a shortcut).
   */
    Mode mode;

    /**A cursor to represent the zoom state.*/
    QCursor zoomCursor;

    /**Border on the left and right sides inside the window (QRect corresponding
  * to the part of the drawing which will actually be drawn onto the widget).*/
    int Xborder;

    /**Border on the top and bottom sides inside the window (QRect corresponding
  * to the part of the drawing which will actually be drawn onto the widget).*/
    int Yborder;

    /**Boolean indicating if a rubber band has to be drawn while not in ZOOM mode, the default is false.*/
    bool isRubberBandToBeDrawn;

    /**Boolean indicating if  while not in ZOOM mode,
  * the rubber band has to be drawn on whole the height of the window.
  * The default is no.*/
    bool wholeHeightRectangle;

    /**Color use to display the legends.*/
    QColor colorLegend;
    KlusterRubberBand *mRubberBand;

};

#endif
