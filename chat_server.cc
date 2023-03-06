#include "cmake/build/chatStreamingService.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <memory>
#include <string>


using chatStreamingService::MessageRequest;
using chatStreamingService::MessageResponse;
using chatStreamingService::ChatService;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

class ChatServiceImpl final : public ChatService::Service {
    Status Chat(ServerContext *context,
                grpc::ServerReaderWriter<MessageResponse, MessageRequest> *stream) {
      MessageRequest request;
      while (stream->Read(&request)) {
        std::string message = "Server Received message: " + request.name();
        MessageResponse response;
        response.set_message(message);
        stream->Write(response);
      }
      return Status::OK;
    }
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
