/*
 * ###########################################################################
 *  Copyright 2015-2016. Los Alamos National Security, LLC. This software was
 *  produced under U.S. Government contract ??? (LA-CC-15-056) for Los
 *  Alamos National Laboratory (LANL), which is operated by Los Alamos
 *  National Security, LLC for the U.S. Department of Energy. The
 *  U.S. Government has rights to use, reproduce, and distribute this
 *  software.  NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY,
 *  LLC MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY
 *  FOR THE USE OF THIS SOFTWARE.  If software is modified to produce
 *  derivative works, such modified software should be clearly marked,
 *  so as not to confuse it with the version available from LANL.
 *
 *  Additionally, redistribution and use in source and binary forms,
 *  with or without modification, are permitted provided that the
 *  following conditions are met:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *
 *    * Neither the name of Los Alamos National Security, LLC, Los
 *      Alamos National Laboratory, LANL, the U.S. Government, nor the
 *      names of its contributors may be used to endorse or promote
 *      products derived from this software without specific prior
 *      written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND
 *  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 *  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS NATIONAL SECURITY, LLC OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 *  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 *  OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 *  SUCH DAMAGE.
 * ###########################################################################
 *
 * Notes
 *
 * #####
 */

#ifndef __ARES_COMMUNICATION_H__
#define __ARES_COMMUNICATION_H__

#include <sys/time.h>
#include <unistd.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <strings.h>

#include "CVSemaphore.h"

namespace ares{

class Channel{
public:
  virtual ~Channel(){}

  virtual void send(char* buf, size_t size) = 0;

  virtual void receive(char* buf, size_t size) = 0;
};

class SocketChannel : public Channel{
public:
  SocketChannel(int fd)
  : fd_(fd){}

  void send(char* buf, size_t size) override{
    ::send(fd_, buf, size, 0);
  }

  void receive(char* buf, size_t size) override{
    ::recv(fd_, buf, size, 0);
  }

private:
  int fd_;
};

class FIFOChannel : public Channel{
public:
  FIFOChannel(int fd)
  : fd_(fd){}

  ~FIFOChannel(){
    ::close(fd_);
  }

  void send(char* buf, size_t size) override{
    ::write(fd_, buf, size);
  }

  void receive(char* buf, size_t size) override{
    ::read(fd_, buf, size);
  }

private:
  int fd_;
};

enum class MessageType : uint8_t{
  None,
  Raw,
  Barrier
};

class BarrierMessage{
public:
  static const MessageType type = MessageType::Barrier;
};

class MessageBuffer{
public:
  template<class M>
  MessageBuffer(const M& msg, bool owned)
  : type_(M::type), 
  size_(sizeof(M)), 
  owned_(owned){
    buf_ = (char*)malloc(sizeof(M));
    memcpy(buf_, &msg, sizeof(M));
  }

  MessageBuffer(char* buf, size_t size, bool owned)
  : type_(MessageType::Raw), 
    buf_(buf),
    size_(size),
    owned_(owned){}

  MessageBuffer(MessageType type, char* buf, size_t size, bool owned)
  : type_(type), 
    buf_(buf),
    size_(size),
    owned_(owned){}

  MessageBuffer(MessageType type, size_t size, bool owned)
  : type_(type), 
  size_(size), 
  owned_(owned){
    buf_ = (char*)malloc(size);
  }

  ~MessageBuffer(){
    if(owned_){
      free(buf_);
    }
  }

  template<class T>
  T* as(){
    return reinterpret_cast<T*>(buf_);
  }

  char* buffer(){
    return buf_;
  }

  uint32_t size() const{
    return size_;
  }
  
  MessageType type(){
    return type_;
  }

private:
  MessageType type_;
  char* buf_;
  uint32_t size_;
  bool owned_;
};

class MessageHandler{
public:
  virtual bool handleMessage(MessageBuffer* msg) = 0;
};

class MessageDispatcher{
public:
  MessageDispatcher(MessageHandler* handler,
                    Channel* sendChannel,
                    Channel* receiveChannel)
  : handler_(handler), 
  sendChannel_(sendChannel),
  receiveChannel_(receiveChannel){}

  ~MessageDispatcher(){
    delete sendChannel_;
    delete receiveChannel_;
  }

  void setSendThread(std::thread* t){
    sendThread_ = t;
  }

  void setReceiveThread(std::thread* t){
    receiveThread_ = t;
  }

  void runSend(){
    for(;;){
      sendSem_.acquire();
      sendMutex_.lock();
      MessageBuffer* msg = sendQueue_.front();
      sendQueue_.pop_front();
      sendMutex_.unlock();

      uint32_t size = msg->size();
      char sbuf[5];
      memcpy(sbuf, &size, 4);
      sbuf[4] = char(msg->type());
      sendChannel_->send(sbuf, 5);
      sendChannel_->send(msg->buffer(), size);

      delete msg;
    }
  }

  void runReceive(){
    for(;;){
      char sbuf[5];
      receiveChannel_->receive(sbuf, 5);
      uint32_t size = *((uint32_t*)sbuf);
      MessageType type = MessageType(sbuf[4]);

      MessageBuffer* msg = new MessageBuffer(type, size, false);
      receiveChannel_->receive(msg->buffer(), size);

      if(handler_->handleMessage(msg)){
        delete msg;
        continue;
      }

      receiveMutex_.lock();
      receiveQueue_.push_back(msg);
      receiveMutex_.unlock();
      receiveSem_.release();
    }
  }

  void send(MessageBuffer* msg){
    sendMutex_.lock();
    sendQueue_.push_back(msg);
    sendMutex_.unlock();
    sendSem_.release();
  }

  MessageBuffer* receive(){
    receiveSem_.acquire();
    receiveMutex_.lock();
    MessageBuffer* msg = receiveQueue_.front();
    receiveQueue_.pop_front();
    receiveMutex_.unlock();
    return msg;
  }

private:
  std::thread* sendThread_;
  std::thread* receiveThread_;
  Channel* sendChannel_;
  Channel* receiveChannel_;

  CVSemaphore sendSem_;
  std::mutex sendMutex_;
  std::deque<MessageBuffer*> sendQueue_;
  
  CVSemaphore receiveSem_;
  std::mutex receiveMutex_;
  std::deque<MessageBuffer*> receiveQueue_;

  MessageHandler* handler_;
};

class Communicator : public MessageHandler{
public:
  class Barrier{
  public:
    Barrier(int n)
    : sem_(n){}

    void release(){
      sem_.release();
    }

    void acquire(){
      sem_.acquire();
    }

  private:
    CVSemaphore sem_;
  };

  virtual ~Communicator(){
    for(MessageDispatcher* h : dispatchers_){
      delete h;
    }
  }

  void addDispatcher(MessageDispatcher* dispatcher){
    dispatchers_.push_back(dispatcher);

    auto sendThread = new std::thread(&MessageDispatcher::runSend, dispatcher);
    dispatcher->setSendThread(sendThread);

    auto receiveThread = new std::thread(&MessageDispatcher::runReceive, dispatcher);
    dispatcher->setReceiveThread(receiveThread);
  }

  void send(MessageBuffer* buf){
    dispatchers_[0]->send(buf);
  }

  MessageBuffer* receive(){
    return dispatchers_[0]->receive();
  }

  void createdConnection(){
    ++numConnections_;
  }

  virtual bool isListener() = 0; 

  void barrier(){
    assert(barrier_);

    BarrierMessage bm;
    auto msg = new MessageBuffer(bm, true);

    send(msg);

    barrier_->acquire();
  }

  void init(size_t groupSize){
    assert(!barrier_);
    barrier_ = new Barrier(2 - (int)groupSize);
  }

  bool handleMessage(MessageBuffer* msg) override{
    switch(msg->type()){
      case MessageType::Barrier:{
        auto bm = msg->as<BarrierMessage>();
        assert(barrier_);
        barrier_->release();
        return true;
      }
      default:
        return false;
    }
  }

private:
  using MessageDispatcherVec = std::vector<MessageDispatcher*>;

  MessageDispatcherVec dispatchers_;
  Barrier* barrier_ = nullptr;
  size_t numConnections_ = 0;
};

class SocketCommunicator : public Communicator{
public:
  SocketCommunicator(){}

  bool listen(int port){
    listenFD_ = socket(PF_INET, SOCK_STREAM, 0);
    
    if(listenFD_ < 0){
      return false;
    }

    int so_reuseaddr = 1;

    setsockopt(listenFD_,
               SOL_SOCKET,
               SO_REUSEADDR,
               &so_reuseaddr,
               sizeof(so_reuseaddr));
    
    sockaddr_in addr;
    
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    int status = ::bind(listenFD_, (const sockaddr*)&addr, sizeof(addr));
    if(status < 0){
      return false;
    }
    
    if(::listen(listenFD_, 500) < 0){
      return false;
    }
    
    port_ = port;

    auto t = new std::thread(&SocketCommunicator::accept_, this);

    return true;
  }

  bool connect(const std::string& host, int port){
    hostent* h;
    h = gethostbyname(host.c_str());
    if(!h){
      return false;
    }
    
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    
    if(fd < 0){
      return false;
    }
    
    int reuseOn = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuseOn, sizeof(reuseOn));
    
    sockaddr_in addr;
    bzero((char*)&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    bcopy((char*)h->h_addr,
          (char*)&addr.sin_addr.s_addr,
          h->h_length);
    addr.sin_port = htons(port);
    
    if(::connect(fd, (sockaddr*)&addr, sizeof(addr)) < 0){
      return false;
    }
    
    auto channel = new SocketChannel(fd);
    auto dispatcher = new MessageDispatcher(this, channel, channel);

    addDispatcher(dispatcher);

    return true;
  }

  void accept_(){
    for(;;){
      int opts = fcntl(listenFD_, F_GETFL);
      opts &= ~O_NONBLOCK;
      
      fcntl(listenFD_, F_SETFL, opts);
      
      sockaddr addr;
      socklen_t len = sizeof(sockaddr);
            
      int fd = ::accept(listenFD_, &addr, &len);
      
      if(fd < 0){
        continue;
      }
      
      int reuseOn = 1;
      setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, 
                 &reuseOn, sizeof(reuseOn));

      auto channel = new SocketChannel(fd);
      auto dispatcher = new MessageDispatcher(this, channel, channel);

      addDispatcher(dispatcher);

      createdConnection();
    }
  }

  bool isListener() override{
    return listenFD_ > 0;
  }

private:
  int port_ = -1;
  int listenFD_ = -1;
};

class FIFOCommunicator : public Communicator{
public:
  FIFOCommunicator(){}

  bool listen(const std::string& sendPath, const std::string& receivePath){
    int status = mkfifo(sendPath.c_str(), S_IWUSR|S_IRUSR);
    assert(status == 0);

    status = mkfifo(receivePath.c_str(), S_IWUSR|S_IRUSR);
    assert(status == 0);

    int sendFD = open(sendPath.c_str(), O_WRONLY);
    assert(sendFD > 0);

    int receiveFD = open(receivePath.c_str(), O_RDONLY);
    assert(receiveFD > 0);

    auto sendChannel = new FIFOChannel(sendFD);
    auto receiveChannel = new FIFOChannel(receiveFD);
    auto dispatcher = new MessageDispatcher(this, sendChannel, receiveChannel);

    addDispatcher(dispatcher);

    isListener_ = true;

    createdConnection();

    return true;
  }

  bool connect(const std::string& sendPath, const std::string& receivePath){
    int receiveFD = open(receivePath.c_str(), O_RDONLY);
    assert(receiveFD > 0);

    int sendFD = open(sendPath.c_str(), O_WRONLY);
    assert(sendFD > 0);

    auto sendChannel = new FIFOChannel(sendFD);
    auto receiveChannel = new FIFOChannel(receiveFD);
    auto dispatcher = new MessageDispatcher(this, sendChannel, receiveChannel);

    addDispatcher(dispatcher);

    isListener_ = false;

    return true;
  }

  bool isListener() override{
    return isListener_;
  }

private:
  bool isListener_ = false;
};

} // namespace ares

#endif // __ARES_COMMUNICATION_H__
