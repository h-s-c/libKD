/******************************************************************************
 * libKD
 * zlib/libpng License
 ******************************************************************************
 * Copyright (c) 2014-2016 Kevin Schmidt
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 ******************************************************************************/

#include <KD/kd.h>

KDint KD_APIENTRY kdMain(KDint argc, const KDchar *const *argv)
{
    KDtime time;
    KDTm tm;

    time = 0;
    kdGmtime_r(&time, &tm);
    if(!(tm.tm_year==70 && tm.tm_mon==0 && tm.tm_mday==1 &&
        tm.tm_hour==0 && tm.tm_min==0 && tm.tm_sec==0 && 
        tm.tm_wday==4 && tm.tm_yday==0 && tm.tm_isdst==0))
    {
        return 1;
    }
    time = 1;
    kdGmtime_r(&time, &tm);
    if(!(tm.tm_year==70 && tm.tm_mon==0 && tm.tm_mday==1 &&
        tm.tm_hour==0 && tm.tm_min==0 && tm.tm_sec==1 && 
        tm.tm_wday==4 && tm.tm_yday==0 && tm.tm_isdst==0))
    {
        return 1;
    }
    time = 60;
    kdGmtime_r(&time, &tm);
    if(!(tm.tm_year==70 && tm.tm_mon==0 && tm.tm_mday==1 &&
        tm.tm_hour==0 && tm.tm_min==1 && tm.tm_sec==0 && 
        tm.tm_wday==4 && tm.tm_yday==0 && tm.tm_isdst==0))
    {
        return 1;
    }
    time = 60*60;
    kdGmtime_r(&time, &tm);
    if(!(tm.tm_year==70 && tm.tm_mon==0 && tm.tm_mday==1 &&
        tm.tm_hour==1 && tm.tm_min==0 && tm.tm_sec==0 && 
        tm.tm_wday==4 && tm.tm_yday==0 && tm.tm_isdst==0))
    {
        return 1;
    }
    time = 60*60*24;
    kdGmtime_r(&time, &tm);
    if(!(tm.tm_year==70 && tm.tm_mon==0 && tm.tm_mday==2 &&
        tm.tm_hour==0 && tm.tm_min==0 && tm.tm_sec==0 && 
        tm.tm_wday==5 && tm.tm_yday==1 && tm.tm_isdst==0))
    {
        return 1;
    }
    time = 60*60*24*31;
    kdGmtime_r(&time, &tm);
    if(!(tm.tm_year==70 && tm.tm_mon==1))
    {
        return 1;
    }
    time = 60*60*24*31*12;
    kdGmtime_r(&time, &tm);
    if(!(tm.tm_year==71 && tm.tm_mon==0))
    {
        return 1;
    }
    return 0;
}
