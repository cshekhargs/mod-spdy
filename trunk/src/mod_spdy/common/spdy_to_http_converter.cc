// Copyright 2010 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "mod_spdy/common/spdy_to_http_converter.h"

#include "mod_spdy/common/http_stream_visitor_interface.h"

namespace {

const char *kMethod = "method";
const char *kUrl = "url";
const char *kVersion = "version";

}  // namespace

namespace mod_spdy {

SpdyToHttpConverter::SpdyToHttpConverter(flip::FlipFramer *framer,
                                         HttpStreamVisitorInterface *visitor)
    : framer_(framer), visitor_(visitor) {
}

SpdyToHttpConverter::~SpdyToHttpConverter() {}

void SpdyToHttpConverter::OnError(flip::FlipFramer *framer) {
  // Not yet supported.
  CHECK(false);
}

void SpdyToHttpConverter::OnControl(const flip::FlipControlFrame *frame) {
  // For now we support a subset of FLIP. Crash if we receive a frame
  // we don't yet know how to process.
  CHECK(frame->type() == flip::SYN_STREAM);
  CHECK(frame->flags() & flip::CONTROL_FLAG_FIN);

  flip::FlipHeaderBlock block;
  if (!framer_->ParseHeaderBlock(frame, &block)) {
    // TODO: handle this case
    CHECK(false);
    return;
  }

  CHECK(block.count(kMethod) == 1);
  CHECK(block.count(kUrl) == 1);
  CHECK(block.count(kVersion) == 1);

  // Technically we should decode the URL into a path and a
  // Host. Instead we pass the full URL on to the visitor and leave it
  // up to the visitor to extract Host and path.
  visitor_->OnStatusLine(block[kMethod].c_str(),
                         block[kUrl].c_str(),
                         block[kVersion].c_str());

  for (flip::FlipHeaderBlock::const_iterator it = block.begin(),
           it_end = block.end();
       it != it_end;
       ++it) {
    std::string key = it->first;
    std::string value = it->second;
    if (key == kMethod ||
        key == kUrl ||
        key == kVersion) {
      continue;
    }

    // Split header values on null characters, emitting a separate
    // header key-value pair for each substring. Logic from
    // net/flip/flip_session.cc
    for (size_t start = 0, end = 0; end != value.npos; start = end) {
      start = value.find_first_not_of('\0', start);
      if (start == value.npos) {
        break;
      }
      end = value.find('\0', start);
      std::string tval;
      if (end != value.npos) {
        tval = value.substr(start, (end - start));
      } else {
        tval = value.substr(start);
      }
      visitor_->OnHeader(key.c_str(), tval.c_str());
    }
  }

  visitor_->OnHeadersComplete();
}

void SpdyToHttpConverter::OnStreamFrameData(flip::FlipStreamId stream_id,
                                            const char *data,
                                            size_t len) {
  // Not yet supported.
  CHECK(false);
}

}  // namespace mod_spdy
