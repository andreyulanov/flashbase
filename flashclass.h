#pragma once

#include "flashbase.h"
#include <QMap>

struct FlashClass
{
  Q_GADGET
public:
  static constexpr int max_layer_count = 256;
  enum Type : uchar
  {
    None,
    Point,
    Line,
    Area
  };
  Q_ENUM(Type)
  enum Style : uchar
  {
    Solid,
    Dash,
    DashDot,
    BDiag,
    FDiag,
    Horiz,
    Vert,
    Dots,
    Custom
  };
  Q_ENUM(Style)
  QString                id;
  Type                   type                = None;
  Style                  style               = Solid;
  uchar                  layer               = 0;
  float                  width_mm            = 0.2;
  float                  min_mip             = 0;
  float                  max_mip             = 0;
  int                    coor_precision_coef = 1;
  QColor                 pen;
  QColor                 brush;
  QColor                 text;
  QImage                 image;
  QMap<QString, QString> attributes;

  QMap<QString, QString> detect_tags;

  void save(QFile* f) const;
  void load(QFile* f);
};

struct FlashClassImage
{
  QString id;
  QImage  image;
};

typedef QVector<FlashClassImage> FlashClassImageList;
