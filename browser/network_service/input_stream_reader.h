// create by jiang947

#ifndef BISON_BROWSER_NETWORK_SERVICE_INPUT_STREAM_READER_H_
#define BISON_BROWSER_NETWORK_SERVICE_INPUT_STREAM_READER_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"

namespace net {
class HttpByteRange;
class IOBuffer;
}  // namespace net

namespace bison {

class InputStream;

// Class responsible for reading the InputStream.
class InputStreamReader {
 public:
  // The constructor is called on the IO thread, not on the worker thread.
  InputStreamReader(bison::InputStream* stream);
  virtual ~InputStreamReader();

  // Perform a seek operation on the InputStream associated with this job.
  // On successful completion the InputStream would have skipped reading the
  // number of bytes equal to the lower range of |byte_range|.
  // This method should be called on the |g_worker_thread| thread.
  //
  // |byte_range| is the range of bytes to be read from |stream|
  //
  // A negative return value will indicate an error code, a positive value
  // will indicate the expected size of the content.
  virtual int Seek(const net::HttpByteRange& byte_range);

  // Read data from |stream_|.  This method should be called on the
  // |g_worker_thread| thread.
  //
  // A negative return value will indicate an error code, a positive value
  // will indicate the expected size of the content.
  virtual int ReadRawData(net::IOBuffer* buffer, int buffer_size);

 private:
  // Verify the requested range against the stream size.
  // net::OK is returned on success, the error code otherwise.
  int VerifyRequestedRange(net::HttpByteRange* byte_range, int* content_size);

  // Skip to the first byte of the requested read range.
  // net::OK is returned on success, the error code otherwise.
  int SkipToRequestedRange(const net::HttpByteRange& byte_range);

  bison::InputStream* stream_;

  DISALLOW_COPY_AND_ASSIGN(InputStreamReader);
};

}  // namespace bison

#endif  // BISON_BROWSER_NETWORK_SERVICE_INPUT_STREAM_READER_H_