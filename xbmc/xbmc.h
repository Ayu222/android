#pragma once

/*
 *      Copyright (C) 2012 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <stdint.h>

// runtime application flags
typedef enum {
  XBMCRunNull     = 0x00,
  XBMCRunPrimary  = 0x01,  // XBMC application
  XBMCRunGui      = 0x02,  // initialize GUI services
  XBMCRunServices = 0x04,  // start services: dbus, web, etc.
  XBMCRunAsApp    = XBMCRunPrimary | XBMCRunGui | XBMCRunServices
} XBMC_RUNFLAGS;

#if defined(ANDROID)
struct ANativeWindow;
typedef struct ANativeWindow ANativeWindow;

typedef int (*android_printf_t)(const char *format, ...);
typedef int32_t (*android_setBuffersGeometry_t)(ANativeWindow* window_type, int32_t width, int32_t height, int32_t format);

struct XBMC_PLATFORM {
  XBMC_RUNFLAGS flags;
  const char    *log_name;
  // android specific
  int32_t       width;
  int32_t       height;
  int32_t       format;
  ANativeWindow *window_type;
  // callbacks from xbmc into android
  // these are setup before call into XBMC_Initialize
  android_printf_t android_printf;
  android_setBuffersGeometry_t android_setBuffersGeometry;

  // callbacks from android into xbmc
  // these are valid after call into XBMC_Initialize

};

#else
struct XBMC_PLATFORM {
  XBMC_RUNFLAGS flags;
, const char    *log_name;
};
#endif
typedef struct XBMC_PLATFORM XBMC_PLATFORM;


typedef int (*XBMC_Initialize_t)(XBMC_PLATFORM*, int, const char**);
typedef int (*XBMC_Run_t)();

extern "C" int XBMC_Initialize(XBMC_PLATFORM *platform, int argc, const char** argv);
extern "C" int XBMC_Run();
