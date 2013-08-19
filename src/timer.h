/***************************************************************************
                          timer.h  -  description
                             -------------------
    begin                : lun sep 22 2003
    copyright            : (C) 2003 by Lynn Hazan
    email                : lynn.hazan@myrealbox.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <sys/time.h>

static struct timeval tv0;

inline void RestartTimer()
{
  struct timezone tz;
  gettimeofday(&tv0,&tz);
}

inline float Timer()
{
  struct timeval tv;
  struct timezone tz;
  gettimeofday(&tv,&tz);
  float msec = static_cast<int>(tv.tv_usec/1000)/1000.0;
  float msec0 = static_cast<int>(tv0.tv_usec/1000)/1000.0;
  float time = (tv.tv_sec+msec)-(tv0.tv_sec+msec0);
  return time;
}
