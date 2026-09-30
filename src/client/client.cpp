#include <atomic>
#include <atomic>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#endif

#include <grpcpp/grpcpp.h>

#include "chat.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReaderWriter;
using grpc::Status;

using chat::ChatService;
using chat::MessageReply;
using chat::MessageRequest;

class ChatClient {
 public:
  explicit ChatClient(std::shared_ptr<Channel> channel)
      : stub_(ChatService::NewStub(channel)) {}

  // 双向流对话:返回一个 stream,可在主线程 continue 发送
  std::shared_ptr<ClientReaderWriter<MessageRequest, MessageReply>> StartChat(
      const std::string& me) {
    ClientContext* ctx = new ClientContext();
    auto stream = stub_->Chat(ctx);
    // 保存我自己的名字(在 writer 线程用)
    me_ = me;
    return stream;
  }

  // 在后台线程持续接收服务端来的消息并打印
  void StartReceiveLoop(
      const std::shared_ptr<ClientReaderWriter<MessageRequest, MessageReply>>&
          stream) {
    running_ = true;
    recv_thread_ = std::thread([this, stream]() {
      MessageReply reply;
      while (running_ && stream->Read(&reply)) {
        std::cout << std::endl
                  << "[服务端]: " << reply.content() << std::endl
                  << "[你] 输入消息 (exit 退出): ";
        std::cout.flush();
      }
    });
  }

  // 发送一条消息
  void Send(const std::shared_ptr<ClientReaderWriter<MessageRequest, MessageReply>>&
                stream,
            const std::string& content) {
    MessageRequest req;
    req.set_from(me_);
    req.set_content(content);
    stream->Write(req);
  }

  void Stop() {
    running_ = false;
    if (recv_thread_.joinable()) recv_thread_.join();
  }

  std::string me_;
  std::atomic<bool> running_{false};
  std::thread recv_thread_;
  std::unique_ptr<ChatService::Stub> stub_;
};

int main() {
#ifdef _WIN32
  // 输入+输出代码页都设为 UTF-8: 输入的中文按 UTF-8 进程序, 避免 protobuf 校验失败和乱码
  SetConsoleCP(CP_UTF8);
  SetConsoleOutputCP(CP_UTF8);
#endif

  auto channel = grpc::CreateChannel("localhost:50051",
                                     grpc::InsecureChannelCredentials());
  ChatClient client(channel);

  // 建立双向流
  auto stream = client.StartChat("我");
  client.StartReceiveLoop(stream);

  std::cout << "====== 对话已建立 ======" << std::endl
            << "输入消息回车发送, 输入 exit 退出" << std::endl
            << "[你] 输入消息: ";
  std::cout.flush();

  std::string line;
  while (std::getline(std::cin, line)) {
    if (line == "exit" || line == "quit" || line == "q") {
      stream->WritesDone();
      break;
    }
    if (!line.empty()) {
      client.Send(stream, line);
    }
    std::cout << "[你] 输入消息: ";
    std::cout.flush();
  }

  client.Stop();
  return 0;
}