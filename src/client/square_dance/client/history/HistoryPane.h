#pragma once

#include "square_dance/client/api/Video.h"
#include "square_dance/client/history/PlayHistoryStore.h"

#include <QWidget>

class QListWidget;
class QPushButton;

namespace square_dance {
namespace client {

class HistoryPane : public QWidget {
  Q_OBJECT

 public:
  explicit HistoryPane(PlayHistoryStore* store, QWidget* parent = 0);

  void refresh();

 signals:
  void playRequested(const Video& video, qint64 position_ms);
  void cleared();

 private slots:
  void onClearClicked();
  void onItemActivated();

 private:
  static QString FormatTime(qint64 ms);
  void Rebuild();

  PlayHistoryStore* store_;
  QListWidget* list_;
  QPushButton* clear_btn_;
};

}  // namespace client
}  // namespace square_dance
