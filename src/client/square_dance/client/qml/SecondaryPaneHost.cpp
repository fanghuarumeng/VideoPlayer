#include "square_dance/client/qml/SecondaryPaneHost.h"

#include "square_dance/client/api/CatalogClient.h"
#include "square_dance/client/api/CatalogModel.h"
#include "square_dance/client/elder_ui/AppTheme.h"
#include "square_dance/client/elder_ui/SearchBar.h"
#include "square_dance/client/elder_ui/VideoResultList.h"
#include "square_dance/client/history/HistoryPane.h"
#include "square_dance/client/history/PlayHistoryStore.h"
#include "square_dance/client/library/LibraryPane.h"
#include "square_dance/client/library/UploadController.h"
#include "square_dance/client/playlist/PlaylistStore.h"

#include <QDialog>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QListView>
#include <QPushButton>
#include <QVBoxLayout>

namespace square_dance {
namespace client {

SecondaryPaneHost::SecondaryPaneHost(const ClientConfig& config,
                                     CatalogClient* catalog,
                                     CatalogModel* catalog_model,
                                     PlaylistStore* playlist,
                                     PlayHistoryStore* history,
                                     UploadController* upload, QObject* parent)
    : QObject(parent),
      config_(config),
      catalog_(catalog),
      catalog_model_(catalog_model),
      playlist_(playlist),
      history_(history),
      upload_(upload),
      library_pane_(NULL),
      history_pane_(NULL),
      search_bar_(NULL),
      result_list_(NULL) {
  library_pane_ = new LibraryPane();
  history_pane_ = new HistoryPane(history_);
  search_bar_ = new SearchBar();
  result_list_ = new VideoResultList();
  result_list_->setCatalogModel(catalog_model_);

  connect(library_pane_, &LibraryPane::refreshRequested, this,
          &SecondaryPaneHost::onLibraryRefresh);
  connect(library_pane_, &LibraryPane::uploadRequested, this,
          &SecondaryPaneHost::onLibraryUpload);
  connect(library_pane_, &LibraryPane::addToPlaylistRequested, this,
          &SecondaryPaneHost::onLibraryAddToPlaylist);
  connect(library_pane_, &LibraryPane::playRequested, this,
          &SecondaryPaneHost::libraryPlayRequested);
  connect(history_pane_, &HistoryPane::playRequested, this,
          &SecondaryPaneHost::historyPlayRequested);
  connect(search_bar_, &SearchBar::searchRequested, this,
          &SecondaryPaneHost::onSearchRequested);
  connect(catalog_, &CatalogClient::searchFinished, this,
          &SecondaryPaneHost::onSearchFinished);
  connect(result_list_, &QListView::doubleClicked, this, [this]() {
    const QModelIndex idx = result_list_->currentIndex();
    if (!idx.isValid() || catalog_model_ == NULL) {
      return;
    }
    emit libraryPlayRequested(catalog_model_->videoAt(idx.row()));
  });
}

void SecondaryPaneHost::setLibraryVideos(const QVector<Video>& videos) {
  if (library_pane_ != NULL) {
    library_pane_->setVideos(videos);
  }
}

void SecondaryPaneHost::upsertLibraryVideo(const Video& video) {
  if (library_pane_ != NULL) {
    library_pane_->upsertVideo(video);
  }
}

void SecondaryPaneHost::openLibrary() {
  QDialog dlg;
  dlg.setWindowTitle(QString::fromUtf8("视频库"));
  dlg.resize(900, 640);
  dlg.setStyleSheet(AppTheme::GlobalStylesheet());
  QVBoxLayout* layout = new QVBoxLayout(&dlg);
  layout->setContentsMargins(0, 0, 0, 0);
  library_pane_->setParent(&dlg);
  layout->addWidget(library_pane_);
  dlg.exec();
  library_pane_->setParent(NULL);
}

void SecondaryPaneHost::openHistory() {
  history_pane_->refresh();
  QDialog dlg;
  dlg.setWindowTitle(QString::fromUtf8("历史"));
  dlg.resize(720, 560);
  dlg.setStyleSheet(AppTheme::GlobalStylesheet());
  QVBoxLayout* layout = new QVBoxLayout(&dlg);
  layout->setContentsMargins(0, 0, 0, 0);
  history_pane_->setParent(&dlg);
  layout->addWidget(history_pane_);
  dlg.exec();
  history_pane_->setParent(NULL);
}

void SecondaryPaneHost::openSearch() {
  QDialog dlg;
  dlg.setWindowTitle(QString::fromUtf8("搜索"));
  dlg.resize(720, 560);
  dlg.setStyleSheet(AppTheme::GlobalStylesheet());

  QPushButton* add_btn =
      new QPushButton(QString::fromUtf8("加入播放列表"), &dlg);
  add_btn->setObjectName(QString::fromUtf8("iconButton"));
  connect(add_btn, &QPushButton::clicked, this,
          &SecondaryPaneHost::onAddSearchToPlaylist);

  QVBoxLayout* layout = new QVBoxLayout(&dlg);
  search_bar_->setParent(&dlg);
  result_list_->setParent(&dlg);
  layout->addWidget(search_bar_);
  layout->addWidget(result_list_, 1);
  layout->addWidget(add_btn, 0, Qt::AlignLeft);
  dlg.exec();
  search_bar_->setParent(NULL);
  result_list_->setParent(NULL);
}

void SecondaryPaneHost::onLibraryRefresh() {
  emit statusMessage(QString::fromUtf8("正在刷新视频库…"));
  catalog_->listVideos(200, 0);
}

void SecondaryPaneHost::onLibraryUpload() {
  const QStringList paths = QFileDialog::getOpenFileNames(
      NULL, QString::fromUtf8("选择要上传的视频"), QString(),
      QString::fromUtf8(
          "Videos (*.mp4 *.mkv *.webm *.avi *.mov *.mxf);;All (*.*)"));
  if (paths.isEmpty() || upload_ == NULL) {
    return;
  }
  upload_->enqueueFiles(paths);
}

void SecondaryPaneHost::onLibraryAddToPlaylist(const QVector<Video>& videos) {
  int added = 0;
  for (int i = 0; i < videos.size(); ++i) {
    if (playlist_->append(videos[i])) {
      ++added;
    } else {
      playlist_->mergeFromVideo(videos[i]);
    }
  }
  library_pane_->clearSelection();
  emit playlistChanged();
  emit statusMessage(QString::fromUtf8("已加入播放列表 %1 个").arg(added));
}

void SecondaryPaneHost::onSearchRequested(const QString& query) {
  catalog_model_->setFilterQuery(query);
  emit statusMessage(QString::fromUtf8("正在搜索…"));
  catalog_->search(query, 50);
}

void SecondaryPaneHost::onSearchFinished(const QVector<Video>& videos) {
  catalog_model_->resetFromSearch(videos);
  emit statusMessage(QString::fromUtf8("找到 %1 个结果").arg(videos.size()));
}

void SecondaryPaneHost::onAddSearchToPlaylist() {
  const QModelIndex idx = result_list_->currentIndex();
  if (!idx.isValid()) {
    emit statusMessage(QString::fromUtf8("请先选择一个视频"));
    return;
  }
  const Video video = catalog_model_->videoAt(idx.row());
  if (playlist_->append(video)) {
    emit playlistChanged();
    emit statusMessage(QString::fromUtf8("已加入播放列表：") + video.title);
  } else {
    emit statusMessage(QString::fromUtf8("已在播放列表中：") + video.title);
  }
}

}  // namespace client
}  // namespace square_dance
