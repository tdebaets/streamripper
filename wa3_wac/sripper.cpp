/* sripper.cpp - jonclegg@yahoo.com
 * 
 * Portions from the ExampleVisData example from the Winamp3 SDK
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

//
// Always start with std.h
#include "bfc/std.h"
//
#include "sripper.h"     //EDITME
#include "resource.h"
#include "sripperwnd.h"  //EDITME
//
#include "common/xlatstr.h"
// Window creation services includes.
#include "studio/services/creators.h"
#include "bfc/wndcreator.h"


static WACNAME wac;
WACPARENT *the = &wac;                     

//  *** This is MY GUID.  Get your own!
// {2CF2ED18-08EC-4df5-8D28-5BED227E21B2}
static const GUID guid =  //EDITME (hint: use guidgen.exe)
{ 0x2cf2ed18, 0x8ec, 0x4df5, { 0x8d, 0x28, 0x5b, 0xed, 0x22, 0x7e, 0x21, 0xb2 } };



WACNAME::WACNAME() : WACPARENT(/**/"SRipper Component"/*EDITME*/) {
  // So, what did we want to do again?  Oh yes...

  // 1) we are going to make a window,
  registerService(new WndCreateCreatorSingle< CreateWndByGuid</**/SRipperWnd/*EDITME*/> >);

  // 2) and have it be listed in the Thinger,
  registerService(new WndCreateCreatorSingle< CreateBucketItem</**/SRipperWnd/*EDITME*/> >);

  // 3) and in the main context menu.
  registerAutoPopup(getGUID(), getName());
	
}

WACNAME::~WACNAME() {
}

GUID WACNAME::getGUID() {
  return guid;
}

void WACNAME::onCreate() {
  // *** Do startup stuff here that doesn't require you to have a window yet
}

void WACNAME::onDestroy() {
}

