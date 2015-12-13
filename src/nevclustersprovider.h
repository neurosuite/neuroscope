/***************************************************************************
                          nevclustersprovider.h  -  description
                             -------------------
    copyright            : (C) 2015 by Florian Franzen
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _NEVCLUSTERSPROVIDER_H_
#define _NEVCLUSTERSPROVIDER_H_

#include "blackrock.h"
#include "clustersprovider.h"

class NEVClustersProvider : public ClustersProvider  {
    Q_OBJECT

public:
    static QList<NEVClustersProvider*> fromFile(const QString& file,
                                                QStringList channelLabels,
                                                double samplingRate,
                                                dataType fileMaxTime,
                                                int position);

    ~NEVClustersProvider();

    /**Loads the event ids and the corresponding spike time.
    * @return an loadReturnMessage enum giving the load status
    */
    virtual int loadData();

private:
    NEVClustersProvider(unsigned int channel,
                        Array<dataType>& data,
                        int spikeCount,
                        double samplingRate,
                        double currentSamplingRate,
                        dataType fileMaxTime,
                        int position);
};

#endif
