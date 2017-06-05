/******************************************************************************
 * libKD
 * zlib/libpng License
 ******************************************************************************
 * Copyright (c) 2014-2017 Kevin Schmidt
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
#include <KD/kdext.h>

#ifdef KD_NDEBUG
#error "Dont run tests with NDEBUG defined."
#endif

KDint KD_APIENTRY kdMain(KDint argc, const KDchar *const *argv)
{
    KDint retval = kdNameLookup(KD_AF_INET, "www.icann.org", (void *)1234);
    if(retval == -1)
    {
        if(kdGetError() == KD_ENOSYS)
        {
            return 0;
        }
        kdAssert(0);
    }

    for(;;)
    {
        const KDEvent *event = kdWaitEvent(-1);
        if(event)
        {
            if(event->type == KD_EVENT_NAME_LOOKUP_COMPLETE)
            {
                KDEventNameLookup lookupevent = event->data.namelookup;
                if(lookupevent.error == KD_EHOST_NOT_FOUND)
                {
                    /* No internet. */
                    break;
                }

                KDInAddr address;
                kdMemset(&address, 0, sizeof(address));
                address.s_addr = ((const KDSockaddr *)lookupevent.result)->data.sin.address;
                kdAssert(kdNtohl(address.s_addr) == 3221233671);

                KDchar c_addr[sizeof("255.255.255.255")] = "";
                kdInetNtop(KD_AF_INET, &address, c_addr, sizeof(c_addr));
                kdLogMessagefKHR("%s\n", c_addr);
                kdAssert(kdStrcmp(c_addr, "192.0.32.7") == 0);

                break;
            }
            kdDefaultEvent(event);
        }
    }
    return 0;
}

