#include "square_dance/client/playlist/PlaylistPane.h"

#include "square_dance/client/elder_ui/AppTheme.h"
#include "square_dance/client/playlist/PlaylistModel.h"

#include <QHBoxLayout>
#include <QListView>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

namespace square_dance {
namespace client {

PlaylistPane::PlaylistPane(PlaylistStore* store, QWidget* parent)
    : QWidget(parent),
      store_(store),
      model_(new PlaylistModel(store, this)),
      list_(new QListView(this)),
      clear_btn_(new QPushButton(QString::fromUtf8("清空播放列表"), this)) {
  list_->setModel(model_);
  list_->setUniformItemSizes(true);
  list_->setSelectionMode(QAbstractItemView::SingleSelection);
  list_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

  clear_btn_->setStyleSheet(AppTheme::IconButtonStyle());

  QVBoxLayout* root = new QVBoxLayout(this);
  root->setContentsMargins(0, 0, 0, 0);
  root->setSpacing(8);
  root->addWidget(list_, 1);
  root->addWidget(clear_btn_, 0, Qt::AlignLeft);

  connect(clear_btn_, &QPushButton::clicked, this,
          &PlaylistPane::onClearClicked);
  connect(list_, &QListView::doubleClicked, this,
          &PlaylistPane::onDoubleClicked);
}

PlaylistModel* PlaylistPane::model() const {
  return model_;
}

void PlaylistPane::refresh() {
  model_->reload();
}

bool PlaylistPane::selectedItem(PlaylistItem* out) const {
  if (out == NULL) {
    return false;
  }
  const QModelIndex idx = list_->currentIndex();
  if (!idx.isValid()) {
    return false;
  }
  *out = model_->itemAt(idx.row());
  return out->id != 0;
}

void PlaylistPane::selectRow(int row) {
  if (row < 0 || row >= model_->rowCount()) {
    return;
  }
  list_->setCurrentIndex(model_->index(row, 0));
}

void PlaylistPane::onClearClicked() {
  const int ret = QMessageBox::question(
      this, QString::fromUtf8("清空播放列表"),
      QString::fromUtf8("确定清空播放列表吗？"),
      QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
  if (ret != QMessageBox::Yes || store_ == NULL) {
    return;
  }
  store_->clear();
  model_->reload();
  emit cleared();
}

void PlaylistPane::onDoubleClicked(const QModelIndex& index) {
  if (!index.isValid()) {
    return;
  }
  const PlaylistItem item = model_->itemAt(index.row());
  if (item.id == 0) {
    return;
  }
  if (store_ != NULL) {
    store_->setCurrentIndex(index.row());
  }
  emit playRequested(item);
}

}  // namespace client
}  // namespace square_dance
