#ifndef NWBEVENTSPROVIDER_H
#define NWBEVENTSPROVIDER_H
#include "eventsprovider.h"

class NWBEventsProvider : public EventsProvider  {
    Q_OBJECT

public:
    NWBEventsProvider(const QString &fileUrl, int position);
    ~NWBEventsProvider();

    /**Loads the event ids and the corresponding spike time.
    * @return an loadReturnMessage enum giving the load status
    */
    virtual int loadData();

private:

};

#endif // NWBEVENTSPROVIDER_H
