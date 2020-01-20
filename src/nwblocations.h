#ifndef NWBLOCATIONS_H
#define NWBLOCATIONS_H

#include "neuroscopexmlreader.h"
#include <QString>

class NWBLocations
{
public:
    NWBLocations(std::string hsFileName);

    std::string getVoltageDataSetName();
    std::string getSamplingName();

    std::string getGenericText(std::string strLabel, std::string strDefault = "");

private:
    NWBLocations(){} // defensive move
    QString strLoc;
    QString getLocationName(std::string& s, const std::string& newExt);
};

#endif // NWBLOCATIONS_H
