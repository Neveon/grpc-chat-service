#include "cmake/build/chatStreamingService.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <thread>


using chatStreamingService::LoginRequest;
using chatStreamingService::ChatMessage;
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
      ChatMessage response;

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

    void Chat(const std::string &name) {
      // Context for the client
      ClientContext context;

      // Synchronous (blocking) client-side API for bi-directional streaming RPCs
      std::shared_ptr<grpc::ClientReaderWriter<ChatMessage, ChatMessage>> stream(stub_->Chat(&context));

      while(true) {
        // In separate thread, read messages from other users (that server sends)
        std::thread read_thread([&]() {
          ChatMessage server_message;
          while(stream->Read(&server_message)){
            std::cout << server_message.from() << ": " << server_message.message() << std::endl;
          }
        });

        // Client message container
        ChatMessage client_message;
        std::string message;
        // std::cin >> message;
        while (message.empty()) {
          std::cout << "Enter a message: ";
          std::getline(std::cin, message);
        }

        client_message.set_from(name);
        client_message.set_message(message);
        // Send message to server
        if(!stream->Write(client_message)) {
          std::cout << "Failure occured writing to server.." << std::endl;
          break;
        }

        stream->WritesDone();
        read_thread.join();

        // Status status = stream->Finish();
        // if (status.ok()) {
        //   std::cout << "Chat ended" << std::endl;
        // } else {
        //   std::cout << "Chat failed: " << status.error_message() << std::endl;
        // }
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

  client.JoinChat(name);

  try {
    client.Chat(name);
  } catch (std::exception& e) {
    // Catch any other exceptions of type std::exception
    std::cerr << "Caught exception: " << e.what() << std::endl;
  }

  return 0;
}