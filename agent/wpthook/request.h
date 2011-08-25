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

#pragma once

class TestState;
class TrackSockets;
class TrackDns;
class WptTest;

class DataChunk {
public:
  DataChunk() : _unowned_data(NULL), _data(NULL), _data_len(0) {}
  DataChunk(const char * unowned_data, DWORD data_len) :
      _unowned_data(NULL), _data(NULL), _data_len(0) {
    if (unowned_data && data_len) {
      _unowned_data = unowned_data;
      _data_len = data_len;
    }
  }
  DataChunk(const DataChunk& src) { *this = src; }
  ~DataChunk(){}
  const DataChunk& operator=(const DataChunk& src) {
    _unowned_data = src._unowned_data;
    _data = src._data;
    _data_len = src._data_len;
    return *this;
  }
  void Free() {
    if (_data) {
      free(_data);
    }
    _unowned_data = NULL;
    _data = NULL;
    _data_len = 0;
  }

  /*---------------------------------------------------------------------------
     The caller must always call Free() on the returned DataChunk.
     If |this| has unowned data, the data is copied to the return value.
  ---------------------------------------------------------------------------*/
  DataChunk GiveAwayOwnership() {
    DataChunk new_chunk;
    if (_unowned_data) {
      char *new_data = (char *)malloc(_data_len);
      memcpy(new_data, _unowned_data, _data_len);
      new_chunk.TakeOwnership(new_data, _data_len);
    } else {
      new_chunk.TakeOwnership(_data, _data_len);
      _unowned_data = _data;
      _data = NULL;
    }
    return new_chunk;
  }
  /*---------------------------------------------------------------------------
     |data| is expected to be malloc'd memory. Call Free() to free it.
  ---------------------------------------------------------------------------*/
  void TakeOwnership(char *data, DWORD data_len) {
    Free();
    _unowned_data = NULL;
    _data = data;
    _data_len = data_len;
  }
  const char * GetData() { return _unowned_data ? _unowned_data : _data; }
  DWORD GetLength() { return _data_len; }

private:
  const char * _unowned_data;
  char *       _data;
  DWORD        _data_len;
};

class HeaderField {
public:
  HeaderField(){}
  HeaderField(const HeaderField& src){*this = src;}
  ~HeaderField(){}
  const HeaderField& operator=(const HeaderField& src) {
    _field = src._field;
    _value = src._value;
    return src;
  }

  CStringA  _field;
  CStringA  _value;
};

typedef CAtlList<HeaderField> Fields;

class OptimizationScores {
public:
  OptimizationScores():
    _keep_alive_score(-1)
    , _gzip_score(-1)
    , _gzip_total(0)
    , _gzip_target(0)
    , _image_compression_score(-1)
    , _image_compress_total(0)
    , _image_compress_target(0)
    , _cache_score(-1)
    , _cache_time_secs(-1)
    , _combine_score(-1)
    , _static_cdn_score(-1)
  {}
  ~OptimizationScores() {}
  int _keep_alive_score;
  int _gzip_score;
  DWORD _gzip_total;
  DWORD _gzip_target;
  int _image_compression_score;
  DWORD _image_compress_total;
  DWORD _image_compress_target;
  int _cache_score;
  DWORD _cache_time_secs;
  int _combine_score;
  int _static_cdn_score;
  CStringA _cdn_provider;
};

class Request {
public:
  Request(TestState& test_state, DWORD socket_id, 
          TrackSockets& sockets, TrackDns& dns, WptTest& test);
  ~Request(void);

  void DataIn(DataChunk& chunk);
  bool ModifyDataOut(DataChunk& chunk);
  void DataOut(DataChunk& chunk);
  void SocketClosed();
  bool Process();
  bool IsStatic();
  bool IsText();
  CStringA GetHost();
  CStringA GetMime();
  LARGE_INTEGER GetStartTime();
  void GetExpiresTime(long& age_in_seconds, bool& exp_present, bool& cache_control_present);
  ULONG GetPeerAddress();

  DWORD _data_sent;
  DWORD _data_received;
  DWORD _socket_id;
  bool  _processed;
  bool  _is_ssl;

  // times (in ms from the test start)
  int _ms_start;
  int _ms_first_byte;
  int _ms_end;
  int _ms_connect_start;
  int _ms_connect_end;
  int _ms_dns_start;
  int _ms_dns_end;
  int _ms_ssl_start;
  int _ms_ssl_end;

  // header data
  CStringA  _in_header;
  CStringA  _out_header;
  CStringA  _method;
  CStringA  _object;
  int       _result;
  double    _protocol_version;
  ULONG     _peer_address;

  // processed data
  unsigned char * _body_in;
  DWORD           _body_in_size;

  // Optimization score data.
  OptimizationScores _scores;

  CStringA  GetRequestHeader(CStringA header);
  CStringA  GetResponseHeader(CStringA header);

private:
  TestState&    _test_state;
  WptTest&      _test;
  TrackSockets& _sockets;
  TrackDns&     _dns;
  LARGE_INTEGER _start;
  LARGE_INTEGER _first_byte;
  LARGE_INTEGER _end;

  CRITICAL_SECTION cs;
  bool      _active;
  Fields    _in_fields;
  Fields    _out_fields;
  bool      _are_headers_complete;

  // merged data chunks
  char *  _data_in;
  char *  _data_out;
  DWORD   _data_in_size;
  DWORD   _data_out_size;
  bool    _body_in_allocated;

  // data transmitted in the chunks as it was transmitted
  CAtlList<DataChunk> _data_chunks_in;
  CAtlList<DataChunk> _data_chunks_out;

  void FreeChunkMem();
  void CombineChunks();
  bool FindHeader(const char * data, CStringA& header);
  void ProcessRequest();
  void ProcessResponse();
  void ExtractFields(CStringA& header, Fields& fields);
  CStringA GetHeaderValue(Fields& fields, CStringA header);
  void DechunkResponse();
};
