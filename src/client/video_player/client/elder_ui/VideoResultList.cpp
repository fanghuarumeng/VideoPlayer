#include "video_player/client/elder_ui/VideoResultList.h"

#include "video_player/client/api/CatalogModel.h"
#include "video_player/client/elder_ui/VideoListDelegate.h"

#include <QAbstractItemView>

namespace video_player {
namespace client {

VideoResultList::VideoResultList(QWidget* parent) : QListView(parent) {
  setObjectName(QString::fromUtf8("recommendList"));
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
}  // namespace video_player
