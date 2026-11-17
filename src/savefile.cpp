#include "savefile.h"

#include "klondike.h"
#include "spider.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <memory>

namespace SaveFile {

Result read(const QString &path, std::unique_ptr<Solitaire::Game> *game, QJsonObject *data, QString *problem) {
  if (!QFile::exists(path)) return Result::Missing;
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    if (problem) *problem = file.errorString();
    return Result::Unreadable;
  }
  if (file.size() > MaxBytes) return Result::TooLarge;
  QJsonParseError error;
  const QJsonObject saved = QJsonDocument::fromJson(file.readAll(), &error).object();
  const qint64 seed = saved["seed"].toInteger();
  if (error.error != QJsonParseError::NoError || saved["version"].toInt() != 1 || !saved["moves"].isArray()
      || saved["moves"].toArray().size() > MaxMoves || seed <= 0 || seed > 0xffffffffll)
    return Result::Damaged;
  const int kind = saved["game"].toInt(0);
  const int suits = saved["suits"].toInt(4);
  // Only the games the window deals, and only Spider's suit counts, read back.
  if ((kind != 0 && kind != 1) || (kind == 1 && suits != 1 && suits != 2 && suits != 4)) return Result::Damaged;
  // The game the save names, dealt from its seed, walking the moves back on.
  std::unique_ptr<Solitaire::Game> replayed;
  if (kind == 1) replayed = std::make_unique<Spider::Game>(quint32(seed), suits);
  else replayed = std::make_unique<Klondike::Game>(quint32(seed));
  for (const auto value : saved["moves"].toArray()) {
    const QJsonArray move = value.toArray();
    // A won game takes no more moves, so a save that goes on past one is not ours.
    if (move.size() != 3 || replayed->won() || !replayed->play(move[0].toInt(-1), move[1].toInt(-1), move[2].toInt(-1))) return Result::Damaged;
  }
  *game = std::move(replayed);
  if (data) *data = saved;
  return Result::Ok;
}

}
