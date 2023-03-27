#include "cmake/build/chatStreamingService.grpc.pb.h"
#include <chrono>
#include <google/protobuf/empty.pb.h>
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <thread>

using chatStreamingService::ChatService;
using chatStreamingService::Message;
using chatStreamingService::Time;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

class ChatServiceClient {
public:
  ChatServiceClient(std::shared_ptr<Channel> channel)
      : stub_(ChatService::NewStub(channel)) {
    // Initialize empty user message data struct
    // the last key found here is used to find newer messages on the server
    auto &messageVector = userMessages[0L];
    messageVector.push_back(std::make_tuple("Dummy", "Message"));
  }

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

    while (true) {
      // Message request we are sending to the server
      Message request;
      request.set_username(name);

      std::string message;

      // Continously write messages to server
      while (message.empty()) {
        // Note: You can use the escape sequence "\033[2K" to clear the current
        // line and move the cursor to the beginning of the line
        std::cout << "\033[2k\r"
                  << "\nEnter a message: ";
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
      // sleep(2);
    }
  }

  Time IsNewMessageAvailable() {
    // Context for the client
    ClientContext context;

    // Get last message time in milliseconds
    long lastMsgTime = userMessages.rbegin()->first;

    // Set request (Time)
    // This is the data we will send to the server
    Time myTime;
    myTime.set_utc(lastMsgTime);
    myTime.set_isnew(false);

    // Container for the data we expect from the server
    Time replyTime;

    // The RPC
    Status status = stub_->IsNewMessageAvailable(&context, myTime, &replyTime);

    // Act upon its status
    if (!status.ok()) {
      std::cout << "Status not OK from server" << std::endl;

      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
    }

    return replyTime;
  }

  void ReceiveMessage() {
    // I used to have the ClientContext and Message container for server
    // response defined here. But I kept getting errors after sending a second
    // message. I believe its because we have to refresh the context and
    // response between every stream

    while (true) {
      sleep(1);

      // Check if there are new messages available
      Time check = IsNewMessageAvailable();
      if (check.isnew()) {
        // Client Request to server, the server uses the utc time to pull
        // messages newer than that time
        Time request;
        request.set_utc(check.utc());
        request.set_isnew(check.isnew());

        // Context for the client
        ClientContext context;

        // Container for Server response
        Message response;

        // Call the ReceiveMessage RPC and get the response as a stream
        std::shared_ptr<grpc::ClientReader<Message>> stream(
            stub_->ReceiveMessage(&context, request));

        // Stream in the new messages from the server
        while (stream->Read(&response)) {
          // Note: You can use the escape sequence "\033[2K" to clear the
          // current line and move the cursor to the beginning of the line
          // \033[35m Magenta
          // \033[0m return color to default
          std::cout << "\033[2K\r \t["
                    << "\033[35m" << response.username() << "\033[0m]"
                    << ": " << response.message() << std::endl;
        }
        // This cout is not printed to terminal unless we flush it
        // other this gets stored in the buffer and there's no output
        std::cout << "\nEnter a message: " << std::flush;

        // Get current time in milliseconds
        auto now = std::chrono::system_clock::now();
        auto now_ms =
            std::chrono::time_point_cast<std::chrono::milliseconds>(now);
        auto value = now_ms.time_since_epoch().count();
        long ms_since_epoch = static_cast<long>(value);

        // Add server message to client userMessage data structure
        // This acts as a local copy of messages for the client
        auto &messageVector = userMessages[ms_since_epoch];
        messageVector.push_back(
            std::make_tuple(response.username(), response.message()));
      }
    }
  };

  // For DEBUGGING
  void printUserMessages() {
    for (const auto &kv : userMessages) {
      std::cout << "Key: " << kv.first << std::endl;

      const auto &tuples = kv.second;
      for (const auto &tup : tuples) {
        const std::string &name = std::get<0>(tup);
        const std::string &message = std::get<1>(tup);

        std::cout << "  Username: " << name << "  Message: " << message
                  << std::endl;
      }

      std::cout << std::endl;
    }
  }

private:
  // Used to call RPC methods
  std::shared_ptr<ChatService::Stub> stub_;
  // Client's copy of user messages
  std::map<long, std::vector<std::tuple<std::string, std::string>>>
      userMessages;
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