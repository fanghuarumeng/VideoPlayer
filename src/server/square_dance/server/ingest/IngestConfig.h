#pragma once

#include <string>
#include <vector>

namespace square_dance {
namespace server {

struct IngestConfig {
  int interval_ms;
  std::vector<std::string> keywords;
  int per_keyword_limit;

  static IngestConfig MakeDefault() {
    IngestConfig cfg;
    cfg.interval_ms = 12 * 60 * 60 * 1000;
    cfg.keywords.push_back("广场舞");
    cfg.keywords.push_back("square dance");
    cfg.per_keyword_limit = 50;
    return cfg;
  }
};

}  // namespace server
}  // namespace square_dance
