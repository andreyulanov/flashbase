#include "flashlocker.h"

FlashLocker::FlashLocker(QReadWriteLock* _lock, Mode mode)
{
  lock       = _lock;
  has_locked = false;
  if (mode == Read)
    has_locked = lock->tryLockForRead();
  else if (mode == Write)
    has_locked = lock->tryLockForWrite();
}

bool FlashLocker::hasLocked()
{
  return has_locked;
}

FlashLocker::~FlashLocker()
{
  if (has_locked)
    lock->unlock();
}
