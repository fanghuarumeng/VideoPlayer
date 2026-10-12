#include "video_player/server/local_media/UploadGate.h"

namespace video_player {
namespace server {

UploadGate::UploadGate(int max_slots)
    : max_slots_(max_slots > 0 ? max_slots : 1), in_use_(0) {}

bool UploadGate::tryAcquire() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (in_use_ >= max_slots_) {
    return false;
  }
  ++in_use_;
  return true;
}

void UploadGate::release() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (in_use_ > 0) {
    --in_use_;
  }
}

int UploadGate::max_slots() const {
  return max_slots_;
}

int UploadGate::in_use() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return in_use_;
}

}  // namespace server
}  // namespace video_player
