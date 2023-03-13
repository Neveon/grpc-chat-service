#include "cmake/build/chatStreamingService.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <memory>
#include <string>


using chatStreamingService::LoginRequest;
using chatStreamingService::ChatMessage;
using chatStreamingService::ChatService;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

class ChatServiceImpl final : public ChatService::Service {
  public:
    ChatServiceImpl() {
      // Initialize the streams map
      streams_ = std::map<std::string, std::unique_ptr<grpc::ServerReaderWriter<ChatMessage, ChatMessage>>>();
    }

    Status JoinChat(ServerContext * context, const LoginRequest * request, ChatMessage * response) override {
      std::string serverReceiveMessage = "Server Received message: " + request->name();
      std::cout << serverReceiveMessage << std::endl;

      std::string message = "Hello " + request->name() + "! This is Server! Welcome to the Chat!";
      response->set_from(request->name());
      response->set_message(message);

      return Status::OK;
    }

    Status Chat(ServerContext *context, grpc::ServerReaderWriter<ChatMessage, ChatMessage>* stream) override {
      std::cout << "\nServer is executing RPC Chat()...\n" << std::endl;
      ChatMessage request;
      while (stream->Read(&request)) {
        std::string clientName = request.from();
        std::string message = request.message();

        // Add client to the streams_
        streams_.emplace(clientName, stream);

        std::string serverReceived = "Server received message from `" + clientName + "` with message: " + message;
        std::cout << serverReceived << std::endl;

        // Notify all connected clients of the new message
        ChatMessage response;
        for(auto& pair : streams_) {
          if (pair.first != clientName) {
            ChatMessage response;
            response.set_from(clientName);
            response.set_message(message);
            pair.second->Write(response);
          }
        }

        // Write() returns true if it successfully writes, false otherwise
        // Notify current client their message was recevied from the server
        response.set_from("Server");
        response.set_message("Received your message successfully.");
        if (!stream->Write(response)) {
          break;
        }

      }
      return Status::OK;
    }

  private:
    std::map<std::string, std::unique_ptr<grpc::ServerReaderWriter<ChatMessage, ChatMessage>>> streams_;
};

void RunServer() {
  std::string server_address("0.0.0.0:50051");
  ChatServiceImpl service;
  ServerBuilder builder;

  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  server->Wait();
}

int main(int argc, char **argv) {
  RunServer();
  return 0;
}
