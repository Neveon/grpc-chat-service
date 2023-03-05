#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <grpcpp/grpcpp.h>
#include "chatStreamingService.grpc.pb.h"

using chatSreamingService::MessageResponse;
using chatStreamingService::MessageRequest;
using chatStreamingService::MyService;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

class MyServiceClient
{
public:
  MyServiceClient(std::shared_ptr<Channel> channel) : stub_(MyService::NewStub(channel)) {}

  void Chat(const std::string &name)
  {
    std::unique_ptr<grpc::ClientWriter<MessageRequest>> writer(stub_->Chat(&context));

    std::thread writer_thread([&writer, &name]()
                              {
      MessageRequest request;
      request.set_name(name);
      writer->Write(request); });

    MessageResponse response;
    while (reader_->Read(&response))
    {
      std::cout << "Got message: " << response.message() << std::endl;
    }

    writer->WritesDone();
    writer_thread.join();

    Status status = reader_->Finish();
    if (status.ok())
    {
      std::cout << "Chat ended" << std::endl;
    }
    else
    {
      std::cout << "Chat failed: " << status.error_message() << std::endl;
    }
  }

private:
  std::unique_ptr<MyService::Stub> stub_;
  grpc::ClientContext context;
  std::unique_ptr < grpc::ClientReader < Message
