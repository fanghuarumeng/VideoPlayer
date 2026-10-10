#include "square_dance/client/playback/PlaybackController.h"

#include "square_dance/client/playback/FfplayLocator.h"
#include "square_dance/client/playback/VideoPlayerWidget.h"

#include <QAudioFormat>
#include <QAudioOutput>
#include <QDesktopServices>
#include <QFileInfo>
#include <QTimer>
#include <QUrl>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <tlhelp32.h>
#endif

namespace square_dance {
namespace client {

namespace {
const int kMaxQueuedFrames = 15;
const int kFrameIntervalMs = 40;
}  // namespace

PlaybackController::PlaybackController(QObject* parent)
    : QObject(parent),
      player_widget_(NULL),
      video_proc_(new QProcess(this)),
      audio_proc_(new QProcess(this)),
      audio_out_(NULL),
      audio_device_(NULL),
      frame_timer_(new QTimer(this)),
      frame_w_(640),
      frame_h_(360),
      frame_bytes_(640 * 360 * 3),
      paused_(false),
      playing_(false),
      has_presented_frame_(false) {
  video_proc_->setProcessChannelMode(QProcess::SeparateChannels);
  audio_proc_->setProcessChannelMode(QProcess::SeparateChannels);

  connect(video_proc_, &QProcess::readyReadStandardOutput, this,
          &PlaybackController::onVideoReadyRead);
  connect(audio_proc_, &QProcess::readyReadStandardOutput, this,
          &PlaybackController::onAudioReadyRead);
  connect(video_proc_,
          static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(
              &QProcess::finished),
          this, &PlaybackController::onVideoFinished);
  connect(audio_proc_,
          static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(
              &QProcess::finished),
          this, &PlaybackController::onAudioFinished);

  frame_timer_->setInterval(kFrameIntervalMs);
  connect(frame_timer_, &QTimer::timeout, this,
          &PlaybackController::onFrameTick);
}

PlaybackController::~PlaybackController() { stop(); }

void PlaybackController::setPlayerWidget(VideoPlayerWidget* widget) {
  player_widget_ = widget;
}

QUrl PlaybackController::ResolvePlayUrl(const QString& url_text) const {
  const QUrl url(url_text);
  if (url.isLocalFile()) {
    const QString local = url.toLocalFile();
    if (QFileInfo::exists(local)) {
      return QUrl::fromLocalFile(local);
    }
  }
  if (url.scheme().isEmpty() && QFileInfo::exists(url_text)) {
    return QUrl::fromLocalFile(url_text);
  }
  return url;
}

void PlaybackController::UpdateHint(const QString& text) {
  if (player_widget_ != NULL) {
    player_widget_->setHintText(text);
  }
}

QSize PlaybackController::ChooseFrameSize() const {
  int w = 960;
  int h = 540;
  if (player_widget_ != NULL) {
    w = player_widget_->width();
    h = player_widget_->height();
  }
  if (w < 320) {
    w = 640;
  }
  if (h < 180) {
    h = 360;
  }
  w = w - (w % 2);
  h = h - (h % 2);
  return QSize(w, h);
}

bool PlaybackController::SetProcessSuspended(QProcess* process, bool suspend) {
#ifdef Q_OS_WIN
  if (process == NULL || process->state() != QProcess::Running) {
    return false;
  }
  const DWORD pid = static_cast<DWORD>(process->processId());
  HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
  if (snapshot == INVALID_HANDLE_VALUE) {
    return false;
  }
  THREADENTRY32 entry;
  entry.dwSize = sizeof(entry);
  bool touched = false;
  if (Thread32First(snapshot, &entry)) {
    do {
      if (entry.th32OwnerProcessID != pid) {
        continue;
      }
      HANDLE thread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, entry.th32ThreadID);
      if (thread == NULL) {
        continue;
      }
      if (suspend) {
        SuspendThread(thread);
      } else {
        ResumeThread(thread);
      }
      CloseHandle(thread);
      touched = true;
    } while (Thread32Next(snapshot, &entry));
  }
  CloseHandle(snapshot);
  return touched;
#else
  Q_UNUSED(process);
  Q_UNUSED(suspend);
  return false;
#endif
}

void PlaybackController::StopPipelines() {
  if (paused_) {
    SetProcessSuspended(video_proc_, false);
    SetProcessSuspended(audio_proc_, false);
  }
  playing_ = false;
  paused_ = false;
  has_presented_frame_ = false;
  frame_timer_->stop();
  video_buffer_.clear();
  frame_queue_.clear();

  if (audio_out_ != NULL) {
    audio_out_->stop();
    audio_out_->deleteLater();
    audio_out_ = NULL;
    audio_device_ = NULL;
  }

  if (video_proc_->state() != QProcess::NotRunning) {
    video_proc_->kill();
  }
  if (audio_proc_->state() != QProcess::NotRunning) {
    audio_proc_->kill();
  }
}

bool PlaybackController::StartPipelines(const QUrl& media_url,
                                        const QString& title,
                                        QString* error) {
  const QString ffmpeg = FindFfmpegExecutable();
  if (ffmpeg.isEmpty()) {
    if (error != NULL) {
      *error = QString::fromUtf8("ffmpeg not found");
    }
    return false;
  }
  if (player_widget_ == NULL) {
    if (error != NULL) {
      *error = QString::fromUtf8("播放控件未就绪");
    }
    return false;
  }

  StopPipelines();

  const QSize frame_size = ChooseFrameSize();
  frame_w_ = frame_size.width();
  frame_h_ = frame_size.height();
  frame_bytes_ = frame_w_ * frame_h_ * 3;

  const QString input = media_url.isLocalFile() ? media_url.toLocalFile()
                                               : media_url.toString();
  const QString size_arg =
      QString::number(frame_w_) + QString::fromUtf8("x") +
      QString::number(frame_h_);

  // Opaque cover before any pipeline IO; first painted frame removes it.
  player_widget_->setLoadingCoverVisible(true);
  player_widget_->clearFrame();
  has_presented_frame_ = false;

  QStringList video_args;
  video_args << QString::fromUtf8("-hide_banner")
             << QString::fromUtf8("-loglevel") << QString::fromUtf8("error")
             << QString::fromUtf8("-re")
             << QString::fromUtf8("-i") << input
             << QString::fromUtf8("-an")
             << QString::fromUtf8("-f") << QString::fromUtf8("rawvideo")
             << QString::fromUtf8("-pix_fmt") << QString::fromUtf8("rgb24")
             << QString::fromUtf8("-s") << size_arg
             << QString::fromUtf8("-vf")
             << (QString::fromUtf8("scale=") + QString::number(frame_w_) +
                 QString::fromUtf8(":") + QString::number(frame_h_))
             << QString::fromUtf8("-");

  QStringList audio_args;
  audio_args << QString::fromUtf8("-hide_banner")
             << QString::fromUtf8("-loglevel") << QString::fromUtf8("error")
             << QString::fromUtf8("-re")
             << QString::fromUtf8("-i") << input
             << QString::fromUtf8("-vn")
             << QString::fromUtf8("-f") << QString::fromUtf8("s16le")
             << QString::fromUtf8("-acodec") << QString::fromUtf8("pcm_s16le")
             << QString::fromUtf8("-ar") << QString::fromUtf8("44100")
             << QString::fromUtf8("-ac") << QString::fromUtf8("2")
             << QString::fromUtf8("-");

  video_proc_->start(ffmpeg, video_args);
  if (!video_proc_->waitForStarted(4000)) {
    if (error != NULL) {
      *error = video_proc_->errorString();
    }
    player_widget_->setLoadingCoverVisible(false);
    player_widget_->showHintPage();
    return false;
  }

  audio_proc_->start(ffmpeg, audio_args);
  if (!audio_proc_->waitForStarted(4000)) {
    video_proc_->kill();
    if (error != NULL) {
      *error = audio_proc_->errorString();
    }
    player_widget_->setLoadingCoverVisible(false);
    player_widget_->showHintPage();
    return false;
  }

  QAudioFormat format;
  format.setSampleRate(44100);
  format.setChannelCount(2);
  format.setSampleSize(16);
  format.setCodec(QString::fromUtf8("audio/pcm"));
  format.setByteOrder(QAudioFormat::LittleEndian);
  format.setSampleType(QAudioFormat::SignedInt);

  audio_out_ = new QAudioOutput(format, this);
  audio_out_->setBufferSize(44100 * 2 * 2);
  audio_device_ = audio_out_->start();
  if (audio_device_ == NULL) {
    StopPipelines();
    if (error != NULL) {
      *error = QString::fromUtf8("无法打开音频输出");
    }
    player_widget_->setLoadingCoverVisible(false);
    player_widget_->showHintPage();
    return false;
  }

  pending_title_ = title;
  paused_ = false;
  playing_ = true;
  frame_timer_->start();
  emit statusMessage(QString::fromUtf8("正在播放：") + title);
  return true;
}

bool PlaybackController::PlayWithSystemPlayer(const QUrl& media_url,
                                              const QString& title,
                                              QString* error) {
  if (!QDesktopServices::openUrl(media_url)) {
    if (error != NULL) {
      *error = QString::fromUtf8("无法调用系统播放器");
    }
    return false;
  }
  if (player_widget_ != NULL) {
    player_widget_->showHintPage();
  }
  UpdateHint(QString::fromUtf8("已用系统播放器打开：\n") + title);
  emit statusMessage(QString::fromUtf8("已用系统播放器打开：") + title);
  return true;
}

void PlaybackController::play(const Video& video) {
  const QString url_text = video.play_url.trimmed();
  if (url_text.isEmpty()) {
    emit errorOccurred(QString::fromUtf8("播放地址为空"));
    return;
  }
  const QUrl media_url = ResolvePlayUrl(url_text);
  if (!media_url.isValid()) {
    emit errorOccurred(QString::fromUtf8("播放地址无效"));
    return;
  }
  if (media_url.isLocalFile() && !QFileInfo::exists(media_url.toLocalFile())) {
    emit errorOccurred(QString::fromUtf8("本地视频文件不存在：") +
                       media_url.toLocalFile());
    return;
  }

  if (player_widget_ != NULL) {
    player_widget_->setLoadingCoverVisible(true);
  }

  QString error;
  if (StartPipelines(media_url, video.title, &error)) {
    return;
  }
  if (player_widget_ != NULL) {
    player_widget_->setLoadingCoverVisible(false);
  }
  if (PlayWithSystemPlayer(media_url, video.title, &error)) {
    return;
  }
  emit errorOccurred(
      QString::fromUtf8("播放失败：") +
      (error.isEmpty()
           ? QString::fromUtf8(
                 "请确认 third_party/ffmpeg/bin/ffmpeg.exe 可用。")
           : error));
}

void PlaybackController::pause() {
  if (!playing_) {
    emit statusMessage(QString::fromUtf8("当前没有播放中的视频"));
    return;
  }

  if (!paused_) {
    // Freeze ffmpeg in place (no HWND involved, so no UI deadlock).
    frame_timer_->stop();
    if (audio_out_ != NULL) {
      audio_out_->suspend();
    }
    const bool video_ok = SetProcessSuspended(video_proc_, true);
    const bool audio_ok = SetProcessSuspended(audio_proc_, true);
    if (!video_ok && !audio_ok) {
      if (audio_out_ != NULL) {
        audio_out_->resume();
      }
      frame_timer_->start();
      emit statusMessage(QString::fromUtf8("暂停失败，请重试"));
      return;
    }
    paused_ = true;
    emit statusMessage(QString::fromUtf8("已暂停"));
    return;
  }

  SetProcessSuspended(video_proc_, false);
  SetProcessSuspended(audio_proc_, false);
  paused_ = false;
  if (audio_out_ != NULL) {
    audio_out_->resume();
  }
  frame_timer_->start();
  emit statusMessage(QString::fromUtf8("继续播放：") + pending_title_);
}

void PlaybackController::stop() {
  StopPipelines();
  if (player_widget_ != NULL) {
    player_widget_->setLoadingCoverVisible(false);
    player_widget_->showHintPage();
  }
  UpdateHint(QString::fromUtf8("选择左侧视频后点击「播放」\n画面将在此区域内播放"));
  emit statusMessage(QString::fromUtf8("已停止"));
}

void PlaybackController::onVideoReadyRead() {
  if (!playing_ || paused_) {
    return;
  }
  video_buffer_.append(video_proc_->readAllStandardOutput());
  while (video_buffer_.size() >= frame_bytes_) {
    const QImage frame(
        reinterpret_cast<const uchar*>(video_buffer_.constData()), frame_w_,
        frame_h_, frame_w_ * 3, QImage::Format_RGB888);
    const QImage owned = frame.copy();
    video_buffer_.remove(0, frame_bytes_);
    if (frame_queue_.size() >= kMaxQueuedFrames) {
      frame_queue_.removeFirst();
    }
    frame_queue_.append(owned);
  }
}

void PlaybackController::onFrameTick() {
  if (paused_ || !playing_) {
    return;
  }
  if (frame_queue_.isEmpty()) {
    if (video_proc_->state() == QProcess::NotRunning) {
      playing_ = false;
      frame_timer_->stop();
      if (player_widget_ != NULL) {
        player_widget_->setLoadingCoverVisible(false);
        player_widget_->showHintPage();
      }
      UpdateHint(QString::fromUtf8("播放结束。可重新选择视频播放。"));
      emit statusMessage(QString::fromUtf8("播放结束"));
      if (audio_proc_->state() != QProcess::NotRunning) {
        audio_proc_->kill();
      }
      if (audio_out_ != NULL) {
        audio_out_->stop();
      }
    }
    return;
  }

  const QImage frame = frame_queue_.takeFirst();
  if (player_widget_ != NULL) {
    // Paint first, then remove cover — avoids a blank/frame flash.
    player_widget_->setFrame(frame);
    if (!has_presented_frame_) {
      has_presented_frame_ = true;
      player_widget_->setLoadingCoverVisible(false);
    }
  }
}

void PlaybackController::onAudioReadyRead() {
  if (paused_ || !playing_ || audio_device_ == NULL) {
    return;
  }
  const QByteArray chunk = audio_proc_->readAllStandardOutput();
  if (!chunk.isEmpty()) {
    audio_device_->write(chunk);
  }
}

void PlaybackController::onVideoFinished(int /*exit_code*/,
                                         QProcess::ExitStatus /*status*/) {
}

void PlaybackController::onAudioFinished(int /*exit_code*/,
                                         QProcess::ExitStatus /*status*/) {
}

}  // namespace client
}  // namespace square_dance
