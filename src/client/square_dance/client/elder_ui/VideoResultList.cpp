#include "square_dance/client/elder_ui/VideoResultList.h"

#include "square_dance/client/api/CatalogModel.h"
#include "square_dance/client/elder_ui/AppTheme.h"
#include "square_dance/client/elder_ui/VideoListDelegate.h"

#include <QAbstractItemView>

namespace square_dance {
namespace client {

VideoResultList::VideoResultList(QWidget* parent) : QListView(parent) {
  setFont(AppTheme::UiFont());
  setStyleSheet(AppTheme::ListPanelStyle());
  setItemDelegate(new VideoListDelegate(this));
  setUniformItemSizes(true);
  setSelectionMode(QAbstractItemView::SingleSelection);
  setAlternatingRowColors(false);
  setFocusPolicy(Qt::StrongFocus);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setSpacing(0);
  setToolTip(QString::fromUtf8("双击条目开始播放"));
}

void VideoResultList::setCatalogModel(CatalogModel* model) { setModel(model); }

}  // namespace client
}  // namespace square_dance
