#include "square_dance/client/elder_ui/ElderMainWindow.h"

#include "square_dance/client/api/CatalogClient.h"
#include "square_dance/client/api/CatalogModel.h"
#include "square_dance/client/api/PushClient.h"
#include "square_dance/client/api/Video.h"
#include "square_dance/client/download/DownloadService.h"
#include "square_dance/client/elder_ui/AppIcons.h"
#include "square_dance/client/elder_ui/AppTheme.h"
#include "square_dance/client/elder_ui/ConnectionStatusWidget.h"
#include "square_dance/client/elder_ui/PlayerPane.h"
#include "square_dance/client/elder_ui/SearchBar.h"
#include "square_dance/client/elder_ui/VideoResultList.h"
#include "square_dance/client/playback/PlaybackController.h"
#include "square_dance/client/playback/VideoPlayerWidget.h"
#include "square_dance/client/playlist/PlaylistPane.h"
#include "square_dance/client/playlist/PlaylistStore.h"

#include <QColor>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListView>
#include <QPalette>
#include <QPushButton>
#include <QSplitter>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTabWidget>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

namespace square_dance {
namespace client {

ElderMainWindow::ElderMainWindow(const ClientConfig& config, QWidget* parent)
    : QMainWindow(parent),
      config_(config),
      search_bar_(NULL),
      result_list_(NULL),
      player_pane_(NULL),
      connection_status_(NULL),
      status_label_(NULL),
      side_tabs_(NULL),
      playlist_store_(NULL),
      playlist_pane_(NULL),
      model_(new CatalogModel(this)),
      catalog_(new CatalogClient(config.http_base_url, this)),
      push_(new PushClient(this)),
      playback_(new PlaybackController(this)),
      downloads_(new DownloadService(config.download_dir, this)),
      playing_from_playlist_(false) {
  setWindowTitle(QString::fromUtf8("Square Dance"));
  setWindowIcon(AppIcons::Logo());
  resize(1280, 800);

  setStyleSheet(AppTheme::GlobalStylesheet());
  QPalette pal = palette();
  pal.setColor(QPalette::Window, QColor(AppTheme::WindowBg()));
  pal.setColor(QPalette::Base, QColor(AppTheme::Surface()));
  pal.setColor(QPalette::Text, QColor(AppTheme::TextPrimary()));
  pal.setColor(QPalette::WindowText, QColor(AppTheme::TextPrimary()));
  setPalette(pal);
  setAutoFillBackground(true);

  playlist_store_ = new PlaylistStore(ResolvePlaylistPath());
  QString pl_err;
  playlist_store_->load(&pl_err);

  QLabel* logo = new QLabel(this);
  logo->setPixmap(AppIcons::Logo().pixmap(36, 36));

  QLabel* brand = new QLabel(QString::fromUtf8("Square Dance"), this);
  brand->setFont(AppTheme::TitleFont());
  brand->setStyleSheet(AppTheme::BrandTitleStyle());

  QLabel* tagline =
      new QLabel(QString::fromUtf8("视频库 · 播放列表 · 播放"), this);
  tagline->setFont(AppTheme::CaptionFont());
  tagline->setStyleSheet(AppTheme::PanelTitleStyle());

  connection_status_ = new ConnectionStatusWidget(this);
  search_bar_ = new SearchBar(this);
  result_list_ = new VideoResultList(this);
  result_list_->setCatalogModel(model_);
  result_list_->setMinimumWidth(280);

  QPushButton* add_pl_btn =
      new QPushButton(QString::fromUtf8("加入播放列表"), this);
  add_pl_btn->setStyleSheet(AppTheme::IconButtonStyle());

  QWidget* search_page = new QWidget(this);
  QVBoxLayout* search_layout = new QVBoxLayout(search_page);
  search_layout->setContentsMargins(8, 8, 8, 8);
  search_layout->setSpacing(8);
  search_layout->addWidget(search_bar_);
  search_layout->addWidget(result_list_, 1);
  search_layout->addWidget(add_pl_btn, 0, Qt::AlignLeft);

  playlist_pane_ = new PlaylistPane(playlist_store_, this);

  side_tabs_ = new QTabWidget(this);
  side_tabs_->addTab(search_page, QString::fromUtf8("搜索"));
  side_tabs_->addTab(playlist_pane_, QString::fromUtf8("播放列表"));

  QFrame* sidebar = new QFrame(this);
  sidebar->setStyleSheet(QString::fromUtf8(
      "QFrame { background: %1; border: 1px solid %2; border-radius: 12px; }")
                             .arg(AppTheme::Surface(), AppTheme::Border()));
  QVBoxLayout* sidebar_layout = new QVBoxLayout(sidebar);
  sidebar_layout->setContentsMargins(8, 8, 8, 8);
  sidebar_layout->addWidget(side_tabs_, 1);

  player_pane_ = new PlayerPane(this);
  playback_->setPlayerWidget(player_pane_->playerWidget());

  status_label_ = new QLabel(QString::fromUtf8("搜索视频或打开播放列表"), this);
  status_label_->setFont(AppTheme::CaptionFont());
  statusBar()->addWidget(status_label_, 1);

  QHBoxLayout* brand_row = new QHBoxLayout();
  brand_row->setSpacing(10);
  brand_row->addWidget(logo);
  QVBoxLayout* brand_text = new QVBoxLayout();
  brand_text->setSpacing(0);
  brand_text->addWidget(brand);
  brand_text->addWidget(tagline);
  brand_row->addLayout(brand_text);
  brand_row->addStretch(1);
  brand_row->addWidget(connection_status_, 0, Qt::AlignVCenter);

  QSplitter* split = new QSplitter(Qt::Horizontal, this);
  split->addWidget(sidebar);
  split->addWidget(player_pane_);
  split->setStretchFactor(0, 0);
  split->setStretchFactor(1, 1);
  split->setChildrenCollapsible(false);
  split->setHandleWidth(1);
  QList<int> sizes;
  sizes << 380 << 900;
  split->setSizes(sizes);

  QWidget* central = new QWidget(this);
  central->setAutoFillBackground(true);
  QVBoxLayout* root = new QVBoxLayout(central);
  root->setContentsMargins(16, 14, 16, 8);
  root->setSpacing(12);
  root->addLayout(brand_row);
  root->addWidget(split, 1);
  setCentralWidget(central);

  connect(result_list_, &QListView::doubleClicked, this,
          &ElderMainWindow::onPlayClicked);
  connect(search_bar_, &SearchBar::searchRequested, this,
          &ElderMainWindow::onSearchRequested);
  connect(add_pl_btn, &QPushButton::clicked, this,
          &ElderMainWindow::onAddSearchToPlaylist);
  connect(catalog_, &CatalogClient::searchFinished, this,
          &ElderMainWindow::onSearchFinished);
  connect(catalog_, &CatalogClient::searchFailed, this,
          &ElderMainWindow::onSearchFailed);
  connect(push_, &PushClient::videoUpserted, model_,
          &CatalogModel::applyUpserted);
  connect(push_, &PushClient::videoRemoved, model_,
          &CatalogModel::applyRemoved);
  connect(push_, &PushClient::connectionChanged, this,
          &ElderMainWindow::onConnectionChanged);
  connect(player_pane_, &PlayerPane::playClicked, this,
          &ElderMainWindow::onPlayClicked);
  connect(player_pane_, &PlayerPane::pauseClicked, this,
          &ElderMainWindow::onPauseClicked);
  connect(player_pane_, &PlayerPane::stopClicked, this,
          &ElderMainWindow::onStopClicked);
  connect(player_pane_, &PlayerPane::downloadClicked, this,
          &ElderMainWindow::onDownloadClicked);
  connect(playback_, &PlaybackController::errorOccurred, this,
          &ElderMainWindow::onPlaybackError);
  connect(playback_, &PlaybackController::statusMessage, this,
          &ElderMainWindow::SetStatusMessage);
  connect(playback_, &PlaybackController::playbackProgress, player_pane_,
          &PlayerPane::setPlaybackProgress);
  connect(playback_, &PlaybackController::playbackFinished, this,
          &ElderMainWindow::onPlaybackFinished);
  connect(player_pane_, &PlayerPane::seekRequested, playback_,
          &PlaybackController::seekTo);
  connect(player_pane_, &PlayerPane::scrubbingChanged, playback_,
          &PlaybackController::setScrubbing);
  connect(downloads_, &DownloadService::progress, this,
          &ElderMainWindow::onDownloadProgress);
  connect(downloads_, &DownloadService::finished, this,
          &ElderMainWindow::onDownloadFinished);
  connect(downloads_, &DownloadService::errorOccurred, this,
          &ElderMainWindow::onDownloadError);
  connect(playlist_pane_, &PlaylistPane::playRequested, this,
          &ElderMainWindow::onPlaylistPlayRequested);

  push_->connectToServer(QUrl(config_.ws_url));
}

QString ElderMainWindow::ResolvePlaylistPath() const {
  if (!config_.playlist_path.isEmpty()) {
    return config_.playlist_path;
  }
  return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
         QString::fromUtf8("/playlist.json");
}

Video ElderMainWindow::VideoFromPlaylistItem(const PlaylistItem& item) const {
  Video video;
  video.id = item.id;
  video.title = item.title;
  video.play_url = item.play_url;
  video.thumbnail_url = item.thumbnail_url;
  video.source_type = QString::fromUtf8("playlist");
  return video;
}

void ElderMainWindow::PlayVideo(const Video& video) {
  SetStatusMessage(QString::fromUtf8("正在播放：") + video.title);
  playback_->play(video);
}

bool ElderMainWindow::SelectedSearchVideo(Video* out) const {
  if (out == NULL) {
    return false;
  }
  const QModelIndex idx = result_list_->currentIndex();
  if (!idx.isValid()) {
    return false;
  }
  *out = model_->videoAt(idx.row());
  return out->id != 0 || !out->play_url.isEmpty();
}

void ElderMainWindow::SetStatusMessage(const QString& message) {
  status_label_->setText(message);
}

void ElderMainWindow::onSearchRequested(const QString& query) {
  current_query_ = query;
  model_->setFilterQuery(query);
  SetStatusMessage(QString::fromUtf8("正在搜索…"));
  catalog_->search(query, 50);
}

void ElderMainWindow::onSearchFinished(
    const QVector<square_dance::client::Video>& videos) {
  model_->resetFromSearch(videos);
  model_->setFilterQuery(current_query_);
  SetStatusMessage(QString::fromUtf8("找到 %1 个结果").arg(videos.size()));
  if (!videos.isEmpty()) {
    result_list_->setCurrentIndex(model_->index(0, 0));
  }
}

void ElderMainWindow::onSearchFailed(const QString& error) {
  SetStatusMessage(QString::fromUtf8("搜索失败：") + error);
}

void ElderMainWindow::onPlayClicked() {
  if (side_tabs_ != NULL && side_tabs_->currentWidget() == playlist_pane_) {
    PlaylistItem item;
    if (playlist_pane_->selectedItem(&item)) {
      onPlaylistPlayRequested(item);
      return;
    }
  }
  Video video;
  if (!SelectedSearchVideo(&video)) {
    SetStatusMessage(QString::fromUtf8("请先选择一个视频"));
    return;
  }
  playing_from_playlist_ = false;
  PlayVideo(video);
}

void ElderMainWindow::onPlaylistPlayRequested(const PlaylistItem& item) {
  playing_from_playlist_ = true;
  const int idx = playlist_store_->currentIndex();
  (void)idx;
  // Ensure current index matches the requested item.
  const QVector<PlaylistItem> items = playlist_store_->items();
  for (int i = 0; i < items.size(); ++i) {
    if (items[i].id == item.id) {
      playlist_store_->setCurrentIndex(i);
      playlist_pane_->selectRow(i);
      break;
    }
  }
  PlayVideo(VideoFromPlaylistItem(item));
}

void ElderMainWindow::onPlaybackFinished() {
  if (!playing_from_playlist_ || playlist_store_ == NULL) {
    return;
  }
  const int cur = playlist_store_->currentIndex();
  const int next = cur + 1;
  if (next < 0 || next >= playlist_store_->items().size()) {
    SetStatusMessage(QString::fromUtf8("播放列表已播完"));
    return;
  }
  playlist_store_->setCurrentIndex(next);
  playlist_pane_->selectRow(next);
  const PlaylistItem item = playlist_store_->items()[next];
  PlayVideo(VideoFromPlaylistItem(item));
}

void ElderMainWindow::onAddSearchToPlaylist() {
  Video video;
  if (!SelectedSearchVideo(&video)) {
    SetStatusMessage(QString::fromUtf8("请先选择一个视频"));
    return;
  }
  if (playlist_store_->append(video)) {
    playlist_pane_->refresh();
    SetStatusMessage(QString::fromUtf8("已加入播放列表：") + video.title);
  } else {
    SetStatusMessage(QString::fromUtf8("已在播放列表中：") + video.title);
  }
}

void ElderMainWindow::onPauseClicked() { playback_->pause(); }

void ElderMainWindow::onStopClicked() {
  playing_from_playlist_ = false;
  playback_->stop();
  player_pane_->resetPlaybackProgress();
}

void ElderMainWindow::onDownloadClicked() {
  Video video;
  if (!SelectedSearchVideo(&video)) {
    PlaylistItem item;
    if (playlist_pane_->selectedItem(&item)) {
      video = VideoFromPlaylistItem(item);
    } else {
      SetStatusMessage(QString::fromUtf8("请先选择一个视频"));
      return;
    }
  }
  SetStatusMessage(QString::fromUtf8("开始下载：") + video.title);
  downloads_->startDownload(video);
}

void ElderMainWindow::onPlaybackError(const QString& message) {
  SetStatusMessage(QString::fromUtf8("播放错误：") + message);
}

void ElderMainWindow::onDownloadProgress(qint64 /*video_id*/, qint64 received,
                                         qint64 total) {
  if (total > 0) {
    const int pct = static_cast<int>((received * 100) / total);
    SetStatusMessage(QString::fromUtf8("下载进度：%1%").arg(pct));
  } else {
    SetStatusMessage(QString::fromUtf8("已下载 %1 字节").arg(received));
  }
}

void ElderMainWindow::onDownloadFinished(qint64 /*video_id*/,
                                         const QString& local_path) {
  SetStatusMessage(QString::fromUtf8("下载完成：") + local_path);
}

void ElderMainWindow::onDownloadError(qint64 /*video_id*/,
                                      const QString& message) {
  SetStatusMessage(QString::fromUtf8("下载失败：") + message);
}

void ElderMainWindow::onConnectionChanged(bool connected) {
  connection_status_->setConnected(connected);
  if (connected) {
    SetStatusMessage(QString::fromUtf8("已连接实时推送"));
  } else {
    SetStatusMessage(QString::fromUtf8("推送未连接，正在重试…"));
  }
}

}  // namespace client
}  // namespace square_dance
