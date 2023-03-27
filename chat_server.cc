// #include "LinkedList.h"
#include "cmake/build/chatStreamingService.grpc.pb.h"
#include <google/protobuf/empty.pb.h>
#include <grpcpp/grpcpp.h>

#include <iostream>
#include <memory>
#include <queue>
#include <string>

using chatStreamingService::ChatService;
using chatStreamingService::Message;
using chatStreamingService::Time;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

class ChatServiceImpl final : public ChatService::Service {
public:
  ChatServiceImpl() {}

  Status SendMessage(ServerContext *context,
                     grpc::ServerReader<Message> *stream,
                     google::protobuf::Empty *response) override {
    Message request;

    while (stream->Read(&request)) {
      std::string msg = request.username() + ": " + request.message();

      // Get current time in milliseconds
      auto now = std::chrono::system_clock::now();
      auto now_ms =
          std::chrono::time_point_cast<std::chrono::milliseconds>(now);
      auto value = now_ms.time_since_epoch().count();
      long ms_since_epoch = static_cast<long>(value);

      // Save message in datastructure
      std::string usrnm = request.username();
      std::string usrMsg = request.message();

      // Create a new tuple to add to the vector
      std::tuple<std::string, std::string> newTuple =
          std::make_tuple(usrnm, usrMsg);

      g_mutex.lock();

      // Get the vector associated with the key `ms_since_epoch`
      // Note: The key gets created here
      std::vector<std::tuple<std::string, std::string>> &messageVector =
          userMessages[ms_since_epoch];
      // Add the new tuple to the vector
      messageVector.push_back(newTuple);

      std::cout << "\nSaved a New message in data structure with the time "
                << ms_since_epoch << std::endl;

      // DEBUGGING
      std::cout << "This is what we saved in the data structure" << std::endl;
      printVectorTuples(userMessages[ms_since_epoch]);
      g_mutex.unlock();
    }

    return Status::OK;
  }

  // Client checks if there is a new message available
  Status IsNewMessageAvailable(ServerContext *context,
                               const Time *clientLatestMsgTime,
                               Time *response) override {

    // DEBUGGING
    std::cout << "\nA Client is checking if a new message is available after "
              << clientLatestMsgTime->utc() << std::endl;

    // Compare clientTime.utc() to latest message utc
    // Note: rbegin() is reverse begin which points to the last key/value pair
    // in the map .end() would point to 'one past' the last element
    g_mutex.lock();
    if (!userMessages.empty() &&
        clientLatestMsgTime->utc() < userMessages.rbegin()->first) {

      response->set_utc(clientLatestMsgTime->utc());
      response->set_isnew(true);

      // DEBUGGING
      // std::cout << "A new message is available for the client" << std::endl;
    }
    g_mutex.unlock();
    return Status::OK;
  }

  // Client subscribes to server to receive messages
  Status ReceiveMessage(ServerContext *context,
                        // without const, this RPC method doesnt get called
                        // and we can't override this method definition
                        const Time *request,
                        grpc::ServerWriter<Message> *stream) override {

    std::cout << "A Client is pulling new messages "
                 "from the Server...\n"
              << std::endl;

    // Client sends Time request containing the client's last message's utc
    long clientLatestMsgUtc = request->utc();

    // Iterate backwards on messages hash table until client's utc is greater
    // than hash table key utc

    g_mutex.lock();
    // get all newest messages as a vector and return each as a stream of
    // Message
    std::vector<std::tuple<std::string, std::string>> allMessages =
        getMessagesSinceTime(userMessages, clientLatestMsgUtc);

    // Traverse all user messages using range-based for loop
    for (const auto &msg : allMessages) {
      // Server response container
      Message response;

      // Pulling response from tuple
      response.set_username(std::get<0>(msg));
      response.set_message(std::get<1>(msg));

      std::cout << "Server writing response to client.." << std::endl;

      // Write response to client
      if (!stream->Write(response)) {
        std::cout << "\n\t\t***Error Sending Message to Client***\n"
                  << std::endl;
        return Status::CANCELLED;
      }
    }
    g_mutex.unlock();

    return Status::OK;
  }

  // Traverse ordered hash map and return all messages (tuple of <username,
  // message>) as a vector
  std::vector<std::tuple<std::string, std::string>> getMessagesSinceTime(
      const std::map<long, std::vector<std::tuple<std::string, std::string>>>
          &messages,
      long time) {

    // DEBUGGING
    std::cout << "Server is aggregating new messages for the client..."
              << std::endl;

    std::vector<std::tuple<std::string, std::string>> result;

    // Start traversal from a message greater than client's last message time
    for (auto it = messages.lower_bound(time); it != messages.end(); ++it) {
      result.insert(result.end(), it->second.begin(), it->second.end());
    }

    return result;
  }

  // For DEBUGGING
  void printVectorTuples(
      std::vector<std::tuple<std::string, std::string>> myVector) {

    if (myVector.size() > 0) {
      // Iterate over the vector and print the tuples
      for (const auto &tuple : myVector) {
        std::cout << std::get<0>(tuple) << ": " << std::get<1>(tuple)
                  << std::endl;
      }
    } else {
      std::cout << "The vector is empty" << std::endl;
    }
  }

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
  std::mutex g_mutex;

  // Use a hash table to keep track of time & user's messages
  // [ {username, msg}, {usrnm, msg}, ...]
  std::map<long, std::vector<std::tuple<std::string, std::string>>>
      userMessages;
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
