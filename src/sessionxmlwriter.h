/***************************************************************************
                          sessionxmlwriter.h  -  description
                             -------------------
    begin                : Fri Apr 2 2004
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

#ifndef SESSIONXMLWRITER_H
#define SESSIONXMLWRITER_H




//Application specific includes
#include "sessionInformation.h"

//include files for QT
#include <QList>
#include <QMap>
#include <qdom.h> 

// forward declaration

/**
  * Class writting the session xml file.
  *@author Lynn Hazan
  */

class SessionXmlWriter {
public:

    /**Constructor which will write a session file to the @p url.
  */
    SessionXmlWriter();

    ~SessionXmlWriter();

    /**Writes the xml tree to a session file.
  * @param url url of the file to write to.
  * @return true if the session could be write to disk, falsse otherwise.
  */
    bool writeTofile(const QString& url);

    /**
  * Creates the elements related to the list of files which where loaded during the session,
  * cluster, spike or event files.
  * @param fileList list of files loaded during the session.
  */
    void setLoadedFilesInformation(const QList<SessionFile> &fileList);

    /**
  * Creates the elements related to the displays.
  * @param displayList list of DisplayInformation given the information on each display.
  */
    void setDisplayInformation(const QList<DisplayInformation> &displayList);

private:

    /**The session document.*/
    QDomDocument doc;

    /**The root element.*/
    QDomElement root;

    /**The element containing the video information.*/
    QDomElement video;

    /**The sampling rate by extension element.*/
    QDomElement samplingRates;

    /**The element corresponding gto the loaded files.*/
    QDomElement loadedFiles;

    /**The element containing the display information.*/
    QDomElement displays;

};

#endif
