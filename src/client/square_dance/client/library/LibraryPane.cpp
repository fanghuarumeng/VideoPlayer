#include "square_dance/client/library/LibraryPane.h"

#include "square_dance/client/elder_ui/AppIcons.h"
#include "square_dance/client/elder_ui/AppTheme.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QVBoxLayout>

namespace square_dance {
namespace client {

LibraryPane::LibraryPane(QWidget* parent)
    : QWidget(parent),
      list_(new QListWidget(this)),
      refresh_btn_(new QPushButton(QString::fromUtf8("刷新"), this)),
      upload_btn_(new QPushButton(QString::fromUtf8("上传"), this)),
      add_btn_(new QPushButton(QString::fromUtf8("加入播放列表"), this)),
      empty_label_(new QLabel(
          QString::fromUtf8("暂无视频。可上传本地文件，或等待目录同步。"),
          this)) {
  list_->setViewMode(QListView::IconMode);
  list_->setResizeMode(QListView::Adjust);
  list_->setMovement(QListView::Static);
  list_->setSelectionMode(QAbstractItemView::ExtendedSelection);
  list_->setIconSize(QSize(140, 90));
  list_->setGridSize(QSize(160, 140));
  list_->setWordWrap(true);
  list_->setUniformItemSizes(true);
  list_->setStyleSheet(AppTheme::ListPanelStyle());

  refresh_btn_->setStyleSheet(AppTheme::IconButtonStyle());
  upload_btn_->setStyleSheet(AppTheme::PrimaryButtonStyle());
  add_btn_->setStyleSheet(AppTheme::IconButtonStyle());
  empty_label_->setStyleSheet(AppTheme::PanelTitleStyle());
  empty_label_->setWordWrap(true);

  QHBoxLayout* tools = new QHBoxLayout();
  tools->setSpacing(8);
  tools->addWidget(upload_btn_);
  tools->addWidget(add_btn_);
  tools->addStretch(1);
  tools->addWidget(refresh_btn_);

  QVBoxLayout* root = new QVBoxLayout(this);
  root->setContentsMargins(8, 8, 8, 8);
  root->setSpacing(8);
  root->addLayout(tools);
  root->addWidget(empty_label_);
  root->addWidget(list_, 1);

  connect(refresh_btn_, &QPushButton::clicked, this,
          &LibraryPane::refreshRequested);
  connect(upload_btn_, &QPushButton::clicked, this,
          &LibraryPane::uploadRequested);
  connect(add_btn_, &QPushButton::clicked, this, &LibraryPane::onAddClicked);
  connect(list_, &QListWidget::itemDoubleClicked, this,
          &LibraryPane::onItemDoubleClicked);
}

QIcon LibraryPane::PlaceholderIcon() const {
  return AppIcons::Film();
}

void LibraryPane::setVideos(const QVector<Video>& videos) {
  videos_ = videos;
  RebuildList();
}

void LibraryPane::upsertVideo(const Video& video) {
  for (int i = 0; i < videos_.size(); ++i) {
    if (videos_[i].id == video.id) {
      videos_[i] = video;
      RebuildList();
      return;
    }
  }
  videos_.prepend(video);
  RebuildList();
}

void LibraryPane::removeVideo(qint64 id) {
  for (int i = 0; i < videos_.size(); ++i) {
    if (videos_[i].id == id) {
      videos_.removeAt(i);
      RebuildList();
      return;
    }
  }
}

void LibraryPane::RebuildList() {
  list_->clear();
  empty_label_->setVisible(videos_.isEmpty());
  list_->setVisible(!videos_.isEmpty());
  for (int i = 0; i < videos_.size(); ++i) {
    const Video& v = videos_[i];
    QListWidgetItem* item = new QListWidgetItem(PlaceholderIcon(), v.title);
    item->setData(Qt::UserRole, static_cast<qulonglong>(v.id));
    item->setData(Qt::UserRole + 1, v.play_url);
    item->setData(Qt::UserRole + 2, v.thumbnail_url);
    item->setToolTip(v.title);
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    item->setCheckState(Qt::Unchecked);
    list_->addItem(item);
  }
}

QVector<Video> LibraryPane::selectedVideos() const {
  QVector<Video> out;
  for (int i = 0; i < list_->count(); ++i) {
    QListWidgetItem* item = list_->item(i);
    if (item == NULL) {
      continue;
    }
    const bool checked = item->checkState() == Qt::Checked;
    const bool selected = item->isSelected();
    if (!checked && !selected) {
      continue;
    }
    Video v;
    v.id = static_cast<qint64>(item->data(Qt::UserRole).toULongLong());
    v.title = item->text();
    v.play_url = item->data(Qt::UserRole + 1).toString();
    v.thumbnail_url = item->data(Qt::UserRole + 2).toString();
    out.push_back(v);
  }
  return out;
}

void LibraryPane::clearSelection() {
  list_->clearSelection();
  for (int i = 0; i < list_->count(); ++i) {
    list_->item(i)->setCheckState(Qt::Unchecked);
  }
}

void LibraryPane::onAddClicked() {
  const QVector<Video> selected = selectedVideos();
  if (!selected.isEmpty()) {
    emit addToPlaylistRequested(selected);
  }
}

void LibraryPane::onItemDoubleClicked() {
  QListWidgetItem* item = list_->currentItem();
  if (item == NULL) {
    return;
  }
  Video v;
  v.id = static_cast<qint64>(item->data(Qt::UserRole).toULongLong());
  v.title = item->text();
  v.play_url = item->data(Qt::UserRole + 1).toString();
  v.thumbnail_url = item->data(Qt::UserRole + 2).toString();
  emit playRequested(v);
}

}  // namespace client
}  // namespace square_dance
