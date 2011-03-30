/******************************************************************************
Copyright (c) 2010, Google Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, 
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.
    * Neither the name of the <ORGANIZATION> nor the names of its contributors 
    may be used to endorse or promote products derived from this software 
    without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE 
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
******************************************************************************/

#include "stdafx.h"
#include "shared_mem.h"

#pragma once
#pragma data_seg (".shared")
HHOOK	shared_hook_handle = 0;
WCHAR  shared_results_file_base[MAX_PATH] = {NULL};
DWORD  shared_test_timeout = 120000;
bool   shared_test_force_on_load = false;
bool   shared_cleared_cache = false;
DWORD  shared_current_run = 0;
#pragma data_seg ()

#pragma comment(linker,"/SECTION:.shared,RWS")

extern "C" {
__declspec( dllexport ) void WINAPI SetResultsFileBase(const WCHAR* file_base);
__declspec( dllexport ) void WINAPI SetTestTimeout(DWORD timeout);
__declspec( dllexport ) void WINAPI SetForceDocComplete(bool force);
__declspec( dllexport ) void WINAPI SetClearedCache(bool cleared_cache);
__declspec( dllexport ) void WINAPI SetCurrentRun(DWORD run);
}

/*-----------------------------------------------------------------------------
  Set the base file name to use for results files
-----------------------------------------------------------------------------*/
void WINAPI SetResultsFileBase(const WCHAR * file_base) {
  lstrcpyW(shared_results_file_base, file_base);
}

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
void WINAPI SetTestTimeout(DWORD timeout) {
  shared_test_timeout = timeout;
}

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
void WINAPI SetForceDocComplete(bool force) {
  shared_test_force_on_load = force;
}

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
void WINAPI SetClearedCache(bool cleared_cache) {
  shared_cleared_cache = cleared_cache;
}

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
void WINAPI SetCurrentRun(DWORD run) {
  shared_current_run = run;
}