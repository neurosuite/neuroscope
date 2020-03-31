#ifndef NWBLOCATIONS_H
#define NWBLOCATIONS_H

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "neuroscopexmlreader.h"
#include <QString>

class NWBLocations
{
public:
    NWBLocations(std::string hsFileName);

    std::string getVoltageDataSetName();
    std::string getSamplingName();

    std::string getGenericText(std::string strLabel, std::string strDefault = "");
    QList<std::string> getListGenericTexts(std::string strLabel, std::string strDefault);

private:
    NWBLocations(){} // defensive move
    QString strLoc;
    QString getLocationName(std::string& s, const std::string& newExt);
};

#endif // NWBLOCATIONS_H
