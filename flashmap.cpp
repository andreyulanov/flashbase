#include "math.h"
#include "flashmap.h"
#include "flashserialize.h"
#include <QDebug>
#include <QElapsedTimer>
#include <QDateTime>
#include <QRegularExpression>

bool FlashMap::ObjectAddress::isValid() const
{
  return (tile_idx >= 0 && obj_idx >= 0);
}

FlashMap::FlashMap(const QString& path)
{
    this->path = path;
}

FlashMap::FlashMap(const QString& path, Settings v)
{
  this->path = path;
  settings = v;
}

void FlashMap::clear()
{
  if (main.status != VectorTile::Loaded)
    return;
  if (main.status == VectorTile::Loading)
    return;

  main.clear();
  tiles.clear();
  classes.clear();
  main.status = VectorTile::Null;
}

qint64 FlashMap::count() const
{
  qint64 total_count = main.count();
  for (auto t: tiles)
    total_count += t.count();
  return total_count;
}

void FlashMap::save() const
{
    save(path);
}

void FlashMap::save(const QString& path) const
{
  using namespace FlashSerialize;

  QFile f(path);
  if (!f.open(QIODevice::WriteOnly))
  {
    qDebug() << "write error:" << path;
    return;
  }

  write(&f, QString("flashmap"));
  write(&f, settings.compression_policy);
  char has_borders = (borders.count() > 0);
  write(&f, has_borders);

  if (has_borders)
  {
    QByteArray ba;
    write(ba, borders.count());
    for (auto border: borders)
      border.save(ba, border_coor_precision_coef);
    if (settings.compression_policy == CompressionOn)
      ba = qCompress(ba, 9);
    write(&f, ba.count());
    f.write(ba.data(), ba.count());
  }

  write(&f, settings.main_mip);
  write(&f, settings.tile_mip);
  write(&f, classes.count());
  for (auto cl: classes)
    cl.save(&f);

  write(&f, main.count());

  QByteArray ba;
  for (auto& obj: main)
  {
    if (!obj.isEmpty())
      obj.save(classes, ba);
  }
  if (settings.compression_policy == CompressionOn)
    ba = qCompress(ba, 9);
  write(&f, ba.count());
  f.write(ba.data(), ba.count());
  write(&f, tiles.count());
  QList<qint64> small_part_pos_list;

  for (int part_idx = 0; auto& tile: tiles)
  {
    small_part_pos_list.append(f.pos());
    if (tile.count() > 0)
    {
      write(&f, tile.count());
      ba.clear();
      for (auto& obj: tile)
        obj.save(classes, ba);
      if (settings.compression_policy == CompressionOn)
        ba = qCompress(ba, 9);
      write(&f, ba.count());
      f.write(ba.data(), ba.count());
    }
    else
      write(&f, 0);
    part_idx++;
  }
  auto small_idx_start_pos = f.pos();
  for (auto& pos: small_part_pos_list)
    write(&f, pos);
  write(&f, small_idx_start_pos);
}

void FlashMap::loadMainVectorTile(bool load_objects)
{
  if (main.status == VectorTile::Loading)
    return;
  QElapsedTimer t;
  t.start();

  using namespace FlashSerialize;
  QFile f(path);
  if (!f.open(QIODevice::ReadOnly))
  {
    qDebug() << "read error:" << path;
    return;
  }

  if (main.status != VectorTile::Null)
    return;

  QString format_id;
  read(&f, format_id);

  read(&f, settings.compression_policy);

  char has_borders = false;
  read(&f, has_borders);
  if (has_borders)
  {
    QByteArray ba;
    int        ba_count = 0;
    read(&f, ba_count);
    ba.resize(ba_count);
    f.read(ba.data(), ba_count);
    if (settings.compression_policy == CompressionOn)
      ba = qUncompress(ba);
    int pos = 0;
    int borders_count;
    read(ba, pos, borders_count);
    borders.resize(borders_count);

    for (int idx = -1; auto& border: borders)
    {
      idx++;
      border.load(ba, pos, border_coor_precision_coef);
      if (idx == 0)
        frame = border.getFrame();
      else
        frame = frame.united(border.getFrame());
    }

    borders_m.clear();
    for (auto border: borders)
    {
      QPolygonF border_m = border.toPolygonM();
      if (!border_m.isEmpty())
        borders_m.append(border_m);
    }
  }

  read(&f, settings.main_mip);
  read(&f, settings.tile_mip);

  if (!load_objects)
    return;

  qDebug() << "loading main from" << path;
  main.status = VectorTile::Loading;
  int class_count;
  read(&f, class_count);
  qDebug() << "class_count" << class_count;
  for (int i = 0; i < class_count; i++)
  {
    FlashClass cl;
    cl.load(&f);
    classes.append(cl);
  }

  int pos = 0;

  int big_obj_count;
  read(&f, big_obj_count);
  main.resize(big_obj_count);

  QByteArray ba;
  int        ba_count = 0;
  read(&f, ba_count);
  ba.resize(ba_count);
  f.read(ba.data(), ba_count);
  if (settings.compression_policy == CompressionOn)
    ba = qUncompress(ba);
  pos = 0;
  for (auto& obj: main)
    obj.load(classes, pos, ba);

  int small_count;
  read(&f, small_count);
  tiles.resize(small_count);
}

void FlashMap::loadAll()
{
  loadMainVectorTile(true);
  main.status = VectorTile::Loaded;
  for (int i = 0; i < tiles.count(); i++)
  {
    loadVectorTile(i);
    tiles[i].status = VectorTile::Loaded;
  }
}

void FlashMap::loadVectorTile(int tile_idx)
{
  if (main.status != VectorTile::Loaded)
    return;
  if (tiles[tile_idx].status == VectorTile::Loading)
    return;

  qDebug() << "loading tile" << tile_idx << "from" << path;
  QElapsedTimer t;
  t.start();
  using namespace FlashSerialize;
  QFile f(path);
  if (!f.open(QIODevice::ReadOnly))
  {
    qDebug() << "read error:" << path;
    return;
  }

  qint64 small_idx_start_pos;
  f.seek(f.size() - sizeof(qint64));
  read(&f, small_idx_start_pos);

  int calc_small_part_count =
      (f.size() - sizeof(qint64) - small_idx_start_pos) /
      sizeof(qint64);
  if (calc_small_part_count != tiles.count())
    return;

  if (tile_idx > tiles.count() - 1)
    return;

  f.seek(small_idx_start_pos + tile_idx * sizeof(qint64));
  qint64 part_pos;
  read(&f, part_pos);
  f.seek(part_pos);

  int part_obj_count;
  read(&f, part_obj_count);

  if (part_obj_count == 0)
    return;

  tiles[tile_idx].resize(part_obj_count);

  int ba_count = 0;
  read(&f, ba_count);
  QByteArray ba;
  ba.resize(ba_count);
  f.read(ba.data(), ba_count);
  if (settings.compression_policy == CompressionOn)
    ba = qUncompress(ba);

  tiles[tile_idx].status = VectorTile::Loading;
  int pos                = 0;
  for (auto& obj: tiles[tile_idx])
    obj.load(classes, pos, ba);
}

FreeObject FlashMap::getObject(const ObjectAddress& addr) const
{
  if (addr.isValid())
  {
    if (addr.tile_idx == 0)
    {
      auto& obj = main[addr.obj_idx];
      return {obj, classes.at(obj.class_idx)};
    }
    else
    {
      auto& obj = tiles[addr.tile_idx - 1][addr.obj_idx];
      return {obj, classes.at(obj.class_idx)};
    }
  }
  else
    return FreeObject();
}

FlashMap::VectorTile FlashMap::getMainTile() const
{
  return main;
}

QVector<FlashMap::VectorTile> FlashMap::getLocalTiles() const
{
  return tiles;
}

QVector<FlashObject> FlashMap::getLoadedObjects() const
{
  auto objects = main;
  for (auto& tile: tiles)
    objects.append(tile);
  return objects;
}

void FlashMap::addMap(const FlashMap& map)
{
  auto objects = map.getLoadedObjects();
  for (auto& obj: objects)
    addObject(obj);
}

void FlashMap::setObject(const ObjectAddress& addr,
                         const FreeObject&    free_obj)
{
  auto obj = free_obj.first;
  auto cl  = getClassById(free_obj.second.id);
  if (cl.id.isEmpty())
  {
    classes.append(free_obj.second);
    obj.class_idx = classes.count() - 1;
  }
  setObject(addr, obj);
}

FlashMap::ObjectAddress
FlashMap::addObject(const FreeObject& free_obj)
{
  auto obj = free_obj.first;
  auto cl  = getClassById(free_obj.second.id);
  if (cl.id.isEmpty())
  {
    classes.append(free_obj.second);
    obj.class_idx = classes.count() - 1;
  }
  return addObject(obj);
}

void FlashMap::setBorders(const QVector<FlashGeoPolygon>& v)
{
  borders = v;
}

void FlashMap::addObjects(const QVector<FlashObject>& _objects,
                          const QVector<FlashClass>&  _classes)
{
  classes = _classes;
  for (int idx = -1; auto& border: borders)
  {
    idx++;
    if (idx == 0)
      frame = border.getFrame();
    else
      frame = frame.united(border.getFrame());
  }

  if (settings.main_mip == 0 && settings.tile_mip == 0)
  {
    main.append(_objects);
    return;
  }

  int tile_side_num = std::ceil(1.0 * _objects.count() /
                                settings.max_objects_per_tile);
  int tile_count    = pow(tile_side_num, 2);
  tiles.resize(tile_count);
  auto map_size_m     = frame.getSizeMeters();
  auto map_top_left_m = frame.top_left.toMeters();
  for (auto& src_obj: _objects)
  {
    FlashObject obj(src_obj);
    if (obj.polygons.isEmpty())
    {
      qDebug() << "No geometry defined for point object"
               << obj.attributes.value("name");
      FlashGeoPolygon empty_polygon;
      empty_polygon.append(FlashGeoCoor());
      obj.polygons.append(empty_polygon);
    }

    auto cl = classes.at(obj.class_idx);

    if (cl.max_mip == 0 || cl.max_mip > settings.tile_mip)
      main.append(obj);
    else
    {
      auto   obj_top_left_m = obj.frame.top_left.toMeters();
      double shift_x_m      = obj_top_left_m.x() - map_top_left_m.x();
      int    part_idx_x =
          1.0 * shift_x_m / map_size_m.width() * tile_side_num;
      part_idx_x  = std::clamp(part_idx_x, 0, tile_side_num - 1);
      int shift_y = obj_top_left_m.y() - map_top_left_m.y();
      int part_idx_y =
          1.0 * shift_y / map_size_m.height() * tile_side_num;
      part_idx_y   = std::clamp(part_idx_y, 0, tile_side_num - 1);
      int tile_idx = part_idx_y * tile_side_num + part_idx_x;
      tiles[tile_idx].append(obj);
    }
  }
  qDebug() << "  main tile count" << main.count();
}

FlashMap::ObjectAddress FlashMap::addObject(const FlashObject& obj)
{
  if (frame.isNull())
    frame = obj.polygons.first().getFrame();
  else
    frame = obj.frame.united(obj.polygons.first().getFrame());

  auto map_size_m     = frame.getSizeMeters();
  auto map_top_left_m = frame.top_left.toMeters();

  int  tile_side_num = sqrt(tiles.count());
  int  tile_idx      = -1;
  int  obj_idx       = -1;
  auto cl            = getClass(obj.class_idx);
  if (cl.max_mip == 0 || cl.max_mip > settings.tile_mip)
  {
    main.append(obj);
    obj_idx = main.count() - 1;
  }
  else
  {
    auto   obj_top_left_m = obj.frame.top_left.toMeters();
    double shift_x_m      = obj_top_left_m.x() - map_top_left_m.x();
    int    part_idx_x =
        1.0 * shift_x_m / map_size_m.width() * tile_side_num;
    int shift_y = obj_top_left_m.y() - map_top_left_m.y();
    int part_idx_y =
        1.0 * shift_y / map_size_m.height() * tile_side_num;
    tile_idx = part_idx_y * tile_side_num + part_idx_x;
    tiles[tile_idx].append(obj);
    obj_idx = tiles[tile_idx].count() - 1;
  }
  return {tile_idx + 1, obj_idx};
}

void FlashMap::setObject(const ObjectAddress& addr,
                         const FlashObject&   obj)
{
  if (addr.isValid())
  {
    if (addr.tile_idx == 0)
      main[addr.obj_idx] = obj;
    else
      tiles[addr.tile_idx - 1][addr.obj_idx] = obj;
  }
}

double FlashMap::getMainMip() const
{
  return settings.main_mip;
}

double FlashMap::getTileMip() const
{
  return settings.tile_mip;
}

const FlashClass& FlashMap::getClass(int idx) const
{
  return classes.at(idx);
}

FlashClass FlashMap::getClassById(QString id) const
{
  for (auto& cl: classes)
    if (cl.id == id)
      return cl;
  return FlashClass();
}

QVariant FlashMap::modifyClass(FlashClass new_cl)
{
  for (auto& cl: classes)
    if (cl.id == new_cl.id)
    {
      cl = new_cl;
      return 0;
    }
  return QString(Q_FUNC_INFO) + ": class id" + new_cl.id +
         "not found";
}

void FlashMap::setClass(int idx, const FlashClass& cl)
{
  classes[idx] = cl;
}

int FlashMap::getClassCount() const
{
  return classes.count();
}

void FlashMap::setClasses(QVector<FlashClass> v)
{
  classes = v;
}

FlashGeoRect FlashMap::getFrame() const
{
  return frame;
}

FlashMap::VectorTile::Status FlashMap::getMainTileStatus() const
{
  return main.status;
}

FlashMap::VectorTile::Status
FlashMap::getTileStatus(int tile_idx) const
{
  return tiles.at(tile_idx).status;
}

int FlashMap::getTileCount() const
{
  return tiles.count();
}
