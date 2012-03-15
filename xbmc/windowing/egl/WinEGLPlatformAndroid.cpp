/*
 *      Copyright (C) 2012 Team XBMC
 *      http://www.xbmc.org
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

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/ioctl.h>

#include "WinEGLPlatformAndroid.h"
#include "utils/PlatformUtils.h"

////////////////////////////////////////////////////////////////////////////////////////////
EGLNativeWindowType CWinEGLPlatformAndroid::InitWindowSystem(EGLNativeDisplayType nativeDisplay, int width, int height, int bpp)
{
  if (CPlatformUtils::GetPlatform() == NULL || CPlatformUtils::GetPlatform()->window == NULL)
    return 0;

  CWinEGLPlatformGeneric::InitWindowSystem(nativeDisplay, width, height, bpp);
  
  return getNativeWindow();
}

void CWinEGLPlatformAndroid::DestroyWindowSystem(EGLNativeWindowType native_window)
{
}

bool CWinEGLPlatformAndroid::ClampToGUIDisplayLimits(int &width, int &height)
{
  return false;
}

void CWinEGLPlatformAndroid::createSurfaceCallback()
{
  EGLint format;
  // EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
  // guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
  // As soon as we picked a EGLConfig, we can safely reconfigure the
  // ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID.
  eglGetConfigAttrib(m_display, m_config, EGL_NATIVE_VISUAL_ID, &format);

  CPlatformUtils::GetPlatform()->android_setBuffersGeometry(CPlatformUtils::GetPlatform()->window, 0, 0, format);
}

EGLNativeWindowType CWinEGLPlatformAndroid::getNativeWindow()
{
  if (CPlatformUtils::GetPlatform() == NULL || CPlatformUtils::GetPlatform()->window == NULL)
    return 0;

  return (EGLNativeWindowType)CPlatformUtils::GetPlatform()->window;
}
