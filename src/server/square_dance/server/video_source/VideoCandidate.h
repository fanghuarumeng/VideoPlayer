#pragma once

#include <string>

namespace square_dance {
namespace server {

struct VideoCandidate {
  std::string external_id;
  std::string title;
  std::string play_url;
  std::string download_url;
  std::string source_type;
};

}  // namespace server
}  // namespace square_dance
