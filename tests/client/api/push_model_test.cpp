#include "square_dance/client/api/CatalogModel.h"

#include <gtest/gtest.h>

namespace {

square_dance::client::Video MakeVideo(qint64 id, const char* title) {
  square_dance::client::Video v;
  v.id = id;
  v.external_id = QString::fromUtf8("ext-") + QString::number(id);
  v.title = QString::fromUtf8(title);
  v.play_url = QString::fromUtf8("https://example.com/x.mp4");
  v.download_url = v.play_url;
  v.source_type = QString::fromUtf8("authorized");
  v.updated_at = QString::fromUtf8("2026-10-09T08:00:00Z");
  return v;
}

}  // namespace

TEST(CatalogModelPush, EmptyFilterPrependsNew) {
  square_dance::client::CatalogModel model;
  model.setFilterQuery(QString());
  QVector<square_dance::client::Video> seed;
  seed.push_back(MakeVideo(1, "old"));
  model.resetFromSearch(seed);

  model.applyUpserted(MakeVideo(2, "new"));
  ASSERT_EQ(model.rowCount(), 2);
  EXPECT_EQ(model.videoAt(0).id, 2);
  EXPECT_EQ(model.videoAt(1).id, 1);
}

TEST(CatalogModelPush, EmptyFilterUpdatesExisting) {
  square_dance::client::CatalogModel model;
  QVector<square_dance::client::Video> seed;
  seed.push_back(MakeVideo(1, "old"));
  model.resetFromSearch(seed);

  model.applyUpserted(MakeVideo(1, "updated"));
  ASSERT_EQ(model.rowCount(), 1);
  EXPECT_EQ(model.videoAt(0).title, QString::fromUtf8("updated"));
}

TEST(CatalogModelPush, NonEmptyFilterIgnoresNonMatching) {
  square_dance::client::CatalogModel model;
  model.setFilterQuery(QString::fromUtf8("广场舞"));
  model.applyUpserted(MakeVideo(9, "square dance only"));
  EXPECT_EQ(model.rowCount(), 0);
}

TEST(CatalogModelPush, NonEmptyFilterKeepsMatchingAndRemovesMismatchUpdate) {
  square_dance::client::CatalogModel model;
  model.setFilterQuery(QString::fromUtf8("广场舞"));
  model.applyUpserted(MakeVideo(1, "欢乐广场舞"));
  ASSERT_EQ(model.rowCount(), 1);

  model.applyUpserted(MakeVideo(1, "english only"));
  EXPECT_EQ(model.rowCount(), 0);
}

TEST(CatalogModelPush, ApplyRemovedDropsRow) {
  square_dance::client::CatalogModel model;
  QVector<square_dance::client::Video> seed;
  seed.push_back(MakeVideo(5, "gone"));
  model.resetFromSearch(seed);
  model.applyRemoved(5);
  EXPECT_EQ(model.rowCount(), 0);
  model.applyRemoved(5);
  EXPECT_EQ(model.rowCount(), 0);
}
