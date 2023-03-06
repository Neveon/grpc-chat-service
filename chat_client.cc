#include "cmake/build/chatStreamingService.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <memory>
#include <string>
#include <thread>


using chatStreamingService::MessageRequest;
using chatStreamingService::MessageResponse;
using chatStreamingService::ChatService;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

class ChatServiceClient {
  public:
    ChatServiceClient(std::shared_ptr<Channel> channel)
        : stub_(ChatService::NewStub(channel)) {}

    void Chat(const std::string &name) {
      ClientContext context;
      std::shared_ptr<grpc::ClientReaderWriter<MessageRequest, MessageResponse>> stream(
          stub_->Chat(&context));

      std::thread writer_thread([&stream, name]() {
        MessageRequest request;

        request.set_name(name);
        stream->Write(request);
      });

      MessageResponse response;
      while (stream->Read(&response)) {
        std::cout << "Got message: '" << response.message() << "'" << std::endl;
      }

      stream->WritesDone();
      writer_thread.join();

      Status status = stream->Finish();
      if (status.ok()) {
        std::cout << "Chat ended" << std::endl;
      } else {
        std::cout << "Chat failed: " << status.error_message() << std::endl;
      }
    };

  private:
    std::unique_ptr<ChatService::Stub> stub_;
};

int main(int argc, char** argv) {
  ChatServiceClient client(grpc::CreateChannel(
      "localhost:50051", grpc::InsecureChannelCredentials()));
  // client.Chat("Client1");

  std::cout << "What is your name? ";
  std::string name;
  std::cin >> name;
  std::cout << "Hello " << name << "!" << std::endl; 

  client.Chat(name);

  return 0;
}