#include "square_dance/client/elder_ui/VideoResultList.h"

#include "square_dance/client/api/CatalogModel.h"

#include <QAbstractItemView>
#include <QFont>

namespace square_dance {
namespace client {

VideoResultList::VideoResultList(QWidget* parent) : QListView(parent) {
  QFont body;
  body.setPointSize(16);
  setFont(body);
  setUniformItemSizes(true);
  setSelectionMode(QAbstractItemView::SingleSelection);
  setAlternatingRowColors(true);
}

void VideoResultList::setCatalogModel(CatalogModel* model) {
  setModel(model);
}

}  // namespace client
}  // namespace square_dance
