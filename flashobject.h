#pragma once

#include <QMap>
#include <QUuid>
#include "flashclass.h"

struct FlashObject
{
  int                       class_idx = -1;
  QMap<QString, QByteArray> attributes;
  QVector<FlashGeoPolygon>  polygons;
  int                       inner_polygon_start_idx = -1;
  FlashGeoRect              frame;

public:
  void save(const QVector<FlashClass>& class_list,
            QByteArray&                ba) const;
  void load(QVector<FlashClass>& class_list, int& pos,
            const QByteArray& ba);
  bool isEmpty() const;
};

typedef QPair<FlashObject, FlashClass> FreeObject;
