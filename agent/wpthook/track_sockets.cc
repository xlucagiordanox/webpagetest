
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

#include "StdAfx.h"
#include "track_sockets.h"
#include "requests.h"
#include "test_state.h"

const DWORD LOCALHOST = 0x0100007F; // 127.0.0.1

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
bool SocketInfo::IsLocalhost() {
  if (_addr.sin_addr.S_un.S_addr == LOCALHOST) {
    return true;
  }
  return false;
}

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
TrackSockets::TrackSockets(Requests& requests, TestState& test_state):
  _nextSocketId(1)
  , _requests(requests)
  , _test_state(test_state) {
  InitializeCriticalSection(&cs);
  _openSockets.InitHashTable(257);
  _socketInfo.InitHashTable(257);
  _sslSockets.InitHashTable(257);
}

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
TrackSockets::~TrackSockets(void) {
  Reset();
  DeleteCriticalSection(&cs);
}

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
void TrackSockets::Create(SOCKET s) {
}

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
void TrackSockets::Close(SOCKET s) {
  EnterCriticalSection(&cs);
  DWORD socket_id = 0;
  if (_openSockets.Lookup(s, socket_id) && socket_id)
    _requests.SocketClosed(socket_id);
  _openSockets.RemoveKey(s);
  LeaveCriticalSection(&cs);
}

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
void TrackSockets::Connect(SOCKET s, const struct sockaddr FAR * name, 
                            int namelen) {
  WptTrace(loglevel::kFunction, 
            _T("[wpthook] - TrackSockets::Connect(%d)\n"), s);

  // We only care about IP sockets at this point.
  if (name->sa_family == AF_INET6) {
    WptTrace(
        loglevel::kFunction, 
        _T("[wpthook] - TrackSockets::Connect: Warning: IPv6 unsupported!\n"));
  }
  if (namelen >= sizeof(struct sockaddr_in) && name->sa_family == AF_INET) {
    EnterCriticalSection(&cs);
    SocketInfo* info = GetSocketInfo(s);
    struct sockaddr_in* ip_name = (struct sockaddr_in *)name;
    memcpy(&info->_addr, ip_name, sizeof(ip_name));
    QueryPerformanceCounter(&info->_connect_start);
    LeaveCriticalSection(&cs);
  }
}

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
void TrackSockets::Connected(SOCKET s) {
  WptTrace(loglevel::kFunction, 
            _T("[wpthook] - TrackSockets::Connected(%d)\n"), s);
  EnterCriticalSection(&cs);
  SocketInfo* info = GetSocketInfo(s);
  QueryPerformanceCounter(&info->_connect_end);
  LeaveCriticalSection(&cs);
}

/*-----------------------------------------------------------------------------
  Look up the socket ID (or create one if it doesn't already exist)
  and pass the data on to the request tracker
-----------------------------------------------------------------------------*/
void TrackSockets::DataIn(SOCKET s, DataChunk& chunk) {
  EnterCriticalSection(&cs);
  SocketInfo* info = GetSocketInfo(s);
  DWORD socket_id = info->_id;
  bool is_localhost = info->IsLocalhost();
  LeaveCriticalSection(&cs);

  if (!is_localhost) {
    _requests.DataIn(socket_id, chunk);
  }
}

/*-----------------------------------------------------------------------------
  Allow data to be modified.
-----------------------------------------------------------------------------*/
bool TrackSockets::ModifyDataOut(SOCKET s, DataChunk& chunk) {
  bool is_modified = false;

  EnterCriticalSection(&cs);
  SocketInfo* info = GetSocketInfo(s);
  if (info->_is_ssl && !info->_ssl_start.QuadPart) {
    QueryPerformanceCounter(&info->_ssl_start);
  }
  DWORD socket_id = info->_id;
  bool is_localhost = info->IsLocalhost();
  LeaveCriticalSection(&cs);

  if (!is_localhost) {
    is_modified = _requests.ModifyDataOut(socket_id, chunk);
    WptTrace(loglevel::kProcess, _T("[wpthook] TrackSockets::ModifyDataOut")
        _T("(socket=%d, socket_id=%d, len=%d) -> %d"),
        s, socket_id, chunk.GetLength(), is_modified);
  }
  return is_modified;
}

/*-----------------------------------------------------------------------------
  Look up the socket ID (or create one if it doesn't already exist)
  and pass the data on to the request tracker
-----------------------------------------------------------------------------*/
void TrackSockets::DataOut(SOCKET s, DataChunk& chunk) {
  EnterCriticalSection(&cs);
  SocketInfo* info = GetSocketInfo(s);
  if (info->_is_ssl && info->_ssl_start.QuadPart && !info->_ssl_end.QuadPart) {
    QueryPerformanceCounter(&info->_ssl_end);
  }
  if (info->_connect_start.QuadPart && !info->_connect_end.QuadPart) {
    QueryPerformanceCounter(&info->_connect_end);
  }
  DWORD socket_id = info->_id;
  bool is_localhost = info->IsLocalhost();
  LeaveCriticalSection(&cs);

  if (!is_localhost) {
    _requests.DataOut(socket_id, chunk);
  }
}

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
void TrackSockets::Reset() {
  EnterCriticalSection(&cs);
  POSITION pos = _socketInfo.GetStartPosition();
  while (pos) {
    DWORD id = 0;
    SocketInfo* info = NULL;
    _socketInfo.GetNextAssoc(pos, id, info);
    if (info)
      delete info;
  }
  _socketInfo.RemoveAll();
  LeaveCriticalSection(&cs);
}

/*-----------------------------------------------------------------------------
  Claim ownership of a connection (associate it with a request)
-----------------------------------------------------------------------------*/
bool TrackSockets::ClaimConnect(DWORD socket_id, LONGLONG before, 
                                LONGLONG& start, LONGLONG& end,
                                LONGLONG& ssl_start, LONGLONG& ssl_end) {
  bool claimed = false;
  EnterCriticalSection(&cs);
  SocketInfo * info = NULL;
  if (_socketInfo.Lookup(socket_id, info) && info) {
    if (!info->_accounted_for &&
        info->_connect_start.QuadPart <= before && 
        info->_connect_end.QuadPart <= before) {
      claimed = true;
      info->_accounted_for = true;
      start = info->_connect_start.QuadPart;
      end = info->_connect_end.QuadPart;
      ssl_start = info->_ssl_start.QuadPart;
      ssl_end = info->_ssl_end.QuadPart;
    }
  }
  LeaveCriticalSection(&cs);
  return claimed;
}

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
ULONG TrackSockets::GetPeerAddress(DWORD socket_id) {
  ULONG peer_address = 0;
  EnterCriticalSection(&cs);
  SocketInfo * info = NULL;
  if (_socketInfo.Lookup(socket_id, info) && info) {
    peer_address = info->_addr.sin_addr.S_un.S_addr;
  }
  LeaveCriticalSection(&cs);
  return peer_address;
}

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
void TrackSockets::SetIsSsl(SOCKET s, bool is_ssl) {
  EnterCriticalSection(&cs);
  SocketInfo* info = GetSocketInfo(s);
  info->_is_ssl = is_ssl;
  LeaveCriticalSection(&cs);
}

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
bool TrackSockets::IsSsl(SOCKET s) {
  EnterCriticalSection(&cs);
  SocketInfo* info = GetSocketInfo(s);
  bool is_ssl = info->_is_ssl;
  LeaveCriticalSection(&cs);
  return is_ssl;
}

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
bool TrackSockets::IsSslById(DWORD socket_id) {
  EnterCriticalSection(&cs);
  SocketInfo* info = GetSocketInfoById(socket_id);
  bool is_ssl = false;
  if (info != NULL) {
    is_ssl = info->_is_ssl;
  }
  LeaveCriticalSection(&cs);
  return is_ssl;
}

/*-----------------------------------------------------------------------------
  This must always be called from within a critical section.
-----------------------------------------------------------------------------*/
SocketInfo* TrackSockets::GetSocketInfo(SOCKET s) {
  DWORD socket_id = 0;
  _openSockets.Lookup(s, socket_id);
  SocketInfo* info = NULL;
  if (!socket_id) {
    socket_id = _nextSocketId;
    _openSockets.SetAt(s, socket_id);
    _nextSocketId++;
  } else {
    _socketInfo.Lookup(socket_id, info);
  }
  if (!info) {
    info = new SocketInfo;
    info->_id = socket_id;
    info->_during_test = _test_state._active;
    _socketInfo.SetAt(info->_id, info);
  }
  if (info->_addr.sin_addr.S_un.S_addr == 0 && info->_connect_start.QuadPart) {
    int addr_len = sizeof(info->_addr);
    getpeername(s, (sockaddr *)&info->_addr, &addr_len);
  }
  return info;
}

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
SocketInfo* TrackSockets::GetSocketInfoById(DWORD socket_id) {
  SocketInfo* info = NULL;
  _socketInfo.Lookup(socket_id, info);
  return info;
}
