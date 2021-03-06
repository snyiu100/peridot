// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "peridot/lib/socket/socket_writer.h"

#include <utility>

#include "gtest/gtest.h"
#include "lib/fsl/tasks/message_loop.h"
#include "lib/fxl/macros.h"
#include "peridot/lib/socket/socket_drainer_client.h"
#include "peridot/lib/socket/socket_pair.h"

namespace socket {
namespace {

class StringClient : public SocketWriter::Client {
 public:
  explicit StringClient(std::string value) : value_(std::move(value)) {}

  void GetNext(size_t offset,
               size_t max_size,
               std::function<void(fxl::StringView)> callback) override {
    fxl::StringView data = value_;
    callback(data.substr(offset, max_size));
  }

  void OnDataComplete() override { completed_ = true; }

  bool completed() { return completed_; }

 private:
  std::string value_;
  bool completed_ = false;
};

TEST(SocketWriter, WriteAndRead) {
  fsl::MessageLoop message_loop;
  SocketPair socket;
  StringClient client("bazinga\n");
  SocketWriter writer(&client);
  writer.Start(std::move(socket.socket1));

  std::string value;
  auto drainer = std::make_unique<SocketDrainerClient>();
  drainer->Start(std::move(socket.socket2),
                 [&value, &message_loop](const std::string& v) {
                   value = v;
                   message_loop.PostQuitTask();
                 });
  message_loop.Run();

  EXPECT_EQ("bazinga\n", value);
  EXPECT_TRUE(client.completed());
}

TEST(SocketWriter, ClientClosedTheirEnd) {
  fsl::MessageLoop message_loop;
  SocketPair socket;
  StringClient client("bazinga\n");
  SocketWriter writer(&client);
  socket.socket2.reset();
  writer.Start(std::move(socket.socket1));
  EXPECT_TRUE(client.completed());
}

TEST(SocketWriter, StringSocketWriter) {
  fsl::MessageLoop message_loop;
  SocketPair socket;
  StringSocketWriter* writer = new StringSocketWriter();
  writer->Start("bazinga\n", std::move(socket.socket1));

  std::string value;
  auto drainer = std::make_unique<SocketDrainerClient>();
  drainer->Start(std::move(socket.socket2),
                 [&value, &message_loop](const std::string& v) {
                   value = v;
                   message_loop.PostQuitTask();
                 });
  message_loop.Run();

  EXPECT_EQ("bazinga\n", value);
}

}  // namespace
}  // namespace socket
