#include "LinkedList.h"
#include "cmake/build/chatStreamingService.grpc.pb.h"
#include <google/protobuf/empty.pb.h>
#include <grpcpp/grpcpp.h>

#include <iostream>
#include <memory>
#include <queue>
#include <string>

using chatStreamingService::ChatService;
using chatStreamingService::Message;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

class ChatServiceImpl final : public ChatService::Service {
public:
  ChatServiceImpl()
      : chat_messages(new LinkedList()), dummy(Node("Server", "Welcome to the Server")) {
    // Initialize the queue of messages
    // chat_messages = std::shared_ptr<LinkedList>(new LinkedList());

    // Initialize Dummy Node to help track latest messages
    // dummy = *(new Node("dummy", "dummy"));
  }

  Status SendMessage(ServerContext *context,
                     grpc::ServerReader<Message> *stream,
                     google::protobuf::Empty *response) override {
    Message request;

    while (stream->Read(&request)) {
      std::cout << "\n\t\t----------Before----------" << std::endl;
      traverseAndPrint(chat_messages);

      std::string msg = request.username() + ": " + request.message();
      // std::cout << msg << std::endl;

      // Save message in datastructure
      addToLL(request.username(), request.message());

      std::cout << "\n\t\t----------After----------" << std::endl;
      traverseAndPrint(chat_messages);
    }

    return Status::OK;
  }

  // Client subscribes to server to receive messages
  Status ReceiveMessage(ServerContext *context,
                        // without const, this RPC method doesnt get called and
                        // we can't override this method definition
                        const google::protobuf::Empty *request,
                        grpc::ServerWriter<Message> *stream) override {

    std::cout << "\nA Client has subscribed to the Server...\n" << std::endl;

    // Server First message - (username, message)
    // addToLL("Server", "Welcome to the Server!");

    while (true) {
      g_mutex.lock();
      // At init, there are no latest messages sent to the client
      if (dummy.next == nullptr) {
        std::cout << "\n[DEBUGGING] Init dummy.msg = " << dummy.message
                  << std::endl;
        std::cout << "\n[DEBUGGING] Init, dummy.next = nullptr " << std::endl;

        dummy.next = chat_messages->head;
        latestMessage = &dummy;

        // Set current node to the LL's head, a node ahead latestMessage
        cur = &latestMessage->next;
      }

      // DEBUGGING
      // if (cur == nullptr) {
      //   std::cout << "\n[DEBUGGING] cur->next = nullptr" << std::endl;
      // } else {
      //   std::cout << "\n[DEBUGGING] cur->next = " << cur->message << std::endl;
      // }

      // DEBUGGING
      if (latestMessage != nullptr) {
        std::cout << "\n[DEBUGGING] latestMessage = " << latestMessage->message
                  << std::endl;
      }

      // DEBUGGING
      if (latestMessage->next != nullptr) {
        std::cout << "\n[DEBUGGING] latestMessage.next (cur) = " << latestMessage->next->message
                  << std::endl;
      }

      // Write all messages in data structure to the client
      while (latestMessage->next != nullptr) {

        // DEBUGGING
        std::cout << "\n[DEBUGGING] Response message being sent to Client... "
                  << std::endl;

        // Repsonse container
        Message response;

        // Update previous message written as the latest message client received
        latestMessage = *cur;
        std::cout << "\n[DEBUGGING] latestMessage = " << latestMessage->message
                  << std::endl;

        response.set_username(latestMessage->username);
        response.set_message(latestMessage->message);
        if (!stream->Write(response)) {
          std::cout << "\n\t\t***Error Sending Message to Client***\n" << std::endl;
          return Status::CANCELLED; 
        }

        // std::cout << "\n[DEBUGGING] cur = " << cur.message
        //           << std::endl;

        // Update cur pointer to next node in LL
        cur = &(latestMessage->next);
      }
      g_mutex.unlock();
      sleep(4);
    }

    return Status::OK;
  }

  // Function to add a new string to the queue.
  void addToLL(const std::string &username, const std::string &msg) {
    g_mutex.lock();
    // traverseAndPrint(chat_messages);
    chat_messages->add(username, msg);
    // traverseAndPrint(chat_messages);
    g_mutex.unlock();
  }

  // Traverse and build responses to send to client
  void traverseAndPrint(std::shared_ptr<LinkedList> ll) {
    g_mutex.lock();
    Node *current = ll->head;
    std::cout << "\n\t\tSERVER DEBUG LINKED LIST MESSAGES\n\t\t" << std::endl;
    while (current != nullptr) {
      std::cout << current->message << std::endl;
      current = current->next;
    }
    g_mutex.unlock();
    std::cout << "\n\t\t*********************************\n\t\t" << std::endl;
  }

private:
  std::shared_ptr<LinkedList> chat_messages;
  std::mutex g_mutex;

  // Keep track of the latest chat message in LL
  Node dummy;
  Node *latestMessage;
  Node **cur;
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
