#pragma once

#include <QListView>

namespace square_dance {
namespace client {

class CatalogModel;

class VideoResultList : public QListView {
  Q_OBJECT

 public:
  explicit VideoResultList(QWidget* parent = 0);

  void setCatalogModel(CatalogModel* model);
};

}  // namespace client
}  // namespace square_dance
