#ifndef NWBCLUSTERPROVIDER_H
#define NWBCLUSTERPROVIDER_H

#include <QStringList>
#include "clustersprovider.h"


class NWBClustersProvider : public ClustersProvider  {
    Q_OBJECT

public:
    static QList<NWBClustersProvider*> fromFile(const QString& file,
                                                QStringList channelLabels,
                                                double samplingRate,
                                                dataType fileMaxTime,
                                                int position);

    ~NWBClustersProvider();

    /**Loads the event ids and the corresponding spike time.
    * @return an loadReturnMessage enum giving the load status
    */
    virtual int loadData();

private:
    NWBClustersProvider(unsigned int channel,
                        Array<dataType>& data,
                        int spikeCount,
                        double samplingRate,
                        double currentSamplingRate,
                        dataType fileMaxTime,
                        int position);
};

#endif // NWBCLUSTERPROVIDER_H
