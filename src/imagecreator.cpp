/***************************************************************************
 *   Copyright (C) 2004 by Lynn Hazan                                      *
 *   lynn@myrealbox.com                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "imagecreator.h"


#include <QPixmap>


ImageCreator::ImageCreator(PositionsProvider& provider, int width, int height, const QString& backgroundImage, const QColor& backgroundColor, const QColor &foregroundColor)
    :positionsProvider(provider),
      width(width),
      height(height),
      backgroundImage(backgroundImage),
      backgroundColor(backgroundColor),
      foregroundColor(foregroundColor)
{
    nbSpots = positionsProvider.getNbSpots();

    //Set Connection.
    connect(&positionsProvider,SIGNAL(dataReady(Array<dataType>&,QObject*)),this,SLOT(dataAvailable(Array<dataType>&,QObject*)));
}

ImageCreator::~ImageCreator(){}

QImage ImageCreator::createImage(){ 
    //request the data need it to create the image.
    positionsProvider.retrieveAllData(this);

    return image;
}

void ImageCreator::dataAvailable(Array<dataType>& data,QObject* initiator){
    //If another widget was the initiator of the request, ignore the data.
    if(initiator != this) return;

    this->data = data;

    //Create a painter to paint on the pixmap
    QPainter painter;
    QPixmap pixmap(width,height);
    painter.begin(&pixmap);

    //If an image has been set to be used as background, scale it if need it and then draw it.
    if(!backgroundImage.isEmpty()){
        QImage image(backgroundImage);
        QPixmap scaledBackground;
        scaledBackground.convertFromImage(image.scaled(width,height),Qt::PreferDither);
        painter.drawPixmap(0,0,scaledBackground);
    }

    //The points are drawn in the QT coordinate system where the Y axis in oriented downwards
    painter.setWindow(QRect(QPoint(0,0),QPoint(width,height)));
    
    //Fill the pixmap with the background color if no image has been set as background.
    if(backgroundImage.isEmpty())
        pixmap.fill(backgroundColor);

    //Paint all the positions on the pixmap.
    drawPositions(painter);
    
    //Closes the painter on the pixmap
    painter.end();

    image = pixmap.toImage();
}

void ImageCreator::drawPositions(QPainter& painter){
    //The points are drawn in the QT coordinate system where the Y axis in oriented downwards
    if(nbSpots == 0)
        return;
    const int nbPoints = data.nbOfRows();
    if(nbPoints == 0)
        return;
    painter.setPen(foregroundColor);
    for(int i = 1;i<=nbPoints;++i)
        painter.drawPoint(data(i,1),data(i,2));
}
