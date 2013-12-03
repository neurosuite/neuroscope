/***************************************************************************
                          positionview.h  -  description
                             -------------------
    begin                : Mon Jul 26 2004
    copyright            : (C) 2004 by Lynn Hazan
    email                : lynn.hazan.myrealbox.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef POSITIONVIEW_H
#define POSITIONVIEW_H

// include files for Qt
#include <QWidget>
#include <QPixmap>
#include <QImage>
#include <QPainter>
#include <QPrinter>
#include <QHash>

#include <QResizeEvent>
#include <QList>


// application specific includes
#include <baseframe.h>
#include "positionsprovider.h"
#include <types.h>
#include "eventdata.h"
#include "globaleventsprovider.h"

class ItemColors;

/**Class which draws the positions.
  *@author Lynn Hazan
  */

class PositionView : public BaseFrame  {
    Q_OBJECT
public:
    /**Constructor.
  * @param provider a reference on the provider of the position data.
  * @param globalEventProvider a reference on the global event provider.
  * @param backgroundImage image used as background.
  * @param start starting time in miliseconds.
  * @param timeFrameWidth time window in miliseconds.
  * @param showEvents 1 if events are displayed in the view, 0 otherwise.
  * @param windowTopLeft the top-left corner of the window (QRect corresponding
  * to the part of the drawing which will actually be drawn onto the widget).
  * @param windowBottomRight bottom-right corner of the window (QRect corresponding
  * to the part of the drawing which will actually be drawn onto the widget).
  * @param parent the parent QWidget.
  * @param name name of the widget (can be used for introspection).
  * @param backgroundColor color used as background.
  * @param minSize minumum size of the view.
  * @param maxSize maximum size of the view.
  * @param border size of the border between the frame and the contents.
  */
    explicit PositionView(PositionsProvider& provider, GlobalEventsProvider& globalEventProvider, const QImage &backgroundImage, long start, long timeFrameWidth, bool showEvents, int windowTopLeft = -500, int windowBottomRight = 1001, QWidget *parent=0, const char *name=0,
                 const QColor &backgroundColor = Qt::black, int minSize = 100, int maxSize = 4000, int border = 0);
    ~PositionView();

public Q_SLOTS:  
    /**Updates the information needed to draw the position of the animal.
  * @param width video image width.
  * @param height video image height.
  * @param backgroundImage image used as a background for the position view.
  * @param newOrientation true if the image has been transformed (rotate and or flip), false otherwise.
  * @param active true if the view is the active one, false otherwise.
  */
    void updatePositionInformation(int width, int height,const QImage& backgroundImage,bool newOrientation,bool active);

    /**Updates the postions to show between @p start and @p start + @p timeFrameWidth.
   * @param start starting time in miliseconds.
   * @param timeFrameWidth time window in miliseconds.
   */
    void displayTimeFrame(long start,long timeFrameWidth);

    /**Displays the data that has been retrieved.
  * @param data n column array containing the position of the animal. The two first columns contain
  * the position of the first spot and the following optional pair of columns contain the position of optional spots.
  * @param initiator instance requesting the data.
  */
    void dataAvailable(Array<dataType>& data,QObject* initiator);

    /**Prints the currently display information on a printer via the painter @p printPainter.
  * Does not print zoomed display.
  * @param printPainter painter on a printer.
  * @param metrics object providing informatin about the printer.
  * @param whiteBackground true if the printed background has to be white, false otherwise.
  * @param backgroundForPrinting special background to be used for printing.
  */
    void print(QPainter& printPainter, int width, int height, bool whiteBackground, const QImage &backgroundForPrinting = QImage());

    /***Changes the color of the background.*/
    void changeBackgroundColor(const QColor& color);
    
    /** Displays the event data that has been retrieved.
  * @param eventsData dictionary between the event provider names and the event data and status.
  * @param selectedEvents map between the event provider names and the list of currently selected events.
  * @param providerItemColors dictionary between the provider names and the item color lists.
  * @param initiator instance requesting the data.
  * @param samplingRate sampling rate of the current open data file in Hz.
  */
    void dataAvailable(QHash<QString, EventData*>& eventsData,QMap<QString, QList<int> >& selectedEvents,QHash<QString, ItemColors*>& providerItemColors,QObject* initiator,double samplingRate);

    /**Updates the event display if any event are available.*/
    void updateEventDisplay();

    /**Changes the color of an event.
  * @param name name use to identified the event provider containing the updated event.
  * @param eventId id of the event to redraw.
  * @param active true if the view is the active one, false otherwise.
  */
    void eventColorUpdate(const QColor &color, const QString& name, int eventId, bool active);

    /**Update the information presented in the view.*/
    void updateDrawing(){
        //Everything has to be redraw
        drawContentsMode = REDRAW ;
        update();
    }
    
    void addEventProvider(){updateEventDisplay();}

    /**Removes a provider of event data.
  * @param name name use to identified the event provider.
  * @param active true if the view is the active one, false otherwise.
  * @param lastFile true if the event file removed is the last event provider, false otherwise.
  */
    void removeEventProvider(const QString &name, bool active, bool lastFile);

    /**Sets if events are displayed in the view.
  * @param shown 1 if events are displayed in the view, 0 otherwise.
  */
    void setEventsInPositionView(bool shown);

protected:
    /**
  * Draws the contents of the frame
  * @param p painter used to draw the contents
  */
    void paintEvent ( QPaintEvent*);

    /**The view responds to a resize event.
  * The bachground image is recomputed.
  * @param event resize event.
  */
    void resizeEvent(QResizeEvent* event){
        drawContentsMode = REDRAW;
        resized = true;
    }

private:
    /**Background image.*/
    QImage background;

    /**Scaled background pixmap.*/
    QPixmap scaledBackground;

    /**
  * Buffer to enable smooth updating, obtain flicker-free drawing.
  * Prevent from unnecessary redrawing.
  */
    QPixmap doublebuffer;

    /**True if the data information needed to draw the positions are available.*/
    bool dataReady;

    /**Boolean used to correctly display the background image at startup.*/
    bool isInit;

    /**Boolean used to update the display after a resize event.*/
    bool resized;

    /**Provider of the position data.*/
    PositionsProvider& positionsProvider;

    /**Array containing the position data.
  * n column array containing the position of the animal. The two first columns contain
  * the position of the first spot and the following optional pair of columns contain the position of optional spots.*/
    Array<dataType> data;

    /**This variable keeps track of the current start time, in milisecond, of the time window.*/
    long startTime;

    /**This variable keeps track of the current end time, in milisecond, of the time window.*/
    long endTime;

    /**Time amount, in milisecond, of the time frame used to display the positions.*/
    long timeFrameWidth;

    /**Number of spots recorded for each position, from 1 to n.*/
    int nbSpots;

    /**Dictionary between the event provider names and the event data and status.*/
    QHash<QString, EventData*> eventsData;

    /**Map between the event provider names and the list of selected events.*/
    QMap<QString, QList<int> > selectedEvents;
    
    /**Dictionary between the provider names and the item color lists.*/
    QHash<QString, ItemColors*> providerItemColors;

    /**Provider for whole set of event files.*/
    GlobalEventsProvider& globalEventProvider;

    /**Boolean used to manage the display of events.*/
    bool showEvents;
    
    /// Functions

    /**Scale the background image to fit it in the widget.*/
    void scaleBackgroundImage();

    /**
  * Draws the positions.
  * @param painter painter on which to draw the positions.
  */
    void drawPositions(QPainter& painter);

    /**Computes the event positions if any.
  * @param samplingRate sampling rate of the current open data file in Hz.
  */
    void computeEventPositions(double samplingRate);

    /**Draw events if any.
  * @param painter painter on which to draw the events.
  */
    void drawEvents(QPainter& painter);
};

#endif
