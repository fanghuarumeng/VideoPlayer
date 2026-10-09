#pragma once

#include <cstdint>
#include <string>

namespace square_dance {

// Wire-format DTO shared by server and client. No Qt types.
struct VideoDto {
  std::int64_t id;
  std::string external_id;
  std::string title;
  std::string play_url;
  std::string download_url;
  std::string source_type;  // "archive_org" | "authorized"
  std::string updated_at;   // ISO-8601 UTC
};

}  // namespace square_dance
