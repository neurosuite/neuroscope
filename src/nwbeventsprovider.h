#ifndef NWBEVENTSPROVIDER_H
#define NWBEVENTSPROVIDER_H

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
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
