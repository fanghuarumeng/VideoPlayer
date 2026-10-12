#pragma once

#include <QMetaType>
#include <QString>
#include <QVector>

namespace video_player {
namespace client {

// Qt-facing catalog row; converted from shared VideoDto at the boundary.
struct Video {
  Video() : id(0) {}

  qint64 id;
  QString external_id;
  QString title;
  QString play_url;
  QString download_url;
  QString source_type;
  QString updated_at;
  QString thumbnail_url;
};

}  // namespace client
}  // namespace video_player

Q_DECLARE_METATYPE(video_player::client::Video)
Q_DECLARE_METATYPE(QVector<video_player::client::Video>)
