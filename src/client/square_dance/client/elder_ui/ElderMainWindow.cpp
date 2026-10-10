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
#include "square_dance/client/library/LibraryPane.h"
#include "square_dance/client/history/PlayHistoryStore.h"
#include "square_dance/client/library/UploadController.h"
#include "square_dance/client/playback/PlaybackController.h"
#include "square_dance/client/playback/VideoPlayerWidget.h"
#include "square_dance/client/playlist/PlaylistPane.h"
#include "square_dance/client/playlist/PlaylistStore.h"

#include <QColor>
#include <QDateTime>
#include <QFileDialog>
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
      library_pane_(NULL),
      upload_(NULL),
      history_store_(NULL),
      history_placeholder_(NULL),
      model_(new CatalogModel(this)),
      catalog_(new CatalogClient(config.http_base_url, this)),
      push_(new PushClient(this)),
      playback_(new PlaybackController(this)),
      downloads_(new DownloadService(config.download_dir, this)),
      playing_from_playlist_(false),
      last_history_save_ms_(0) {
  upload_ = new UploadController(config.http_base_url, 2, this);
  history_store_ = new PlayHistoryStore(ResolveHistoryPath(), 100);
  QString hist_err;
  history_store_->load(&hist_err);
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
      new QLabel(QString::fromUtf8("视频库 · 播放列表 · 历史 · 搜索"), this);
  tagline->setFont(AppTheme::CaptionFont());
  tagline->setStyleSheet(AppTheme::PanelTitleStyle());
  connection_status_ = new ConnectionStatusWidget(this);

  library_pane_ = new LibraryPane(this);
  playlist_pane_ = new PlaylistPane(playlist_store_, this);

  history_placeholder_ = new QWidget(this);
  QVBoxLayout* hist_layout = new QVBoxLayout(history_placeholder_);
  QLabel* hist_label = new QLabel(
      QString::fromUtf8("播放历史将在后续版本启用。"), history_placeholder_);
  hist_label->setStyleSheet(AppTheme::PanelTitleStyle());
  hist_layout->addWidget(hist_label);
  hist_layout->addStretch(1);

  search_bar_ = new SearchBar(this);
  result_list_ = new VideoResultList(this);
  result_list_->setCatalogModel(model_);
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

  side_tabs_ = new QTabWidget(this);
  side_tabs_->addTab(library_pane_, QString::fromUtf8("视频库"));
  side_tabs_->addTab(playlist_pane_, QString::fromUtf8("播放列表"));
  side_tabs_->addTab(history_placeholder_, QString::fromUtf8("历史"));
  side_tabs_->addTab(search_page, QString::fromUtf8("搜索"));
  side_tabs_->setCurrentIndex(0);

  QFrame* sidebar = new QFrame(this);
  sidebar->setStyleSheet(QString::fromUtf8(
      "QFrame { background: %1; border: 1px solid %2; border-radius: 12px; }")
                             .arg(AppTheme::Surface(), AppTheme::Border()));
  QVBoxLayout* sidebar_layout = new QVBoxLayout(sidebar);
  sidebar_layout->setContentsMargins(8, 8, 8, 8);
  sidebar_layout->addWidget(side_tabs_, 1);

  player_pane_ = new PlayerPane(this);
  playback_->setPlayerWidget(player_pane_->playerWidget());

  status_label_ =
      new QLabel(QString::fromUtf8("正在加载视频库…"), this);
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
  QList<int> sizes;
  sizes << 420 << 860;
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
  connect(catalog_, &CatalogClient::listFinished, this,
          &ElderMainWindow::onListFinished);
  connect(catalog_, &CatalogClient::listFailed, this,
          &ElderMainWindow::onListFailed);
  connect(push_, &PushClient::videoUpserted, this,
          &ElderMainWindow::onVideoUpserted);
  connect(push_, &PushClient::videoRemoved, this,
          &ElderMainWindow::onVideoRemoved);
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
  connect(playback_, &PlaybackController::playbackProgress, this,
          &ElderMainWindow::onPlaybackProgressForHistory);
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
  connect(library_pane_, &LibraryPane::refreshRequested, this,
          &ElderMainWindow::onLibraryRefresh);
  connect(library_pane_, &LibraryPane::uploadRequested, this,
          &ElderMainWindow::onLibraryUpload);
  connect(library_pane_, &LibraryPane::addToPlaylistRequested, this,
          &ElderMainWindow::onLibraryAddToPlaylist);
  connect(library_pane_, &LibraryPane::playRequested, this,
          &ElderMainWindow::onLibraryPlayRequested);
  connect(upload_, &UploadController::statusMessage, this,
          &ElderMainWindow::SetStatusMessage);
  connect(upload_, &UploadController::uploadFinished, this,
          &ElderMainWindow::onUploadFinished);
  connect(upload_, &UploadController::uploadFailed, this,
          [this](const QString& path, const QString& error) {
            SetStatusMessage(QString::fromUtf8("上传失败：") + path +
                             QString::fromUtf8(" — ") + error);
          });

  push_->connectToServer(QUrl(config_.ws_url));
  catalog_->listVideos(200, 0);
}

QString ElderMainWindow::ResolvePlaylistPath() const {
  if (!config_.playlist_path.isEmpty()) {
    return config_.playlist_path;
  }
  return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
         QString::fromUtf8("/playlist.json");
}

QString ElderMainWindow::ResolveHistoryPath() const {
  return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
         QString::fromUtf8("/play_history.json");
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
  current_video_ = video;
  last_history_save_ms_ = 0;
  SetStatusMessage(QString::fromUtf8("正在播放：") + video.title);
  playback_->play(video);
  RecordHistory(0.0, 0.0);
}

void ElderMainWindow::RecordHistory(double position_sec, double duration_sec) {
  if (history_store_ == NULL || current_video_.id == 0) {
    return;
  }
  const qint64 pos_ms = static_cast<qint64>(position_sec * 1000.0);
  const qint64 dur_ms = static_cast<qint64>(duration_sec * 1000.0);
  history_store_->recordProgress(current_video_, pos_ms, dur_ms);
  last_history_save_ms_ = QDateTime::currentMSecsSinceEpoch();
}

void ElderMainWindow::onPlaybackProgressForHistory(double position_sec,
                                                   double duration_sec) {
  const qint64 now = QDateTime::currentMSecsSinceEpoch();
  if (last_history_save_ms_ != 0 && now - last_history_save_ms_ < 5000) {
    return;
  }
  RecordHistory(position_sec, duration_sec);
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

void ElderMainWindow::onLibraryRefresh() {
  SetStatusMessage(QString::fromUtf8("正在刷新视频库…"));
  catalog_->listVideos(200, 0);
}

void ElderMainWindow::onLibraryUpload() {
  const QStringList paths = QFileDialog::getOpenFileNames(
      this, QString::fromUtf8("选择要上传的视频"), QString(),
      QString::fromUtf8(
          "Videos (*.mp4 *.mkv *.webm *.avi *.mov *.mxf);;All (*.*)"));
  if (paths.isEmpty()) {
    return;
  }
  upload_->enqueueFiles(paths);
}

void ElderMainWindow::onUploadFinished(const Video& video) {
  library_pane_->upsertVideo(video);
}

void ElderMainWindow::onListFinished(const QVector<Video>& videos) {
  library_pane_->setVideos(videos);
  SetStatusMessage(QString::fromUtf8("视频库共 %1 个").arg(videos.size()));
}

void ElderMainWindow::onListFailed(const QString& error) {
  SetStatusMessage(QString::fromUtf8("加载视频库失败：") + error);
}

void ElderMainWindow::onLibraryAddToPlaylist(const QVector<Video>& videos) {
  int added = 0;
  for (int i = 0; i < videos.size(); ++i) {
    if (playlist_store_->append(videos[i])) {
      ++added;
    }
  }
  playlist_pane_->refresh();
  library_pane_->clearSelection();
  SetStatusMessage(QString::fromUtf8("已加入播放列表 %1 个").arg(added));
}

void ElderMainWindow::onLibraryPlayRequested(const Video& video) {
  playing_from_playlist_ = false;
  PlayVideo(video);
}

void ElderMainWindow::onVideoUpserted(const Video& video) {
  model_->applyUpserted(video);
  library_pane_->upsertVideo(video);
}

void ElderMainWindow::onVideoRemoved(qint64 id) {
  model_->applyRemoved(id);
  library_pane_->removeVideo(id);
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
  if (side_tabs_->currentWidget() == playlist_pane_) {
    PlaylistItem item;
    if (playlist_pane_->selectedItem(&item)) {
      onPlaylistPlayRequested(item);
      return;
    }
  }
  if (side_tabs_->currentWidget() == library_pane_) {
    const QVector<Video> selected = library_pane_->selectedVideos();
    if (!selected.isEmpty()) {
      onLibraryPlayRequested(selected.first());
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
  const int next = playlist_store_->currentIndex() + 1;
  if (next < 0 || next >= playlist_store_->items().size()) {
    SetStatusMessage(QString::fromUtf8("播放列表已播完"));
    return;
  }
  playlist_store_->setCurrentIndex(next);
  playlist_pane_->selectRow(next);
  PlayVideo(VideoFromPlaylistItem(playlist_store_->items()[next]));
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
  // Best-effort persist last known progress already saved by progress hook.
}

void ElderMainWindow::onDownloadClicked() {
  Video video;
  if (!SelectedSearchVideo(&video)) {
    PlaylistItem item;
    if (playlist_pane_->selectedItem(&item)) {
      video = VideoFromPlaylistItem(item);
    } else {
      const QVector<Video> selected = library_pane_->selectedVideos();
      if (!selected.isEmpty()) {
        video = selected.first();
      } else {
        SetStatusMessage(QString::fromUtf8("请先选择一个视频"));
        return;
      }
    }
  }
  SetStatusMessage(QString::fromUtf8("开始下载：") + video.title);
  downloads_->startDownload(video);
}

void ElderMainWindow::onPlaybackError(const QString& message) {
  SetStatusMessage(QString::fromUtf8("播放错误：") + message);
}

void ElderMainWindow::onDownloadProgress(qint64, qint64 received, qint64 total) {
  if (total > 0) {
    SetStatusMessage(QString::fromUtf8("下载进度：%1%").arg(
        static_cast<int>((received * 100) / total)));
  } else {
    SetStatusMessage(QString::fromUtf8("已下载 %1 字节").arg(received));
  }
}

void ElderMainWindow::onDownloadFinished(qint64, const QString& local_path) {
  SetStatusMessage(QString::fromUtf8("下载完成：") + local_path);
}

void ElderMainWindow::onDownloadError(qint64, const QString& message) {
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
