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
#ifndef IMAGECREATOR_H
#define IMAGECREATOR_H

// include files for Qt
#include <QWidget>
#include <QPixmap>
#include <QImage>
#include <QPainter>

// application specific includes
#include "positionsprovider.h"
#include <types.h>


/**
Utility class used to create an image containing all the first spot positions of the animal during the recording.

@author Lynn Hazan
*/
class ImageCreator : public QWidget
{
    Q_OBJECT
public:
    /**
   * Constructor.
   * @param provider a reference on the provider of the position data.
   * @param width video image width to use as the image width.
   * @param height video image height to use as the image height.
   * @param backgroundImage image to use as the background.
   * @param backgroundColor color used as the image background if no image has been set.
   * @param foregroundColor color used as the image foreground.
   */
    explicit ImageCreator(PositionsProvider& provider,int width,int height,const QString& backgroundImage=QString(),const QColor& backgroundColor = Qt::black,const QColor& foregroundColor = "#BFBFBF");
    ~ImageCreator();

    /**Creates an image containg all the positions of a given position file.*/
    QImage createImage();

public Q_SLOTS:     

    /**Receive the data requested and actually creates the image containg all the positions.
   * @param data n column array containing the position of the animal. The two first columns contain
   * the position of the first spot and the following optional pair of columns contain the position of optional spots.
   * @param initiator instance requesting the data.
   */
    void dataAvailable(Array<dataType>& data,QObject* initiator);

private:

    /**Provider of the position data.*/
    PositionsProvider& positionsProvider;

    /**Image width.*/
    int width;

    /**Image height.*/
    int height;

    /**Image to use as the background.*/
    QString backgroundImage;

    /**Image background color.*/
    QColor backgroundColor;

    /**Image foreground color.*/
    QColor foregroundColor;

    /**Array containing the position data.
  * n column array containing the position of the animal. The two first columns contain
  * the position of the first spot and the following optional pair of columns contain the position of optional spots.*/
    Array<dataType> data;

    /**Number of spots recorded for each position, from 1 to n.*/
    int nbSpots;

    /**QImage containing the created image.*/
    QImage image;

    ////Function

    /**
  * Draws the positions.
  * @param painter painter on which to draw the positions.
  */
    void drawPositions(QPainter& painter);
};

#endif
