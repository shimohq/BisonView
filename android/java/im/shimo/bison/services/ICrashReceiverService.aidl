// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package im.shimo.bison.services;

interface ICrashReceiverService {
    void transmitCrashes(in ParcelFileDescriptor[] fileDescriptors, in List crashInfo);
}
