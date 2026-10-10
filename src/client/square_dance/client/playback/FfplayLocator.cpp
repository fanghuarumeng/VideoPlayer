#include "square_dance/client/playback/FfplayLocator.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>

namespace square_dance {
namespace client {
namespace {

bool IsRunnableTool(const QString& path) {
  if (path.isEmpty() || !QFileInfo::exists(path)) {
    return false;
  }
  QProcess probe;
  probe.start(path, QStringList() << QString::fromUtf8("-version"));
  if (!probe.waitForFinished(3000)) {
    probe.kill();
    return false;
  }
  return probe.exitStatus() == QProcess::NormalExit;
}

QString CandidateNextToApp(const QString& exe_name) {
  const QString app_dir = QCoreApplication::applicationDirPath();
  const QStringList relatives =
      QStringList()
      << (QString::fromUtf8("ffmpeg/bin/") + exe_name)
      << (QString::fromUtf8("../ffmpeg/bin/") + exe_name)
      << (QString::fromUtf8("../../third_party/ffmpeg/bin/") + exe_name)
      << (QString::fromUtf8("../../../third_party/ffmpeg/bin/") + exe_name);
  for (int i = 0; i < relatives.size(); ++i) {
    const QString path = QDir(app_dir).absoluteFilePath(relatives.at(i));
    if (IsRunnableTool(path)) {
      return QFileInfo(path).absoluteFilePath();
    }
  }
  return QString();
}

QString FindTool(const char* env_name,
                 const QString& exe_name,
                 const QString& path_name) {
  const QByteArray env = qgetenv(env_name);
  if (!env.isEmpty()) {
    const QString from_env = QString::fromLocal8Bit(env);
    if (IsRunnableTool(from_env)) {
      return QFileInfo(from_env).absoluteFilePath();
    }
  }

  const QString beside_app = CandidateNextToApp(exe_name);
  if (!beside_app.isEmpty()) {
    return beside_app;
  }

#ifdef SQUARE_DANCE_SOURCE_DIR
  const QString from_source =
      QString::fromUtf8(SQUARE_DANCE_SOURCE_DIR) +
      QString::fromUtf8("/third_party/ffmpeg/bin/") + exe_name;
  if (IsRunnableTool(from_source)) {
    return QFileInfo(from_source).absoluteFilePath();
  }
#endif

  const QString from_path = QStandardPaths::findExecutable(path_name);
  if (IsRunnableTool(from_path)) {
    return from_path;
  }
  return QString();
}

}  // namespace

QString FindFfplayExecutable() {
  return FindTool("SQUARE_DANCE_FFPLAY",
                  QString::fromUtf8("ffplay.exe"),
                  QString::fromUtf8("ffplay"));
}

QString FindFfmpegExecutable() {
  return FindTool("SQUARE_DANCE_FFMPEG",
                  QString::fromUtf8("ffmpeg.exe"),
                  QString::fromUtf8("ffmpeg"));
}

}  // namespace client
}  // namespace square_dance
