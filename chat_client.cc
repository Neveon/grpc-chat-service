#include "cmake/build/chatStreamingService.grpc.pb.h"
#include <google/protobuf/empty.pb.h>
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <thread>

using chatStreamingService::ChatService;
using chatStreamingService::Message;
using grpc::Channel;
using grpc::ClientContext;
// using grpc::Status;

class ChatServiceClient {
public:
  ChatServiceClient(std::shared_ptr<Channel> channel)
      : stub_(ChatService::NewStub(channel)) {}

  void SendMessage(const std::string &name) {

    // Container for the data we expect from the server
    // ChatMessage response;
    google::protobuf::Empty response;

    // Context for the client
    // Extra information for the server and/or tweak certain RPC behaviors
    ClientContext context;

    // The actual RPC
    // Status status = stub_->SendMessage(&context, message, &response);

    // Synchronous (blocking) client-side API for directional streaming RPCs
    std::shared_ptr<grpc::ClientWriter<Message>> stream =
        stub_->SendMessage(&context, &response);

    // Init wait for ReceiveMessage() to stream messages to client first
    sleep(2);

    while (true) {
      // Message request we are sending to the server
      Message request;
      request.set_username(name);

      std::string message;

      // Wait 1 second so debugging prints in ReceiveMessage go through
      // sleep(2);

      // Continously write messages to server
      while (message.empty()) {
        std::cout << "Enter a message: ";
        std::getline(std::cin, message);
      }

      request.set_message(message);

      // Check if request was written to server successfully
      if (!stream->Write(request)) {
        std::cout << "Failure occured writing to server.." << std::endl;

        // End stream
        // stream->WritesDone();
        stream->Finish();

        break;
      }
      // Wait a second for the message to process in the server side
      sleep(3);
    }
  }

  void ReceiveMessage() {
    // Context for the client
    ClientContext context;

    // Empty Request
    google::protobuf::Empty request;

    // Call the ReceiveMessage RPC and get the response as a stream
    std::shared_ptr<grpc::ClientReader<Message>> stream(
        stub_->ReceiveMessage(&context, request));

    // DEBUGGING
    // std::cout << "Subscribing to server to receive messages..." << std::endl;

    Message server_message;

    while (true) {
      // DEBUGGING
      // std::cout << "Reading Messages..." << std::endl;

      while (stream->Read(&server_message)) {
        std::cout << server_message.username() << ": "
                  << server_message.message() << std::endl;
      }
    }
  };

private:
  std::shared_ptr<ChatService::Stub> stub_;
};

int main(int argc, char **argv) {
  // Channel to Send Messages
  ChatServiceClient clientSend(grpc::CreateChannel(
      "localhost:50051", grpc::InsecureChannelCredentials()));

  // Channel to Receive Messages
  ChatServiceClient clientReceive(grpc::CreateChannel(
      "localhost:50051", grpc::InsecureChannelCredentials()));

  std::string name;
  while (name.empty()) {
    std::cout << "What is your name? ";
    std::getline(std::cin, name);
  }

  std::cout << "Welcome " << name << "!" << std::endl;

  // Start a new thread to receive messages.
  std::thread t([&]() { clientReceive.ReceiveMessage(); });

  clientSend.SendMessage(name);

  // Wait for the thread to complete (i.e., the client shuts down).
  t.join();

  return 0;
}