// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "peridot/bin/ledger/app/merging/test_utils.h"

#include <lib/async/dispatcher.h>

#include "garnet/lib/callback/capture.h"
#include "gtest/gtest.h"
#include "lib/fsl/tasks/message_loop.h"
#include "peridot/bin/ledger/app/constants.h"
#include "peridot/bin/ledger/encryption/primitives/hash.h"
#include "peridot/bin/ledger/storage/impl/page_storage_impl.h"
#include "peridot/bin/ledger/storage/public/constants.h"

namespace ledger {
namespace test {
TestBackoff::TestBackoff(int* get_next_count)
    : get_next_count_(get_next_count) {}
TestBackoff::~TestBackoff() {}

zx::duration TestBackoff::GetNext() {
  FXL_DCHECK(get_next_count_);
  (*get_next_count_)++;
  return zx::sec(0);
}

void TestBackoff::Reset() {}

TestWithPageStorage::TestWithPageStorage()
    : encryption_service_(message_loop_.async()){};

TestWithPageStorage::~TestWithPageStorage() {}

std::function<void(storage::Journal*)>
TestWithPageStorage::AddKeyValueToJournal(const std::string& key,
                                          std::string value) {
  return
      [this, key, value = std::move(value)](storage::Journal* journal) mutable {
        storage::Status status;
        storage::ObjectIdentifier object_identifier;
        page_storage()->AddObjectFromLocal(
            storage::DataSource::Create(std::move(value)),
            callback::Capture(MakeQuitTask(), &status, &object_identifier));
        RunLoop();
        EXPECT_EQ(storage::Status::OK, status);

        journal->Put(key, object_identifier, storage::KeyPriority::EAGER,
                     callback::Capture(MakeQuitTask(), &status));
        RunLoop();
        EXPECT_EQ(storage::Status::OK, status);
      };
}

std::function<void(storage::Journal*)>
TestWithPageStorage::DeleteKeyFromJournal(const std::string& key) {
  return [this, key](storage::Journal* journal) {
    storage::Status status;
    journal->Delete(key, callback::Capture(MakeQuitTask(), &status));
    RunLoop();
    EXPECT_EQ(storage::Status::OK, status);
  };
}

::testing::AssertionResult TestWithPageStorage::GetValue(
    storage::ObjectIdentifier object_identifier,
    std::string* value) {
  storage::Status status;
  std::unique_ptr<const storage::Object> object;
  page_storage()->GetObject(
      std::move(object_identifier), storage::PageStorage::Location::LOCAL,
      callback::Capture(MakeQuitTask(), &status, &object));
  RunLoop();
  if (status != storage::Status::OK) {
    return ::testing::AssertionFailure()
           << "PageStorage::GetObject returned status: " << status;
  }

  fxl::StringView data;
  status = object->GetData(&data);
  if (status != storage::Status::OK) {
    return ::testing::AssertionFailure()
           << "Object::GetData returned status: " << status;
  }

  *value = data.ToString();
  return ::testing::AssertionSuccess();
}

::testing::AssertionResult TestWithPageStorage::CreatePageStorage(
    std::unique_ptr<storage::PageStorage>* page_storage) {
  std::unique_ptr<storage::PageStorageImpl> local_page_storage =
      std::make_unique<storage::PageStorageImpl>(
          message_loop_.async(), &coroutine_service_,
          &encryption_service_, tmp_dir_.path(), kRootPageId.ToString());
  storage::Status status;
  local_page_storage->Init(callback::Capture(MakeQuitTask(), &status));
  RunLoop();

  if (status != storage::Status::OK) {
    return ::testing::AssertionFailure()
           << "PageStorageImpl::Init returned status: " << status;
  }
  *page_storage = std::move(local_page_storage);
  return ::testing::AssertionSuccess();
}

}  // namespace test
}  // namespace ledger
