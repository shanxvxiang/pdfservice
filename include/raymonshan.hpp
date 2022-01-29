
#ifndef __RAYMON_SHAN_ALLINONE_H
#define __RAYMON_SHAN_ALLINONE_H

#include "raymoncommon.hpp"     // basic declare
#include "raymoncmdparser.hpp"  // parse command line para
#include "raymonspin.hpp"       // for SpinLocker and SpinLong class
#include "raymontest.hpp"       // some function for testing

#include "raymonlockbuffer.hpp" // for fix-length buffer Get() & Put()
#include "raymonepoll.hpp"      // framework of epoll
#include "raymonipc.hpp"        // framework of one to multi process communication

#include "pdfdispatch.hpp"      // dispatch process and send msg
#include "pdfprocess.hpp"       // get file and translate to picture






#endif  // __RAYMON_SHAN_ALLINONE_H


/*
konwing problem
-static link, nanolog segment fault
nanolog not support multi PROCESS
 */
