#pragma once

#include <string>
#include <stdint.h>
#include <msgpack.hpp>
#include "cmdArgs.hpp"
#include <msgpack.hpp>
#include <msgpack/rpc/server.h>

#define MODE_SERVER_READ 1 << 0
#define MODE_SERVER_WRITE 1 << 1
#define MODE_KEEP_ASPECT 1 << 2
#define MODE_MAGICK 1 << 3

namespace rpc {
        using namespace msgpack;
        using namespace msgpack::rpc;
}  // namespace rpc
typedef rpc::request request;

using namespace std;
using msgpack::type::raw_ref;

enum jobStatus {
  jobStatusOk=0,
  jobStatusFail,
  jobStatusUnableToOpenInput,
  jobStatusErrorReadingInput,
  jobStatusUnableToOpenOutput,
  jobStatusErrorWritingOutput,
  jobStatusReadNotAllowed,
  jobStatusWriteNotAllowed,
};

struct jobDefinition {
  int quality;
  int scale;
  int sizeX;
  int sizeY;
  uint32_t jobMode;
  string inputFile;
  string outputFile;
  raw_ref inputBuffer;
  raw_ref outputBuffer;
  int threadId;
  jobStatus status;
  request m_req;
  jobDefinition(request req) : m_req(req) {
    quality=appArguments.quality;
    scale=0;
    sizeX=0;
    sizeY=0;
    jobMode=0;
    inputFile="";
    outputFile="";
    threadId=-1;
    status=jobStatusOk;
  };
};
