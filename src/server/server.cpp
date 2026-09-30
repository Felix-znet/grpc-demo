#include <atomic>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#endif

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "chat.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReaderWriter;
using grpc::Status;

using chat::ChatService;
using chat::MessageReply;
using chat::MessageRequest;

class ChatServiceImpl final : public ChatService::Service {
 public:
  // 双向流式对话
  Status Chat(ServerContext* /*ctx*/,
              ServerReaderWriter<MessageReply, MessageRequest>* stream) override {
    // 标志: 是否仍在接收客户端消息
    running_ = true;

    // 后台线程: 持续读客户端发来的消息
    recv_thread_ = std::thread([this, stream]() {
      MessageRequest req;
      while (running_ && stream->Read(&req)) {
        std::cout << std::endl
                  << "[收到客户端]: " << req.content() << std::endl
                  << "[服务端] 输入回复 (exit 退出, 直接回车跳到下一行): ";
        std::cout.flush();
      }
    });

    // 主线程: 读取要回复给客户端的内容
    std::string line;
    while (running_) {
      std::getline(std::cin, line);
      if (line == "exit" || line == "quit" || line == "q") {
        break;
      }
      if (!line.empty()) {
        MessageReply reply;
        reply.set_receiver("客户端");
        reply.set_content(line);
        // 把一个回复写回 stream
        std::lock_guard<std::mutex> lock(write_mu_);
        stream->Write(reply);
        std::cout << "[服务端] 已发送: " << line << std::endl;
      }
    }

    running_ = false;
    if (recv_thread_.joinable()) recv_thread_.join();
    return Status::OK;
  }

 private:
  std::atomic<bool> running_{false};
  std::thread recv_thread_;
  std::mutex write_mu_;
};

void RunServer() {
  std::string address("0.0.0.0:50051");
  ChatServiceImpl service;

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();

  ServerBuilder builder;
  builder.AddListeningPort(address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);

  std::unique_ptr<Server> server = builder.BuildAndStart();
  std::cout << "服务端已启动, 监听: " << address << std::endl
            << "等待客户端连接... (对话中按 exit 可退出)" << std::endl;
  std::cout.flush();

  server->Wait();
}

int main() {
#ifdef _WIN32
  // 输入+输出代码页都设为 UTF-8: 输入的中文按 UTF-8 进程序, 避免 protobuf 校验失败和乱码
  SetConsoleCP(CP_UTF8);
  SetConsoleOutputCP(CP_UTF8);
#endif
  RunServer();
  return 0;
}