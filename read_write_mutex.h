#ifndef _PLAYER_GUARD
#define _PLAYER_GUARD

#include <mutex>
#include <atomic>
#include <condition_variable>

class read_write_mutex {
  public:
    read_write_mutex() : readers(0) {}
    void lock_reader() {
      std::lock_guard<std::mutex> lck(mtx);
      readers++;
    }

    void unlock_reader() {
      readers--;
      cv.notify_all();
    }

    void lock_writer() {
      mtx.lock();
      std::unique_lock<std::mutex> cv_lck(cv_mtx);
      while (readers > 0) cv.wait(cv_lck);
    }

    void unlock_writer() {
      mtx.unlock();
    }

  private:
    std::atomic<int> readers;
    std::mutex mtx;
    std::mutex cv_mtx;
    std::condition_variable cv;
};

#endif
