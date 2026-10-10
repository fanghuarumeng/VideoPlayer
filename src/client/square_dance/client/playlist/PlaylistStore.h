#pragma once

#include "square_dance/client/api/Video.h"

#include <QString>
#include <QVector>

namespace square_dance {
namespace client {

struct PlaylistItem {
  PlaylistItem() : id(0) {}

  qint64 id;
  QString title;
  QString play_url;
  QString thumbnail_url;
};

class PlaylistStore {
 public:
  explicit PlaylistStore(const QString& path);

  QString path() const;

  bool load(QString* error);
  bool save(QString* error) const;

  // Returns false if duplicate id (already present); still considered success
  // for callers that only care that the id is in the list — use appendedOut.
  bool append(const PlaylistItem& item);
  bool append(const Video& video);
  void clear();

  QVector<PlaylistItem> items() const;
  int currentIndex() const;
  void setCurrentIndex(int index);

 private:
  PlaylistStore(const PlaylistStore&);
  PlaylistStore& operator=(const PlaylistStore&);

  bool ContainsId(qint64 id) const;
  void ClampIndex();

  QString path_;
  QVector<PlaylistItem> items_;
  int current_index_;
};

}  // namespace client
}  // namespace square_dance
