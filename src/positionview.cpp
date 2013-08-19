/***************************************************************************
                          positionview.cpp  -  description
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

// include files for QT
#include "qapplication.h"

#include <QVector>
#include <QList>
#include <QPixmap>
#include <QDebug>

//include files for the application
#include "positionview.h"
#include "positionsprovider.h"
#include "timer.h"
#include "itemcolors.h"



PositionView::PositionView(PositionsProvider& provider,GlobalEventsProvider& globalEventProvider,const QImage& backgroundImage,long start,long timeFrameWidth,bool showEvents,int windowTopLeft,
                           int windowBottomRight,QWidget* parent,const char* name,const QColor &backgroundColor,int minSize,int maxSize,
                           int border) :
    BaseFrame(0,0,parent,name,backgroundColor,minSize,maxSize,windowTopLeft,windowBottomRight,border),
    background(backgroundImage),dataReady(false),isInit(true),resized(false),positionsProvider(provider),globalEventProvider(globalEventProvider),showEvents(showEvents) {

    //The video recording stores information like in the QT coordinate system: Y axis in oriented downwards
    window = ZoomWindow(QRect(QPoint(0,0),QPoint(windowBottomRight,windowTopLeft)));

    startTime = start;
    endTime = start + timeFrameWidth;
    this->timeFrameWidth = timeFrameWidth;
    nbSpots = positionsProvider.getNbSpots();
    
    //Set Connection.
    connect(&positionsProvider,SIGNAL(dataReady(Array<dataType>&,QObject*)),this,SLOT(dataAvailable(Array<dataType>&,QObject*)));

    scaleBackgroundImage();

    //Get the data.
    positionsProvider.requestData(startTime,endTime,this);
}


PositionView::~PositionView(){
    qDeleteAll(eventsData);
    eventsData.clear();
}

void PositionView::paintEvent ( QPaintEvent*){

    if(isInit){
        QRect contentsRec = contentsRect();
        viewport =  QRect(contentsRec.left(),contentsRec.top(),contentsRec.width(),contentsRec.height());
        if(viewport.width() == 0)
            update();
        else
            scaleBackgroundImage();

        isInit = false;
    }

    if(resized){
        resized = false;
        scaleBackgroundImage();
        drawContentsMode = REDRAW;
        update();
    }
    QPainter p(this);
    if(drawContentsMode == REDRAW && dataReady){
        QRect contentsRec = contentsRect();
        QRect r((QRect)window);

        viewport = QRect(contentsRec.left(),contentsRec.top(),contentsRec.width(),contentsRec.height());

        //Resize the double buffer with the width and the height of the widget(QFrame)
        //doublebuffer.resize(contentsRec.width(),contentsRec.height());

        if (contentsRec.size() != doublebuffer.size()) {
            if(!doublebuffer.isNull()) {
                QPixmap tmp = QPixmap( contentsRec.width(),contentsRec.height() );
                tmp.fill( Qt::white );
                QPainter painter2( &tmp );
                painter2.drawPixmap( 0,0, doublebuffer );
                painter2.end();
                doublebuffer = tmp;
            } else {
                doublebuffer = QPixmap(contentsRec.width(),contentsRec.height());
            }
        }

        //Create a painter to paint on the double buffer
        QPainter painter;
        painter.begin(&doublebuffer);

        //if need it, draw the background image before applying any transformation to the painter.
        if(!background.isNull())
            painter.drawPixmap(0,0,scaledBackground);

        //Set the window (part of the world I want to show)
        painter.setWindow(r.left(),r.top(),r.width()-1,r.height()-1);//hack because Qt QRect is used differently in this function

        //Set the viewport (part of the device I want to write on).
        //By default, the viewport is the same as the device's rectangle (contentsRec).
        painter.setViewport(viewport);

        //Fill the double buffer with the background color if no image has been set.
        if(background.isNull())
            doublebuffer.fill(backgroundRole());

        //Paint all the positions in the double buffer on top of the background image or the background color.
        drawPositions(painter);

        //Paint the event if any
        if(showEvents && !selectedEvents.isEmpty())
            drawEvents(painter);

        //Closes the painter on the double buffer
        painter.end();

        //Back to the default
        drawContentsMode = REFRESH;
    }

    //Draw the double buffer (pixmap) by copying it into the widget device.
    p.drawPixmap(0, 0, doublebuffer);
}

void PositionView::updatePositionInformation(int width, int height, const QImage &backgroundImage, bool newOrientation, bool active){
    background = backgroundImage;
    //The video recording stores information like in the QT coordinate system: Y axis in oriented downwards
    window = ZoomWindow(QRect(QPoint(0,0),QPoint(width,height)));

    scaleBackgroundImage();

    if(newOrientation){
        //Get the updated data.
        positionsProvider.requestData(startTime,endTime,this);
    }
    else{
        drawContentsMode = REDRAW;
        if(active)
            update();
    }
}

void PositionView::scaleBackgroundImage(){
    if(!background.isNull()){
        QRect contentsRec = contentsRect();
        scaledBackground.convertFromImage(background.scaled(contentsRec.width(),contentsRec.height()),Qt::PreferDither);
    }
}

void PositionView::displayTimeFrame(long start,long timeFrameWidth){
    startTime = start;
    endTime = start + timeFrameWidth;
    this->timeFrameWidth = timeFrameWidth;

    //Request the data
    dataReady = false;
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    //Get the data.
    positionsProvider.requestData(startTime,endTime,this);
    globalEventProvider.requestData(startTime,endTime,this);
}

void PositionView::dataAvailable(Array<dataType>& data,QObject* initiator){
    //If another widget was the initiator of the request, ignore the data.
    if(initiator != this) return;

    this->data = data;
    dataReady = true;

    QApplication::restoreOverrideCursor();

    //Everything has to be redraw
    drawContentsMode = REDRAW;

    update();
}

void PositionView::drawPositions(QPainter& painter){

    //The points are drawn in the QT coordinate system where the Y axis in oriented downwards
    if(nbSpots == 0) return;
    int nbPoints = data.nbOfRows();
    if(nbPoints == 0) return;
    //If there is only one spot, draw a point in red
    if(nbSpots == 1){
        painter.setPen(Qt::red);
        painter.setBrush(Qt::red);
        for(int i = 1;i<nbPoints;++i) painter.drawEllipse(data(i,1)-1,data(i,2)-1,2,2);
        //The last position is emphasized, bigger points with white in the center.
        painter.setBrush(Qt::red);
        painter.setPen(Qt::black);
        painter.drawEllipse(data(nbPoints,1)-4,data(nbPoints,2)-4,8,8);
    }
    //If there are two spots, draw a line between them. The first point is red, the second green and the line is light grey.
    else if(nbSpots == 2){
        QColor lineColor = QColor(60,60,60);
        for(int i = 1;i<nbPoints;++i){
            painter.setPen(lineColor);
            if(data(i,1) > 0 && data(i,2) > 0 && data(i,3) > 0 && data(i,4) > 0)//if a spot has been misdetected, do not draw a line
                painter.drawLine(data(i,1),data(i,2),data(i,3),data(i,4));
            painter.setPen(Qt::red);
            painter.setBrush(Qt::red);
            painter.drawEllipse(data(i,1)-1,data(i,2)-1,2,2);
            painter.setPen(Qt::green);
            painter.setBrush(Qt::green);
            painter.drawEllipse(data(i,3)-1,data(i,4)-1,2,2);
        }
        //The last position is emphasized, white line and bigger points with white in the center.
        painter.setPen(Qt::white);
        if(data(nbPoints,1) > 0 && data(nbPoints,2) > 0 && data(nbPoints,3) > 0 && data(nbPoints,4) > 0)//if a spot has been misdetected, do not draw a line
            painter.drawLine(data(nbPoints,1),data(nbPoints,2),data(nbPoints,3),data(nbPoints,4));
        painter.setBrush(Qt::red);
        painter.setPen(Qt::black);
        painter.drawEllipse(data(nbPoints,1)-4,data(nbPoints,2)-4,8,8);
        painter.setBrush(Qt::green);
        painter.setPen(Qt::black);
        painter.drawEllipse(data(nbPoints,3)-4,data(nbPoints,4)-4,8,8);
    }
    //If there are n spots, draw a line between them. The first point is red, the second green and the line light grey.
    else{
        QColor lineColor = QColor(60,60,60);
        int nbCoordinates = nbSpots*2;
        for(int i = 1;i<nbPoints;++i){
            QPolygon polygon(nbSpots);
            int index = 1;
            int nbPointsInPolygon = 0;
            for(int j = 0;j<nbSpots;++j){
                if(data(i,index) > 0 && data(i,index+1) > 0){ //if a spot has been misdetected, do not includd it in the polygon
                    polygon.setPoint(j,data(i,index),data(i,index+1));
                    nbPointsInPolygon++;
                }
                index = index + 2;
            }
            painter.setBrush(Qt::NoBrush);
            QPen linePen(lineColor);
            linePen.setCosmetic(true);
            painter.setPen(linePen);

            int pointCount = (nbPointsInPolygon == -1) ?  polygon.size() : nbPointsInPolygon;

            painter.drawPolygon( polygon.constData(), pointCount, Qt::OddEvenFill);

            painter.setPen(Qt::red);//first point red
            painter.setBrush(Qt::red);
            painter.drawEllipse(data(i,1)-1,data(i,2)-1,2,2);
            QPen lineGreen(Qt::green);
            lineGreen.setCosmetic(true);
            painter.setPen(lineGreen);//other points green
            painter.setBrush(Qt::green);
            for(int j = 3;j<nbCoordinates;j=j+2){
                painter.drawEllipse(data(i,j)-1,data(i,j+1)-1,2,2);
            }
        }
        //The last position is emphasized, white line and bigger points with white in the center.
        QPolygon polygon(nbSpots);
        int index = 1;
        int nbPointsInPolygon = 0;
        for(int j = 0;j<nbSpots;++j){
            if(data(nbPoints,index) > 0 && data(nbPoints,index+1) > 0){ //if a spot has been misdetected, do not includd it in the polygon
                polygon.setPoint(j,data(nbPoints,index),data(nbPoints,index+1));
                nbPointsInPolygon++;
            }
            index = index + 2;
        }
        QPen lineWhite(Qt::white);
        lineWhite.setCosmetic(true);
        painter.setPen(lineWhite);
        painter.setBrush(Qt::NoBrush);
        int pointCount = (nbPointsInPolygon == -1) ?  polygon.size() : nbPointsInPolygon;

        painter.drawPolygon( polygon.constData(), pointCount, Qt::OddEvenFill);


        painter.setBrush(Qt::red);
        QPen lineBlack(Qt::black);
        lineBlack.setCosmetic(true);
        painter.setPen(lineBlack);
        painter.drawEllipse(data(nbPoints,1)-4,data(nbPoints,2)-4,8,8);
        painter.setBrush(Qt::green);
        painter.setPen(Qt::black);
        for(int j = 3;j<nbCoordinates;j=j+2){
            painter.drawEllipse(data(nbPoints,j)-4,data(nbPoints,j+1)-4,8,8);
        }
    }
}

void PositionView::print(QPainter& printPainter,int width, int height,bool whiteBackground,const QImage &backgroundForPrinting){
    //first  print the information on the file: name and position in the file.
    int nbMinutes = static_cast<int>(startTime / 60000.0);
    float remainingSeconds = static_cast<float>(fmod(static_cast<double>(startTime),60000));
    int nbSeconds = static_cast<int>(remainingSeconds / 1000);
    int nbMiliseconds = static_cast<int>(fmod(static_cast<double>(remainingSeconds),1000) + 0.5);
    if(nbMiliseconds == 1000){
        nbMiliseconds = 0;
        nbSeconds++;
    }

    QRect textRec = QRect(printPainter.viewport().left() + 5 ,printPainter.viewport().height() - 20,printPainter.viewport().width() - 5,20);
    QFont f("Helvetica",8);
    printPainter.setFont(f);
    printPainter.setPen(Qt::black);
    printPainter.drawText(textRec,Qt::AlignLeft | Qt::AlignVCenter,
                          QString("File: %1     Start time: %2 min %3 s %4 ms, Duration: %5 ms").arg(positionsProvider.getFilePath()).arg(nbMinutes).arg(nbSeconds).arg(nbMiliseconds).arg(timeFrameWidth));

    //Modify the viewport so the positions will not be drawn on the legend
    QRect newViewport = QRect(printPainter.viewport().left(),printPainter.viewport().top(),printPainter.viewport().width(),printPainter.viewport().height());
    newViewport.setBottom(printPainter.viewport().bottom() - 20);
    printPainter.setViewport(newViewport);

    //print the positions and the choosen background.


    //Draw the double buffer (pixmap) by copying it into the printer device throught the painter.
    QRect viewportOld = QRect(viewport.left(),viewport.top(),viewport.width(),viewport.height());

    viewport = QRect(printPainter.viewport().left(),printPainter.viewport().top(),printPainter.viewport().width(),printPainter.viewport().height());
    QRect r = ((QRect)window);

    //Fill the background with the background color if
    QRect back = QRect(r.left(),r.top(),r.width(),r.height());

    if(!whiteBackground && !background.isNull()){
        QPixmap printPixmap;
        printPixmap.convertFromImage(background.scaled(viewport.width(),viewport.height()),Qt::PreferDither);
        printPainter.drawPixmap(0,0,printPixmap);
    }

    //use the image dedicated for printing
    if(whiteBackground && !backgroundForPrinting.isNull()){
        QPixmap printPixmap;
        printPixmap.convertFromImage(backgroundForPrinting.scaled(viewport.width(),viewport.height()),Qt::PreferDither);
        printPainter.drawPixmap(0,0,printPixmap);
    }

    //Set the window (part of the world I want to show)
    printPainter.setWindow(r.left(),r.top(),r.width()-1,r.height()-1);//hack because Qt QRect is used differently in this function
    printPainter.setViewport(viewport);

    if(whiteBackground && backgroundForPrinting.isNull())
        printPainter.fillRect(back,Qt::white);
    if(!whiteBackground && background.isNull()){
        QColor color = palette().color(backgroundRole());
        printPainter.fillRect(back,color);
    }

    //Paint all the positions in the printPainter on top of the background image or the background color.
    drawPositions(printPainter);

    //Restore the previous state
    viewport = QRect(viewportOld.left(),viewportOld.top(),viewportOld.width(),viewportOld.height());

    printPainter.resetMatrix();
}

void PositionView::changeBackgroundColor(const QColor &color)
{
    BaseFrame::changeBackgroundColor(color);
}

void PositionView::dataAvailable(QHash<QString, EventData*>& eventsData,QMap<QString, QList<int> >& selectedEvents,QHash<QString, ItemColors*>& providerItemColors,QObject* initiator,double samplingRate){
    //Update the list of selected events.
    this->selectedEvents.clear();
    QMap<QString, QList<int> >::Iterator providersIterator;
    QMap<QString, QList<int> >::Iterator providersEnd(selectedEvents.end());
    for(providersIterator = selectedEvents.begin(); providersIterator != providersEnd; ++providersIterator){
        QList<int> eventsToShow = static_cast< QList<int> >(providersIterator.value());
        QList<int> events;
        QList<int>::iterator shownEventsIterator;
        QList<int>::iterator shownEnd(eventsToShow.end());
        for(shownEventsIterator = eventsToShow.begin(); shownEventsIterator != shownEnd; ++shownEventsIterator){
            events.append(*shownEventsIterator);
        }
        this->selectedEvents.insert(providersIterator.key(),events);
    }

    //Update the event data
    qDeleteAll(this->eventsData);
    this->eventsData.clear();

    //unselect all the items first
    QHashIterator<QString, EventData*> iterator(eventsData);
    while (iterator.hasNext()) {
        iterator.next();
        EventData* eventData = static_cast<EventData*>(iterator.value());
        EventData* eventDataCopy = new EventData();
        *eventDataCopy = *eventData;
        this->eventsData.insert(iterator.key(),eventDataCopy);
    }



    //Update the color list
    this->providerItemColors.clear();
    QHashIterator<QString, ItemColors*> colorIterator(providerItemColors);
    while (colorIterator.hasNext()) {
        colorIterator.next();
        this->providerItemColors.insert(colorIterator.key(),colorIterator.value());
    }

    if(!selectedEvents.isEmpty())
        computeEventPositions(samplingRate);

    //Everything has to be redraw
    drawContentsMode = REDRAW;
    update();
}

void PositionView::computeEventPositions(double samplingRate){
    float samplingRateInMs = static_cast<float>(static_cast<float>(samplingRate) / 1000.0);
    double positionSamplingInterval = 1000.0 / positionsProvider.getSamplingRate();

    QHashIterator<QString, EventData*> iterator(eventsData);
    while (iterator.hasNext()) {
        iterator.next();
        EventData* eventData = static_cast<EventData*>(iterator.value());
        int nbEvents = eventData->getTimes().nbOfColumns();

        eventData->computePositions(samplingRate,positionsProvider.getSamplingRate(),startTime);
        Array<dataType>& currentPositions =  eventData->getPositions();
    }

}

void PositionView::updateEventDisplay()
{
    if(showEvents)
        globalEventProvider.requestData(startTime,endTime,this);
}

void PositionView::drawEvents(QPainter& painter){  
    QPen pen;
    pen.setWidth(3);
    QMap<QString, QList<int> >::Iterator iterator;
    QMap<QString, QList<int> >::Iterator end(selectedEvents.end());
    for(iterator = selectedEvents.begin(); iterator != end; ++iterator){
        QList<int> eventList = iterator.value();
        QString providerName = iterator.key();
        if(eventList.isEmpty() || eventsData[providerName] == 0)
            continue;
        ItemColors* colors = providerItemColors[providerName];
        Array<dataType>& currentData = static_cast<EventData*>(eventsData[providerName])->getPositions();
        Array<int>& currentIds = static_cast<EventData*>(eventsData[providerName])->getIds();
        int nbEvents = currentData.nbOfColumns();
        for(int i = 1; i <= nbEvents;++i){
            dataType index = currentData(1,i);
            int eventId = currentIds(1,i);
            if(eventList.contains(eventId)){
                QColor color = colors->color(eventId);
                pen.setColor(color);
                painter.setPen(pen);
                painter.drawLine(data(index,1)-4,data(index,2)+4,data(index,1)+4,data(index,2)-4);
                painter.drawLine(data(index,1)-4,data(index,2)-4,data(index,1)+4,data(index,2)+4);
            }
        }
    }
}

void PositionView::eventColorUpdate(const QColor &color,const QString &name, int eventId, bool active){
    if(active){
        drawContentsMode = REDRAW ;
        update();
    }
}

void PositionView::removeEventProvider(const QString& name,bool active,bool lastFile){
    selectedEvents.remove(name);
    providerItemColors.remove(name);
    delete eventsData.take(name);
    if(lastFile)
        showEvents = false;

    drawContentsMode = REDRAW;
    if(active)
        update();
}

void PositionView::setEventsInPositionView(bool shown){
    showEvents = shown;
    if(shown) globalEventProvider.requestData(startTime,endTime,this);
    else{
        drawContentsMode = REDRAW;
        update();
    }
}
