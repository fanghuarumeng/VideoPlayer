#pragma once

#include <QString>

namespace video_player {
namespace client {

struct ClientConfig {
  QString http_base_url;
  QString ws_url;
  QString download_dir;
  QString playlist_path;  // empty → caller chooses AppData default
};

ClientConfig MakeDefaultClientConfig();

bool LoadClientConfigFromString(const QString& json_text,
                                ClientConfig* out,
                                QString* error);

bool LoadClientConfigFromFile(const QString& path,
                              ClientConfig* out,
                              QString* error);

}  // namespace client
}  // namespace video_player
