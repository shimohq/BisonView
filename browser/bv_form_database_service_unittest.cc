// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bison/browser/bv_form_database_service.h"

#include <memory>
#include <vector>

#include "base/android/jni_android.h"
#include "base/files/scoped_temp_dir.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/task_environment.h"
#include "components/autofill/core/browser/webdata/autofill_webdata_service.h"
#include "components/autofill/core/common/form_field_data.h"
#include "testing/gtest/include/gtest/gtest.h"

using autofill::AutofillWebDataService;
using autofill::FormFieldData;
using base::android::AttachCurrentThread;
using testing::Test;

namespace bison {

class BvFormDatabaseServiceTest : public Test {
 public:
  BvFormDatabaseServiceTest() {}

 protected:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    env_ = AttachCurrentThread();
    ASSERT_TRUE(env_ != NULL);

    service_.reset(new BvFormDatabaseService(temp_dir_.GetPath()));
  }

  void TearDown() override {
    service_->Shutdown();
    task_environment_.RunUntilIdle();
  }

  // The path to the temporary directory used for the test operations.
  base::test::TaskEnvironment task_environment_;
  base::ScopedTempDir temp_dir_;
  JNIEnv* env_;
  std::unique_ptr<BvFormDatabaseService> service_;
};

TEST_F(BvFormDatabaseServiceTest, HasAndClearFormData) {
  EXPECT_FALSE(service_->HasFormData());
  std::vector<FormFieldData> fields;
  FormFieldData field;
  field.name = base::ASCIIToUTF16("foo");
  field.value = base::ASCIIToUTF16("bar");
  fields.push_back(field);
  service_->get_autofill_webdata_service()->AddFormFields(fields);
  EXPECT_TRUE(service_->HasFormData());
  service_->ClearFormData();
  EXPECT_FALSE(service_->HasFormData());
}

}  // namespace bison_view
