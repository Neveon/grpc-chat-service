#include "cmake/build/chatStreamingService.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <memory>
#include <string>


using chatStreamingService::LoginRequest;
using chatStreamingService::MessageRequest;
using chatStreamingService::MessageResponse;
using chatStreamingService::ChatService;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

class ChatServiceImpl final : public ChatService::Service {
  public:
    ChatServiceImpl() {
      // Initialize the streams map
      streams = std::make_shared<std::map<std::string, std::shared_ptr<grpc::ServerWriter<MessageRequest>>>>();
    }

    Status JoinChat(ServerContext * context, const LoginRequest * request, MessageResponse * response) override {
      std::string serverReceiveMessage = "Server Received message: " + request->name();
      std::cout << serverReceiveMessage << std::endl;

      std::string message = "Hello " + request->name() + "! This is Server! Welcome to the Chat!";
      response->set_message(message);
      return Status::OK;
    }

    // Status Chat(ServerContext *context,
    //             grpc::ServerReaderWriter<MessageResponse, MessageRequest> *stream) override {
    //   MessageRequest request;
    //   while (stream->Read(&request)) {
    //     std::string message = "Server Received message: " + request.name();
    //     MessageResponse response;
    //     response.set_message(message);
    //     stream->Write(response);
    //   }
    //   return Status::OK;
    // }

  private:
    std::shared_ptr<std::map<std::string, std::shared_ptr<grpc::ServerWriter<MessageRequest>>>> streams;
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
