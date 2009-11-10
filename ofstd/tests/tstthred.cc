/*
 *
 *  Copyright (C) 2000-2005, OFFIS
 *
 *  This software and supporting documentation were developed by
 *
 *    Kuratorium OFFIS e.V.
 *    Healthcare Information and Communication Systems
 *    Escherweg 2
 *    D-26121 Oldenburg, Germany
 *
 *  THIS SOFTWARE IS MADE AVAILABLE,  AS IS,  AND OFFIS MAKES NO  WARRANTY
 *  REGARDING  THE  SOFTWARE,  ITS  PERFORMANCE,  ITS  MERCHANTABILITY  OR
 *  FITNESS FOR ANY PARTICULAR USE, FREEDOM FROM ANY COMPUTER DISEASES  OR
 *  ITS CONFORMITY TO ANY SPECIFICATION. THE ENTIRE RISK AS TO QUALITY AND
 *  PERFORMANCE OF THE SOFTWARE IS WITH THE USER.
 *
 *  Module:  ofstd
 *
 *  Author:  Marco Eichelberg
 *
 *  Purpose: Define classes for Semaphore, Mutex and Read/Write Lock
 *           as used by most multithread implementations
 *
 *
 *  Last Update:      $Author: meichel $
 *  Update Date:      $Date: 2005/12/08 15:49:12 $
 *  CVS/RCS Revision: $Revision: 1.10 $
 *  Status:           $State: Exp $
 *
 *  CVS/RCS Log at end of file
 *
 */

#include "dcmtk/config/osconfig.h"

#include "dcmtk/ofstd/ofconsol.h"
#include "dcmtk/ofstd/ofthread.h"
#include "dcmtk/ofstd/ofstring.h"
#include "dcmtk/ofstd/ofstd.h"


static void bailout(const char *message, int line)
{
  CERR << "[" << line << "]: " << message << endl;
  exit(10);
}

static OFMutex *mutex=NULL;
static int mtx_var=0;
static int mtx_cond1=0;
static int mtx_cond2=0;
static int mtx_cond3=0;


class MutexT1: public OFThread
{
public:
  MutexT1(): OFThread() {}
  ~MutexT1() {}

  virtual void run()
  {
    if (OFMutex::busy == mutex->trylock()) mtx_cond1=1; // trylock works
    if (0 == mutex->lock())
    {
      mtx_var = 1;
      OFStandard::sleep(1); // since I've got the mutex, nobody should write to mtx_var
      if ((mtx_var == 1)&&(0 == mutex->unlock())) mtx_cond2 = 1;
    }
  }
};

class MutexT2: public OFThread
{
public:
  MutexT2(): OFThread() {}
  ~MutexT2() {}

  virtual void run()
  {
    if (0 == mutex->lock())
    {
      mtx_var = 2;
      OFStandard::sleep(1); // since I've got the mutex, nobody should write to mtx_var
      if ((mtx_var == 2)&&(0 == mutex->unlock())) mtx_cond3 = 1;
    }
  }
};

void mutex_test()
{
  OFString errmsg;
  mutex = new OFMutex();
  if ((!mutex)||(! mutex->initialized())) bailout("creation of mutex failed", __LINE__);
  int condition = mutex->trylock();
  if (condition)
  {
    mutex->errorstr(errmsg, condition);
    CERR << "mutex lock failed: ";
    bailout(errmsg.c_str(), __LINE__);
  }

  mtx_var = -1;

  MutexT1 t1;
  if (0 != t1.start()) bailout("unable to create thread, mutex test failed", __LINE__);

  MutexT2 t2;
  if (0 != t2.start()) bailout("unable to create thread, mutex test failed", __LINE__);

  OFStandard::sleep(1); // since I've got the mutex, nobody should write to mtx_var
  if (mtx_var != -1) bailout("mutex test failed", __LINE__);

  int i=0;
  while ((i++<5) && (!mtx_cond1)) OFStandard::sleep(1);
  if (!mtx_cond1) bailout("mutex trylock test failed", __LINE__);

  if (0 != (condition = mutex->unlock()))
  {
    mutex->errorstr(errmsg, condition);
    CERR << "mutex unlock failed: ";
    bailout(errmsg.c_str(), __LINE__);
  }

  while ((i++<5) && ((!mtx_cond2)||(!mtx_cond3))) OFStandard::sleep(1);
  if ((!mtx_cond2) || (!mtx_cond3)) bailout("mutex lock/unlock test failed", __LINE__);

  delete mutex;
  CERR << "mutex test passed." << endl;
}


static OFSemaphore *semaphore=NULL;
static int sem_cond1=0;
static int sem_cond2=0;
static int sem_cond3=0;
static int sem_cond4=0;

class SemaT1: public OFThread
{
public:
  SemaT1(): OFThread() {}
  ~SemaT1() {}

  virtual void run()
  {
    if (0 == semaphore->wait())
    {
      sem_cond1 = 1; // acquired semaphore
      mutex->lock();
      mutex->unlock();
      if (0== semaphore->post()) sem_cond2=1;
    }
  }
};

class SemaT2: public OFThread
{
public:
  SemaT2(): OFThread() {}
  ~SemaT2() {}

  virtual void run()
  {
    if (OFSemaphore::busy == semaphore->trywait())
    {
      if (0 == semaphore->wait()) // block on semaphore
      {
        sem_cond3 = 1; // acquired semaphore
        if (0== semaphore->post()) sem_cond4=1;
      }
    }
  }
};

void semaphore_test()
{
  OFString errmsg;
  mutex = new OFMutex();
  if ((!mutex)||(! mutex->initialized())) bailout("creation of mutex failed", __LINE__);
  semaphore = new OFSemaphore(2);
  if ((!semaphore)||(! semaphore->initialized())) bailout("creation of semaphore failed", __LINE__);
  int condition = mutex->trylock();
  if (condition)
  {
    mutex->errorstr(errmsg, condition);
    CERR << "mutex lock failed: ";
    bailout(errmsg.c_str(), __LINE__);
  }
  condition = semaphore->trywait();
  if (condition)
  {
    semaphore->errorstr(errmsg, condition);
    CERR << "semaphore acquisition failed: ";
    bailout(errmsg.c_str(), __LINE__);
  }

  SemaT1 t1;
  if (0 != t1.start()) bailout("unable to create thread, semaphore test failed", __LINE__);

  SemaT2 t2;
  if (0 != t2.start()) bailout("unable to create thread, semaphore test failed", __LINE__);

  int i=0;
  while ((i++<5) && (!sem_cond1)) OFStandard::sleep(1);
  if (!sem_cond1) bailout("semaphore lock/unlock test failed", __LINE__);
  OFStandard::sleep(1);
  if (sem_cond3) bailout("semaphore lock/unlock test failed", __LINE__); // make sure T2 is really blocked
  mutex->unlock();

  i=0;
  while ((i++<5) && ((!sem_cond2)||(!sem_cond3)||(!sem_cond4))) OFStandard::sleep(1);
  if ((!mtx_cond2) || (!mtx_cond3) || (!sem_cond4)) bailout("semaphore lock/unlock test failed", __LINE__);

  delete mutex;
  delete semaphore;
  CERR << "semaphore test passed." << endl;
}

static OFReadWriteLock *rwlock=NULL;
static OFMutex *mutex2=NULL;
static int rw_cond1=0;
static int rw_cond2=0;
static int rw_cond3=0;
static int rw_cond4=0;
static int rw_cond5=0;
static int rw_cond6=0;
static int rw_cond7=0;

class RWLockT1: public OFThread
{
public:
  RWLockT1(): OFThread() {}
  ~RWLockT1() {}

  virtual void run()
  {
    if (0 == rwlock->rdlock())
    {
      rw_cond1 = 1; // acquired read lock
      mutex->lock();
      mutex->unlock();
      if (0== rwlock->unlock()) rw_cond2=1;
      mutex2->lock();
      mutex2->unlock();
      if (OFReadWriteLock::busy == rwlock->tryrdlock()) rw_cond3=1;
      if ((0 == rwlock->rdlock())&&(0==rwlock->unlock())) rw_cond4=1;
    }
    return;
  }
};

class RWLockT2: public OFThread
{
public:
  RWLockT2(): OFThread() {}
  ~RWLockT2() {}

  virtual void run()
  {
    if ((0==mutex2->trylock())&&(OFReadWriteLock::busy == rwlock->trywrlock())) rw_cond5=1;
    if (0 == rwlock->wrlock())
    {
      rw_cond6=1;
      mutex2->unlock();
      OFStandard::sleep(1);
      if (0==rwlock->unlock()) rw_cond7=1;
    }
    return;
  }
};

void rwlock_test()
{
  OFString errmsg;

  mutex = new OFMutex();
  if ((!mutex)||(! mutex->initialized())) bailout("creation of mutex failed", __LINE__);
  mutex2 = new OFMutex();
  if ((!mutex2)||(! mutex2->initialized())) bailout("creation of mutex failed", __LINE__);
  rwlock = new OFReadWriteLock();
  if ((!rwlock)||(! rwlock->initialized())) bailout("creation of read/write lock failed", __LINE__);

  int condition = mutex->trylock();
  if (condition)
  {
    mutex->errorstr(errmsg, condition);
    CERR << "mutex lock failed: ";
    bailout(errmsg.c_str(), __LINE__);
  }

  condition = rwlock->tryrdlock();
  if (condition)
  {
    rwlock->errorstr(errmsg, condition);
    CERR << "read lock failed: ";
    bailout(errmsg.c_str(), __LINE__);
  }

  RWLockT1 t1;
  if (0 != t1.start()) bailout("unable to create thread, semaphore test failed", __LINE__);

  RWLockT2 t2;
  if (0 != t2.start()) bailout("unable to create thread, semaphore test failed", __LINE__);


  int i=0;
  while ((i++<5) && ((!rw_cond1)||(!rw_cond5))) OFStandard::sleep(1);

  if ((!rw_cond1)||(!rw_cond5)) bailout("read/write lock/unlock test failed", __LINE__);
  condition = rwlock->unlock();
  if (condition)
  {
    rwlock->errorstr(errmsg, condition);
    CERR << "read lock failed: ";
    bailout(errmsg.c_str(), __LINE__);
  }
  OFStandard::sleep(1);
  if (rw_cond6) bailout("read/write lock test failed", __LINE__);

  mutex->unlock();

  i=0;
  while ((i++<5) && ((!rw_cond2)||(!rw_cond3)||(!rw_cond4)||(!rw_cond5)||(!rw_cond6)||(!rw_cond7))) OFStandard::sleep(1);
  if ((!rw_cond2)||(!rw_cond3)||(!rw_cond4)||(!rw_cond5)||(!rw_cond6)||(!rw_cond7)) bailout("read/write lock/unlock test failed", __LINE__);

  delete mutex;
  delete mutex2;
  delete rwlock;
  CERR << "read/write lock test passed." << endl;
}

static OFThreadSpecificData *tsdata=NULL;
static int tsd_cond1=0;
static int tsd_cond2=0;
static int tsd_cond3=0;
static int tsd_cond4=0;

class TSDataT1: public OFThread
{
public:
  TSDataT1(): OFThread() {}
  ~TSDataT1() {}

  virtual void run()
  {
    if (0 == tsdata->set(this))
    {
      tsd_cond1 = 1; // acquired read lock
      mutex->lock();
      mutex->unlock();
      void *result = NULL;
      if (0== tsdata->get(result))
      {
        if (result == OFstatic_cast(void *, this)) tsd_cond3 = 1;
      }
    }
  }
};

class TSDataT2: public OFThread
{
public:
  TSDataT2(): OFThread() {}
  ~TSDataT2() {}

  virtual void run()
  {
    if (0 == tsdata->set(this))
    {
      tsd_cond2 = 1; // acquired read lock
      mutex2->lock();
      mutex2->unlock();
      void *result = NULL;
      if (0== tsdata->get(result))
      {
        if (result == OFstatic_cast(void *, this)) tsd_cond4 = 1;
      }
    }
  }
};


void tsdata_test()
{
  OFString errmsg;

  mutex = new OFMutex();
  if ((!mutex)||(! mutex->initialized())) bailout("creation of mutex failed", __LINE__);
  mutex2 = new OFMutex();
  if ((!mutex2)||(! mutex2->initialized())) bailout("creation of mutex failed", __LINE__);
  tsdata = new OFThreadSpecificData();
  if ((!tsdata)||(! tsdata->initialized())) bailout("creation of thread specific data failed", __LINE__);

  int condition = mutex->trylock();
  if (condition)
  {
    mutex->errorstr(errmsg, condition);
    CERR << "mutex lock failed: ";
    bailout(errmsg.c_str(), __LINE__);
  }
  condition = mutex2->trylock();
  if (condition)
  {
    mutex2->errorstr(errmsg, condition);
    CERR << "mutex lock failed: ";
    bailout(errmsg.c_str(), __LINE__);
  }

  TSDataT1 t1;
  if (0 != t1.start()) bailout("unable to create thread, thread specific data test failed", __LINE__);

  TSDataT2 t2;
  if (0 != t2.start()) bailout("unable to create thread, thread specific data test failed", __LINE__);


  int i=0;
  while ((i++<5) && ((!tsd_cond1)||(!tsd_cond2))) OFStandard::sleep(1);

  if ((!tsd_cond1)||(!tsd_cond2)) bailout("thread specific data write test failed", __LINE__);

  condition = mutex->unlock();
  if (condition)
  {
    mutex->errorstr(errmsg, condition);
    CERR << "mutex unlock failed: ";
    bailout(errmsg.c_str(), __LINE__);
  }
  condition = mutex2->unlock();
  if (condition)
  {
    mutex2->errorstr(errmsg, condition);
    CERR << "mutex unlock failed: ";
    bailout(errmsg.c_str(), __LINE__);
  }

  i=0;
  while ((i++<5) && ((!tsd_cond3)||(!tsd_cond4))) OFStandard::sleep(1);
  if ((!tsd_cond3)||(!tsd_cond4)) bailout("thread specific data read test failed", __LINE__);

  delete mutex;
  delete mutex2;
  delete tsdata;
  CERR << "thread specific data test passed." << endl;
}


int main()
{
  mutex_test();
  semaphore_test(); // may assume that mutexes work correctly
  rwlock_test();    // may assume that mutexes and semaphores work correctly
  tsdata_test();
  CERR << "all tests passed." << endl;
  return 0;
}


/*
 *
 * CVS/RCS Log:
 * $Log: tstthred.cc,v $
 * Revision 1.10  2005/12/08 15:49:12  meichel
 * Changed include path schema for all DCMTK header files
 *
 * Revision 1.9  2004/01/16 10:37:23  joergr
 * Removed acknowledgements with e-mail addresses from CVS log.
 *
 * Revision 1.8  2003/08/14 09:01:20  meichel
 * Adapted type casts to new-style typecast operators defined in ofcast.h
 *
 * Revision 1.7  2003/06/06 09:44:14  meichel
 * Added static sleep function in class OFStandard. This replaces the various
 *   calls to sleep(), Sleep() and usleep() throughout the toolkit.
 *
 * Revision 1.6  2002/04/16 13:37:02  joergr
 * Added configurable support for C++ ANSI standard includes (e.g. streams).
 *
 * Revision 1.5  2002/04/11 12:17:19  joergr
 * Removed obsolete return statements to keep Sun CC 2.0.1 quiet.
 *
 * Revision 1.4  2001/09/28 12:42:39  joergr
 * Replaced "cerr" by "CERR".
 *
 * Revision 1.3  2001/06/05 10:32:21  joergr
 * Replaced some #ifdef _WIN32 statements by #ifdef HAVE_WINDOWS_H or #ifdef
 * __CYGWIN__ respectively to reflect the fact that the latest Cygwin/gcc
 * version does not define _WIN32 any more.
 *
 * Revision 1.2  2001/06/01 15:51:42  meichel
 * Updated copyright header
 *
 * Revision 1.1  2000/03/29 16:41:27  meichel
 * Added new classes providing an operating system independent abstraction
 *   for threads, thread specific data, semaphores, mutexes and read/write locks.
 *
 *
 */
