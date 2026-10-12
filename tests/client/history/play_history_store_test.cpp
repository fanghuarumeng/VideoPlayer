#include "video_player/client/history/PlayHistoryStore.h"

#include <QFile>
#include <gtest/gtest.h>

namespace {

video_player::client::Video MakeVideo(qint64 id, const char* title) {
  video_player::client::Video v;
  v.id = id;
  v.title = QString::fromUtf8(title);
  v.play_url = QString::fromUtf8("http://127.0.0.1/%1.mp4").arg(id);
  v.thumbnail_url = QString::fromUtf8("http://127.0.0.1/%1.jpg").arg(id);
  return v;
}

}  // namespace

class PlayHistoryStoreTest : public ::testing::Test {
 protected:
  void SetUp() override {
    path_ = QString::fromUtf8("play_history_store_test.json");
    QFile::remove(path_);
  }
  void TearDown() override { QFile::remove(path_); }
  QString path_;
};

TEST_F(PlayHistoryStoreTest, UpsertMovesToFrontAndCaps) {
  video_player::client::PlayHistoryStore store(path_, 3);
  store.recordProgress(MakeVideo(1, "A"), 1000, 10000);
  store.recordProgress(MakeVideo(2, "B"), 2000, 10000);
  store.recordProgress(MakeVideo(3, "C"), 3000, 10000);
  store.recordProgress(MakeVideo(4, "D"), 4000, 10000);
  ASSERT_EQ(store.items().size(), 3);
  EXPECT_EQ(store.items()[0].id, 4);
  EXPECT_EQ(store.items()[2].id, 2);

  store.recordProgress(MakeVideo(2, "B2"), 2500, 10000);
  EXPECT_EQ(store.items()[0].id, 2);
  EXPECT_EQ(store.items()[0].title, QString::fromUtf8("B2"));
  EXPECT_EQ(store.items()[0].position_ms, 2500);
}

TEST_F(PlayHistoryStoreTest, RoundTripAndResumeNearEnd) {
  {
    video_player::client::PlayHistoryStore store(path_, 100);
    store.recordProgress(MakeVideo(9, "X"), 5000, 10000);
  }
  video_player::client::PlayHistoryStore loaded(path_, 100);
  QString error;
  ASSERT_TRUE(loaded.load(&error)) << error.toStdString();
  ASSERT_EQ(loaded.items().size(), 1);
  EXPECT_EQ(loaded.items()[0].position_ms, 5000);

  video_player::client::PlayHistoryItem near_end = loaded.items()[0];
  near_end.position_ms = 9500;
  near_end.duration_ms = 10000;
  EXPECT_EQ(video_player::client::PlayHistoryStore::ResumePositionMs(near_end),
            0);

  loaded.clear();
  EXPECT_TRUE(loaded.items().isEmpty());
}
