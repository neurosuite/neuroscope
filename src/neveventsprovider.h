/***************************************************************************
                          nsxtracesprovider.h  -  description
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

#ifndef _NEVEVENTSPROVIDER_H_
#define _NEVEVENTSPROVIDER_H_

#include "blackrock.h"
#include "eventsprovider.h"

class NEVEventsProvider : public EventsProvider  {
    Q_OBJECT

public:
    NEVEventsProvider(const QString &fileUrl, int position);
    ~NEVEventsProvider();

    /**Loads the event ids and the corresponding spike time.
    * @return an loadReturnMessage enum giving the load status
    */
    virtual int loadData();

private:
    NEVBasicHeader mBasicHeader;
    NEVExtensionHeader* mExtensionHeaders;

    template <typename T>
    inline bool readStruct(QFile& file, T& s) {
        qint64 bytesToRead = sizeof(T);
        qint64 bytesRead = file.read(reinterpret_cast<char*>(&s), bytesToRead);
        return (bytesToRead == bytesRead);
    }
};

#endif
