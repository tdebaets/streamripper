/* sripper.h - jonclegg@yahoo.com
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

#ifndef _SRIPPER_H
#define _SRIPPER_H

#include "studio/wac.h"

#define WACNAME WACSRipper
#define WACPARENT WAComponentClient

class WACNAME : public WACPARENT {
public:
  WACNAME();
  virtual ~WACNAME();

  virtual GUID getGUID();

  virtual void onCreate();
  virtual void onDestroy();

  // Put your public methods here.
  
private:
  // Put your private data here.
};

extern WACPARENT *the;

#endif//_SRIPPER_H
