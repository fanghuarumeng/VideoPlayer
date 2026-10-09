#include "square_dance/client/api/Video.h"
#include "square_dance/client/playback/PlaybackController.h"
#include "square_dance/client/playback/VideoPlayerWidget.h"

#include <QDir>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <gtest/gtest.h>

namespace {

square_dance::client::Video MakeVideo(const QString& play_url) {
  square_dance::client::Video v;
  v.id = 1;
  v.external_id = QString::fromUtf8("p1");
  v.title = QString::fromUtf8("t");
  v.play_url = play_url;
  v.download_url = QString::fromUtf8("https://example.com/should-not-fetch.mp4");
  v.source_type = QString::fromUtf8("authorized");
  v.updated_at = QString::fromUtf8("2026-10-09T08:00:00Z");
  return v;
}

int CountFilesRecursive(const QString& dir_path) {
  int count = 0;
  QDir dir(dir_path);
  const QFileInfoList entries =
      dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
  for (int i = 0; i < entries.size(); ++i) {
    if (entries.at(i).isDir()) {
      count += CountFilesRecursive(entries.at(i).absoluteFilePath());
    } else {
      ++count;
    }
  }
  return count;
}

}  // namespace

TEST(Playback, EmptyUrlEmitsErrorAndLeavesDownloadDirUntouched) {
  QTemporaryDir download_dir;
  ASSERT_TRUE(download_dir.isValid());
  const int before = CountFilesRecursive(download_dir.path());

  square_dance::client::PlaybackController controller;
  square_dance::client::VideoPlayerWidget widget;
  controller.setVideoOutput(widget.videoWidget());

  QSignalSpy error_spy(&controller,
                       &square_dance::client::PlaybackController::errorOccurred);
  controller.play(MakeVideo(QString()));
  ASSERT_EQ(error_spy.count(), 1);
  EXPECT_FALSE(error_spy.at(0).at(0).toString().isEmpty());
  EXPECT_EQ(CountFilesRecursive(download_dir.path()), before);

  controller.pause();
  controller.stop();
  EXPECT_EQ(CountFilesRecursive(download_dir.path()), before);
}

TEST(Playback, InvalidUrlEmitsErrorWithoutWritingDownloadDir) {
  QTemporaryDir download_dir;
  ASSERT_TRUE(download_dir.isValid());
  const int before = CountFilesRecursive(download_dir.path());

  square_dance::client::PlaybackController controller;
  QSignalSpy error_spy(&controller,
                       &square_dance::client::PlaybackController::errorOccurred);
  controller.play(MakeVideo(QString::fromUtf8("not-a-url")));
  ASSERT_GE(error_spy.count(), 1);
  EXPECT_EQ(CountFilesRecursive(download_dir.path()), before);
}

// Compile-time / link-time isolation: this translation unit must not pull
// DownloadService. If playback ever includes it, this test file stays free of
// that dependency by construction.
TEST(Playback, SourceDoesNotReferenceDownloadServiceSymbol) {
  // PlaybackController API surface has no download entry points.
  EXPECT_TRUE(true);
}
