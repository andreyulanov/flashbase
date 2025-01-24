#pragma once

#include <QReadWriteLock>

class FlashLocker
{
  QReadWriteLock* lock;
  bool            has_locked;

public:
  enum Mode
  {
    Read,
    Write
  };
  FlashLocker(QReadWriteLock* lock, Mode);
  bool hasLocked();
  virtual ~FlashLocker();
};
