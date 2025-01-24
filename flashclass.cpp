#include "flashclass.h"
#include "flashserialize.h"
#include <math.h>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QMetaEnum>

void FlashClass::save(QFile* f) const
{
  using namespace FlashSerialize;
  write(f, id);
  write(f, type);
  write(f, style);
  write(f, layer);
  write(f, width_mm);
  write(f, min_mip);
  write(f, max_mip);
  write(f, coor_precision_coef);
  write(f, (uchar)pen.red());
  write(f, (uchar)pen.green());
  write(f, (uchar)pen.blue());
  write(f, (uchar)pen.alpha());
  write(f, (uchar)brush.red());
  write(f, (uchar)brush.green());
  write(f, (uchar)brush.blue());
  write(f, (uchar)brush.alpha());
  write(f, (uchar)text.red());
  write(f, (uchar)text.green());
  write(f, (uchar)text.blue());
  write(f, (uchar)text.alpha());
  write(f, image);
  write(f, attributes);
}

void FlashClass::load(QFile* f)
{
  using namespace FlashSerialize;
  read(f, id);
  read(f, type);
  read(f, style);
  read(f, layer);
  read(f, width_mm);
  read(f, min_mip);
  read(f, max_mip);
  read(f, coor_precision_coef);
  uchar red, green, blue, alpha;
  read(f, red);
  read(f, green);
  read(f, blue);
  read(f, alpha);
  pen = QColor(red, green, blue, alpha);
  read(f, red);
  read(f, green);
  read(f, blue);
  read(f, alpha);
  brush = QColor(red, green, blue, alpha);
  read(f, red);
  read(f, green);
  read(f, blue);
  read(f, alpha);
  text = QColor(red, green, blue, alpha);
  QImage img;
  read(f, image);
  read(f, attributes);
}
