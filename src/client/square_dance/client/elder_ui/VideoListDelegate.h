#pragma once

#include <QStyledItemDelegate>

namespace square_dance {
namespace client {

class VideoListDelegate : public QStyledItemDelegate {
  Q_OBJECT

 public:
  explicit VideoListDelegate(QObject* parent = 0);

  void paint(QPainter* painter, const QStyleOptionViewItem& option,
             const QModelIndex& index) const;
  QSize sizeHint(const QStyleOptionViewItem& option,
                 const QModelIndex& index) const;
};

}  // namespace client
}  // namespace square_dance
