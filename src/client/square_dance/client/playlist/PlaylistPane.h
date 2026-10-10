#pragma once

#include "square_dance/client/playlist/PlaylistStore.h"

#include <QModelIndex>
#include <QWidget>

class QListView;
class QPushButton;

namespace square_dance {
namespace client {

class PlaylistModel;

class PlaylistPane : public QWidget {
  Q_OBJECT

 public:
  explicit PlaylistPane(PlaylistStore* store, QWidget* parent = 0);

  PlaylistModel* model() const;
  bool selectedItem(PlaylistItem* out) const;
  void selectRow(int row);
  void refresh();

 signals:
  void playRequested(const PlaylistItem& item);
  void cleared();

 private slots:
  void onClearClicked();
  void onDoubleClicked(const QModelIndex& index);

 private:
  PlaylistStore* store_;
  PlaylistModel* model_;
  QListView* list_;
  QPushButton* clear_btn_;
};

}  // namespace client
}  // namespace square_dance
