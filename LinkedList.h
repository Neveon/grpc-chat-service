#include <iostream>
#include <string>

class Node {
public:
  std::string username;
  std::string message;
  Node *next;

  Node(const std::string &username, const std::string &message)
      : username(username), message(message), next(nullptr) {}
};

class LinkedList {
public:
  Node *head;
  LinkedList() : head(nullptr), tail(nullptr), length(0) {}

  void add(const std::string &username, const std::string &msg) {
    Node *node = new Node(username, msg);
    if (head == nullptr) {
      head = node;
    } else {
      tail->next = node;
    }
    tail = node;
    length++;

    if (length > 10) {
      removeHead();
    }
  }

private:
  Node *tail;
  int length;

  void removeHead() {
    Node *temp = head;
    head = head->next;
    delete temp;
    length--;
  }
};