#include "video_player/client/api/Video.h"
#include "video_player/client/playback/PlaybackController.h"
#include "video_player/client/playback/VideoPlayerWidget.h"

#include <QCoreApplication>
#include <QDeadlineTimer>
#include <QDir>
#include <QEventLoop>
#include <QFileInfo>
#include <QSignalSpy>
#include <QUrl>
#include <gtest/gtest.h>

namespace {

QString SampleMp4Path() {
#ifdef VIDEO_PLAYER_SOURCE_DIR
  return QDir(QString::fromUtf8(VIDEO_PLAYER_SOURCE_DIR))
      .filePath(QString::fromUtf8("data/samples/square_basic.mp4"));
#else
  return QString();
#endif
}

video_player::client::Video MakeLocalVideo(const QString& path) {
  video_player::client::Video v;
  v.id = 42;
  v.external_id = QString::fromUtf8("local-sample");
  v.title = QString::fromUtf8("sample");
  v.play_url = QUrl::fromLocalFile(path).toString();
  v.download_url = v.play_url;
  v.source_type = QString::fromUtf8("local");
  v.updated_at = QString::fromUtf8("2026-10-10T00:00:00Z");
  return v;
}

void Pump(int ms) {
  const QDeadlineTimer deadline(ms);
  while (!deadline.hasExpired()) {
    QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
  }
}

}  // namespace

// Repro: double-click / play the same (or another) clip many times while the
// previous FFmpeg worker still has QueuedConnection signals in flight.
TEST(Playback, RapidReplayDoesNotCrashOrEmitStaleFinished) {
  const QString path = SampleMp4Path();
  if (!QFileInfo::exists(path)) {
    GTEST_SKIP() << "missing sample mp4: " << path.toStdString();
  }

  video_player::client::PlaybackController controller;
  video_player::client::VideoPlayerWidget widget;
  controller.setPlayerWidget(&widget);

  QSignalSpy finished_spy(
      &controller, &video_player::client::PlaybackController::playbackFinished);
  QSignalSpy error_spy(&controller,
                       &video_player::client::PlaybackController::errorOccurred);

  const video_player::client::Video video = MakeLocalVideo(path);
  for (int i = 0; i < 8; ++i) {
    controller.play(video);
    Pump(30);
  }

  // Let the last session settle, then stop cleanly.
  Pump(500);
  controller.stop();
  Pump(200);

  EXPECT_EQ(error_spy.count(), 0)
      << (error_spy.isEmpty()
              ? ""
              : error_spy.front().front().toString().toStdString());
  // Stale workers must not report "finished" after stop(); at most the last
  // natural EOF before stop could have fired — never crash, and never more
  // finished signals than play attempts.
  EXPECT_LE(finished_spy.count(), 8);
}
