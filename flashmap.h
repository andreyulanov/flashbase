#pragma once

#include <QPolygon>
#include <QColor>
#include <QPainter>
#include <QReadWriteLock>
#include <QMap>
#include <QElapsedTimer>
#include <QVariant>
#include "flashobject.h"

class FlashMap
{
public:
  enum CompressionPolicy : uchar
  {
    CompressionOff,
    CompressionOn
  };
  struct ObjectAddress
  {
    int  tile_idx = -1;
    int  obj_idx  = -1;
    bool isValid() const;
  };
  struct VectorTile: public QVector<FlashObject>
  {
    enum Status
    {
      Null,
      Loading,
      Loaded
    };
    Status status = Null;
  };
  struct Settings
  {
    double            main_mip             = 0;
    double            tile_mip             = 0;
    int               max_objects_per_tile = 0;
    CompressionPolicy compression_policy   = CompressionOn;
  };

private:
  static constexpr int border_coor_precision_coef = 10000;

  FlashGeoRect frame;
  Settings     settings;

protected:
  QVector<FlashClass>      classes;
  QVector<FlashGeoPolygon> borders;
  QVector<QPolygonF>       borders_m;
  VectorTile               main;
  QVector<VectorTile>      tiles;

public:
  FlashMap(const QString& path);
  FlashMap(const QString& path, Settings);
  void   save() const;
  void   save(const QString&) const;
  void   loadMainVectorTile(bool load_objects);
  void   loadVectorTile(int tile_idx);
  void   loadAll();
  void   clear();
  qint64 count() const;
  void   addMap(const FlashMap&);

  QVector<FlashObject> getLoadedObjects() const;
  VectorTile           getMainTile() const;
  QVector<VectorTile>  getLocalTiles() const;

  FreeObject getObject(const ObjectAddress& addr) const;
  void       setObject(const ObjectAddress& addr, const FreeObject&);
  void       setObject(const ObjectAddress& addr, const FlashObject&);
  ObjectAddress addObject(const FlashObject& obj);
  ObjectAddress addObject(const FreeObject& obj);
  void          addObjects(const QVector<FlashObject>&,
                           const QVector<FlashClass>&);
  void          setBorders(const QVector<FlashGeoPolygon>&);

  double getMainMip() const;
  double getTileMip() const;

  const FlashClass& getClass(int idx) const;
  FlashClass        getClassById(QString id) const;
  QVariant          modifyClass(FlashClass);
  void              setClass(int idx, const FlashClass&);
  int               getClassCount() const;
  void              setClasses(QVector<FlashClass>);

  FlashGeoRect getFrame() const;

  VectorTile::Status getMainTileStatus() const;
  VectorTile::Status getTileStatus(int tile_idx) const;
  int                getTileCount() const;

  QString path;
};
