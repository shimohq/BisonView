// create by jiang947

#ifndef BISON_BROWSER_INPUT_STREAM_H_
#define BISON_BROWSER_INPUT_STREAM_H_

#include <stdint.h>

#include "base/android/scoped_java_ref.h"
#include "base/compiler_specific.h"

namespace net {
class IOBuffer;
}

namespace bison {
class InputStream {
 public:
  // Maximum size of |buffer_|.
  static const int kBufferSize;

  // |stream| should be an instance of the InputStream Java class.
  // |stream| can't be null.
  InputStream(const base::android::JavaRef<jobject>& stream);
  virtual ~InputStream();

  // Gets the underlying Java object. Guaranteed non-NULL.
  const base::android::JavaRef<jobject>& jobj() const { return jobject_; }

  // Sets |bytes_available| to the number of bytes that can be read (or skipped
  // over) from this input stream without blocking by the next caller of a
  // method for this input stream.
  // Returns true if completed successfully or false if an exception was
  // thrown.
  virtual bool BytesAvailable(int* bytes_available) const;

  // Skips over and discards |n| bytes of data from this input stream. Sets
  // |bytes_skipped| to the number of of bytes skipped.
  // Returns true if completed successfully or false if an exception was
  // thrown.
  virtual bool Skip(int64_t n, int64_t* bytes_skipped);

  // Reads at most |length| bytes into |dest|. Sets |bytes_read| to the total
  // number of bytes read into |dest| or 0 if there is no more data because the
  // end of the stream was reached.
  // |dest| must be at least |length| in size.
  // Returns true if completed successfully or false if an exception was
  // thrown.
  virtual bool Read(net::IOBuffer* dest, int length, int* bytes_read);

 protected:
  // Parameterless constructor exposed for testing.
  InputStream();

 private:
  base::android::ScopedJavaGlobalRef<jobject> jobject_;
  base::android::ScopedJavaGlobalRef<jbyteArray> buffer_;
};

}  // namespace bison

#endif  // BISON_BROWSER_INPUT_STREAM_H_
