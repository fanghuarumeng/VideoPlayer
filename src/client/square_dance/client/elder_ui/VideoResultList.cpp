#include "square_dance/client/elder_ui/VideoResultList.h"

#include "square_dance/client/api/CatalogModel.h"
#include "square_dance/client/elder_ui/ElderStyle.h"

#include <QAbstractItemView>

namespace square_dance {
namespace client {

VideoResultList::VideoResultList(QWidget* parent) : QListView(parent) {
  setFont(ElderStyle::BodyFont());
  setStyleSheet(ElderStyle::ListStyle());
  setUniformItemSizes(true);
  setSelectionMode(QAbstractItemView::SingleSelection);
  setAlternatingRowColors(false);
  setFocusPolicy(Qt::StrongFocus);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void VideoResultList::setCatalogModel(CatalogModel* model) { setModel(model); }

}  // namespace client
}  // namespace square_dance
