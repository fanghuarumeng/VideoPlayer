#pragma once

#include <cstdint>
#include <string>

namespace square_dance {
namespace server {

struct Video {
  std::int64_t id;
  std::string external_id;
  std::string title;
  std::string play_url;
  std::string download_url;
  std::string source_type;
  int consecutive_failures;
  std::string created_at;
  std::string updated_at;
};

}  // namespace server
}  // namespace square_dance
