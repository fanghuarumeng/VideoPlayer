#include "square_dance/server/local_media/UploadGate.h"

#include <gtest/gtest.h>

TEST(UploadGate, CapsConcurrentSlots) {
  square_dance::server::UploadGate gate(1);
  EXPECT_TRUE(gate.tryAcquire());
  EXPECT_FALSE(gate.tryAcquire());
  EXPECT_EQ(gate.in_use(), 1);
  gate.release();
  EXPECT_EQ(gate.in_use(), 0);
  EXPECT_TRUE(gate.tryAcquire());
  gate.release();
}

TEST(UploadGate, AllowsUpToMax) {
  square_dance::server::UploadGate gate(2);
  EXPECT_TRUE(gate.tryAcquire());
  EXPECT_TRUE(gate.tryAcquire());
  EXPECT_FALSE(gate.tryAcquire());
  gate.release();
  EXPECT_TRUE(gate.tryAcquire());
  gate.release();
  gate.release();
}
