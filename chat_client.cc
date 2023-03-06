#include "cmake/build/chatStreamingService.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <memory>
#include <string>
#include <thread>


using chatStreamingService::LoginRequest;
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

    std::string JoinChat(const std::string & name) {
      // Name we are sending to the server
      LoginRequest request;
      request.set_name(name);

      // Container for the data we expect from the server
      MessageResponse response;

      // Context for the client
      // Extra information for the server and/or tweak certain RPC behaviors
      ClientContext context;

      // The actual RPC
      Status status = stub_->JoinChat(&context, request, &response);

      // Act upon its status.
      if (status.ok()){
        std::cout << response.message() << std::endl;
        return response.message();
      } else {
        std::cout << status.error_code() << ": " << status.error_message() << std::endl;
        return "gRPC failed";
      }
    }

    // void Chat(const std::string &name) {
    //   ClientContext context;
    //   std::shared_ptr<grpc::ClientReaderWriter<MessageRequest, MessageResponse>> stream(
    //       stub_->Chat(&context));

    //   std::thread writer_thread([&stream, name]() {
    //     MessageRequest request;

    //     request.set_name(name);
    //     stream->Write(request);
    //   });

    //   MessageResponse response;
    //   while (stream->Read(&response)) {
    //     std::cout << "Got message: '" << response.message() << "'" << std::endl;
    //   }

    //   stream->WritesDone();
    //   writer_thread.join();

    //   Status status = stream->Finish();
    //   if (status.ok()) {
    //     std::cout << "Chat ended" << std::endl;
    //   } else {
    //     std::cout << "Chat failed: " << status.error_message() << std::endl;
    //   }
    // };

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

  client.JoinChat(name);

  return 0;
}