#pragma once

#include "video_player/client/api/Video.h"

#include <QString>
#include <QVector>

namespace video_player {
namespace client {

struct PlayHistoryItem {
  PlayHistoryItem()
      : id(0), position_ms(0), duration_ms(0) {}

  qint64 id;
  QString title;
  QString play_url;
  QString thumbnail_url;
  qint64 position_ms;
  qint64 duration_ms;
  QString updated_at;
};

class PlayHistoryStore {
 public:
  explicit PlayHistoryStore(const QString& path, int max_items = 100);

  QString path() const;
  bool load(QString* error);
  bool save(QString* error) const;

  void recordProgress(const Video& video, qint64 position_ms, qint64 duration_ms);
  void clear();
  QVector<PlayHistoryItem> items() const;

  // Resume position: 0 if near end (<=3s remaining when duration known).
  static qint64 ResumePositionMs(const PlayHistoryItem& item);

 private:
  PlayHistoryStore(const PlayHistoryStore&);
  PlayHistoryStore& operator=(const PlayHistoryStore&);

  QString path_;
  int max_items_;
  QVector<PlayHistoryItem> items_;
};

}  // namespace client
}  // namespace video_player
