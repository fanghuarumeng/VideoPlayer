#pragma once

#include <mutex>

namespace video_player {
namespace server {

class UploadGate {
 public:
  explicit UploadGate(int max_slots);

  bool tryAcquire();
  void release();

  int max_slots() const;
  int in_use() const;

 private:
  UploadGate(const UploadGate&);
  UploadGate& operator=(const UploadGate&);

  mutable std::mutex mutex_;
  int max_slots_;
  int in_use_;
};

}  // namespace server
}  // namespace video_player
