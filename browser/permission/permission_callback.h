#ifndef BISON_BROWSER_PERMISSION_PERMISSION_CALLBACK_H_
#define BISON_BROWSER_PERMISSION_PERMISSION_CALLBACK_H_

#include "base/callback.h"

namespace bison {

// Callback for permission requests.
using PermissionCallback = base::OnceCallback<void(bool)>;

}  // namespace bison

#endif  // BISON_BROWSER_PERMISSION_PERMISSION_CALLBACK_H_
