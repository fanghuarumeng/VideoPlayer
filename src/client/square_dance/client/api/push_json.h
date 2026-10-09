#pragma once

#include "square_dance/client/api/Video.h"

#include <QByteArray>
#include <QString>

namespace square_dance {
namespace client {

struct PushMessage {
  enum Kind {
    kIgnored = 0,
    kUpserted,
    kRemoved
  };

  Kind kind;
  Video video;
  qint64 removed_id;

  PushMessage() : kind(kIgnored), removed_id(0) {}
};

// Parses one WS text frame. Unknown type → kind=kIgnored (success).
// Malformed JSON / missing fields → false.
bool ParsePushMessage(const QByteArray& json, PushMessage* out, QString* error);

}  // namespace client
}  // namespace square_dance
