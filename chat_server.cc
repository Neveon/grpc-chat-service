#include <iostream>
#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>
#include "chatStreamingService.grpc.pb.h"

using chatSreamingService::MessageResponse;
using chatStreamingService::MessageRequest;
using chatStreamingService::MyService;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

class MyServiceImpl final : public chatStreamingService::Service
{
public:
  MyServiceImpl() {}

  Status Chat(ServerContext *context, grpc::ServerReaderWriter<MessageResponse, MessageRequest> *stream) override
  {
    MessageRequest request;
    while (stream->Read(&request))
    {
      std::string message = "Received message: " + request.name();
      MessageResponse response;
      response.set_message(message);
      stream->Write(response);
    }
    return Status::OK;
  }
};

void RunServer()
{
  std::string server_address("0.0.0.0:50051");
  MyServiceImpl service;

  grpc::ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  server->Wait();
}

int main(int argc, char **argv)
{
  RunServer();
  return 0;
}
