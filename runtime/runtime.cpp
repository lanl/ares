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

#include <iostream>
#include <cmath>
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <functional>
#include <cassert>
#include <deque>
#include <queue>

#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
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

using namespace std;

#define np(X) cout << __FILE__ << ":" << __LINE__ << ": " << \
__PRETTY_FUNCTION__ << ": " << #X << " = " << (X) << std::endl

namespace{

  const size_t NUM_THREADS = 32;

  class VSem{
  public:
    
    VSem(int count=0)
    : count_(count),
    maxCount_(0){

      pthread_mutex_init(&mutex_, 0);
      pthread_cond_init(&condition_, 0);
    }
    
    VSem(int count, int maxCount)
    : count_(count),
    maxCount_(maxCount){

      pthread_mutex_init(&mutex_, 0);
      pthread_cond_init(&condition_, 0);
    }
    
    ~VSem(){
      pthread_cond_destroy(&condition_);
      pthread_mutex_destroy(&mutex_);
    }

    bool acquire(double dt){
      timeval tv;
      gettimeofday(&tv, 0);
      
      double t = tv.tv_sec + tv.tv_usec/1e6;
      t += dt;
      
      pthread_mutex_lock(&mutex_);
            
      double sec = floor(t);
      double fsec = t - sec;

      timespec ts;
      ts.tv_sec = sec;
      ts.tv_nsec = fsec*1e9;

      while(count_ <= 0){
        if(pthread_cond_timedwait(&condition_, 
                                  &mutex_,
                                  &ts) != 0){
          pthread_mutex_unlock(&mutex_);
          return false;
        }
      }
      
      --count_;
      pthread_mutex_unlock(&mutex_);
      
      return true;
    }
    
    bool acquire(){
      pthread_mutex_lock(&mutex_);
      
      while(count_ <= 0){
        pthread_cond_wait(&condition_, &mutex_);
      }
      
      --count_;
      pthread_mutex_unlock(&mutex_);
      
      return true;
    }
    
    bool tryAcquire(){
      pthread_mutex_lock(&mutex_);
      
      if(count_ > 0){
        --count_;
        pthread_mutex_unlock(&mutex_);
        return true;
      }

      pthread_mutex_unlock(&mutex_);
      return false;
    }
    
    void release(){
      pthread_mutex_lock(&mutex_);

      if(maxCount_ == 0 || count_ < maxCount_){
        ++count_;
      }

      pthread_cond_signal(&condition_);
      pthread_mutex_unlock(&mutex_);
    }
    
    VSem& operator=(const VSem&) = delete;
    
    VSem(const VSem&) = delete;

  private:
    pthread_mutex_t mutex_;
    pthread_cond_t condition_;

    int count_;
    int maxCount_;
  };

  class Synch{
  public:
    Synch(int count)
    : sem_(-count){}

    void release(){
      sem_.release();
    }

    void await(){
      sem_.acquire();
    }

  private:
    VSem sem_;
  };

  struct FuncArg{
    FuncArg(Synch* synch, int n)
      : n(n),
      synch(synch){}

    Synch* synch;
    int n;
  };

  struct TaskArg{
    TaskArg(Synch* futureSync)
      : futureSync(futureSync){}

    Synch* futureSync;
    uint32_t depth;
  };

  using Func = function<void(void*)>;
  using FuncPtr = void (*)(void*);

  class ThreadPool{
  public:
    class Queue{
    public:

      class Item{
      public:
        Item(Func func, void* arg, uint32_t priority)
        : func(func),
        arg(arg),
        priority(priority){}

        Func func;
        void* arg;
        uint32_t priority;
      };

      void push(Func func, void* arg, uint32_t priority){
        mutex_.lock();
        queue_.push(new Item(func, arg, priority));
        mutex_.unlock();
        
        sem_.release();
      }
      
      Item* get(){
        if(!sem_.acquire()){
          return nullptr;
        }

        mutex_.lock();
        Item* item = queue_.top();
        queue_.pop();
        mutex_.unlock();
        return item;
      }

    private:
      struct Compare_{
        bool operator()(const Item* i1, const Item* i2) const{
          return i1->priority < i2->priority;
        }
      };
      
      typedef priority_queue<Item*, vector<Item*>, Compare_> Queue_;

      Queue_ queue_;
      VSem sem_;
      mutex mutex_;
    };

    ThreadPool(){
      start();
    }

    void push(Func func, void* arg, uint32_t priority){
      queue_.push(func, arg, priority);
    }

    void start(){
      for(size_t i = 0; i < NUM_THREADS; ++i){
        threadVec_.push_back(new thread(&ThreadPool::run_, this));  
      }
    }

    void run_(){
      for(;;){
        Queue::Item* item = queue_.get();
        assert(item);
        item->func(item->arg);
        delete item;
      }
    }

  private:
    using ThreadVec = std::vector<thread*>;

    Queue queue_;

    mutex mutex_;

    ThreadVec threadVec_;
  };

  ThreadPool* _threadPool = new ThreadPool;

  mutex _logMutex;

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

  };

  class MessageBuffer{
  public:
    MessageBuffer(MessageType type, uint32_t size, bool owned)
    : type_(type), 
    size_(size), 
    owned_(owned){
      buf_ = (char*)malloc(size);
    }

    MessageBuffer(MessageType type, char* buf, uint32_t size, bool owned)
    : type_(type), 
      buf_(buf),
      size_(size),
      owned_(owned){}

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
    
  private:
    MessageType type_;
    char* buf_;
    uint32_t size_;
    bool owned_;
  };

  class MessageHandler{
  public:
    MessageHandler(Channel* sendChannel, Channel* receiveChannel)
    : sendChannel_(sendChannel),
    receiveChannel_(receiveChannel){}

    ~MessageHandler(){
      delete sendChannel_;
      delete receiveChannel_;
    }

    void setSendThread(thread* t){
      sendThread_ = t;
    }

    void setReceiveThread(thread* t){
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
        sendChannel_->send((char*)&size, 4);
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
    thread* sendThread_;
    thread* receiveThread_;
    Channel* sendChannel_;
    Channel* receiveChannel_;

    VSem sendSem_;
    mutex sendMutex_;
    deque<MessageBuffer*> sendQueue_;
    
    VSem receiveSem_;
    mutex receiveMutex_;
    deque<MessageBuffer*> receiveQueue_;
  };

  class Communicator{
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
      VSem sem_;
    };

    virtual ~Communicator(){
      for(MessageHandler* h : handlers_){
        delete h;
      }
    }

    void addMessageHandler(MessageHandler* handler){
      handlers_.push_back(handler);

      auto sendThread = new thread(&MessageHandler::runSend, handler);
      handler->setSendThread(sendThread);

      auto receiveThread = new thread(&MessageHandler::runReceive, handler);
      handler->setReceiveThread(receiveThread);
    }

    void send(MessageBuffer* buf){
      handlers_[0]->send(buf);
    }

    MessageBuffer* receive(){
      return handlers_[0]->receive();
    }

    void createdConnection(){
      ++numConnections_;
    }

    virtual bool isListener() = 0; 

    void barrier(){
      if(!barrier_){
        barrier_ = new Barrier(numConnections_);
      }

      barrier_->acquire();
    }

    void init(size_t groupSize){
      assert(!barrier_);
      barrier_ = new Barrier(2 - (int)groupSize);
    }

  private:
    using MessageHandlerVec = std::vector<MessageHandler*>;

    MessageHandlerVec handlers_;
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

      auto t = new thread(&SocketCommunicator::accept_, this);

      return true;
    }

    bool connect(const string& host, int port){
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
      auto handler = new MessageHandler(channel, channel);

      addMessageHandler(handler);

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
        auto handler = new MessageHandler(channel, channel);

        addMessageHandler(handler);

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

    bool listen(const string& sendPath, const string& receivePath){
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
      auto handler = new MessageHandler(sendChannel, receiveChannel);

      addMessageHandler(handler);

      isListener_ = true;

      createdConnection();

      return true;
    }

    bool connect(const string& sendPath, const string& receivePath){
      int receiveFD = open(receivePath.c_str(), O_RDONLY);
      assert(receiveFD > 0);

      int sendFD = open(sendPath.c_str(), O_WRONLY);
      assert(sendFD > 0);

      auto sendChannel = new FIFOChannel(sendFD);
      auto receiveChannel = new FIFOChannel(receiveFD);
      auto handler = new MessageHandler(sendChannel, receiveChannel);

      addMessageHandler(handler);

      isListener_ = false;

      return true;
    }

    bool isListener() override{
      return isListener_;
    }

  private:
    bool isListener_ = false;
  };

  Communicator* _communicator = nullptr;

} // namespace

extern "C"{

  void* __ares_create_synch(uint32_t count){
    return new Synch(count - 1);
  }

  void __ares_queue_func(void* synch, void* fp, uint32_t index, uint32_t priority){
    _threadPool->push(reinterpret_cast<FuncPtr>(fp),
      new FuncArg(reinterpret_cast<Synch*>(synch), index), priority);
  }

  void __ares_finish_func(void* arg){
    auto a = reinterpret_cast<FuncArg*>(arg);
    a->synch->release();
    delete a;
  }

  void __ares_await_synch(void* synch){
    auto s = reinterpret_cast<Synch*>(synch);
    s->await();
    delete s;
  }

  void* __ares_alloc(uint64_t bytes){
    return malloc(bytes);
  }

  void __ares_task_queue(void* funcPtr, void* argsPtr){
    auto func = reinterpret_cast<FuncPtr>(funcPtr);
    auto args = reinterpret_cast<TaskArg*>(argsPtr);
    args->futureSync = new Synch(0);
    _threadPool->push(func, args, 0);
  }

  void __ares_task_await_future(void* argsPtr){
    auto args = reinterpret_cast<TaskArg*>(argsPtr);
    args->futureSync->await();
  }

  void __ares_task_release_future(void* argsPtr){
    auto args = reinterpret_cast<TaskArg*>(argsPtr);
    args->futureSync->release();
  }

} // extern "C"

namespace ares{

  bool ares_listen(int port){
    assert(!_communicator);
    auto c = new SocketCommunicator;
    _communicator = c;
    return c->listen(port);
  }

  bool ares_listen(const std::string& sendPath, const std::string& receivePath){
    assert(!_communicator);
    auto c = new FIFOCommunicator;
    _communicator = c;
    return c->listen(sendPath, receivePath);
  }

  bool ares_connect(const char* host, int port){
    assert(!_communicator);
    auto c = new SocketCommunicator;
    _communicator = c;
    return c->connect(host, port);  
  }

  bool ares_connect(const std::string& sendPath, const std::string& receivePath){
    assert(!_communicator);
    auto c = new FIFOCommunicator;
    _communicator = c;
    return c->connect(sendPath, receivePath);  
  }

  void ares_send(char* buf, size_t size){
    auto msg = new MessageBuffer(MessageType::Raw, buf, size, true);
    _communicator->send(msg);
  }

  char* ares_receive(size_t& size){
    MessageBuffer* msg = _communicator->receive();
    size = msg->size();
    char* buf = msg->buffer();
    return buf;
  }

  void init_comm(size_t groupSize){
    assert(_communicator);
    _communicator->init(groupSize);
  }

  void ares_barrier(){
    assert(_communicator);
    _communicator->barrier();
  }

} // namespace ares
