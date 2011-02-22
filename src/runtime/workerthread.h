/***************************************************************************
 *  Copyright (C) 2008-2009 Massachusetts Institute of Technology          *
 *                                                                         *
 *  This source code is part of the PetaBricks project and currently only  *
 *  available internally within MIT.  This code may not be distributed     *
 *  outside of MIT. At some point in the future we plan to release this    *
 *  code (most likely GPL) to the public.  For more information, contact:  *
 *  Jason Ansel <jansel@csail.mit.edu>                                     *
 *                                                                         *
 *  A full list of authors may be found in the file AUTHORS.               *
 ***************************************************************************/
#ifndef PETABRICKSWORKERTHREAD_H
#define PETABRICKSWORKERTHREAD_H

#include "dynamictask.h"
#include "common/thedeque.h"

#include <pthread.h>
#include <set>

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

namespace petabricks {

class DynamicScheduler;
class WorkerThreadPool;

class WorkerThread {
public:
  static WorkerThread* self();

  WorkerThread(DynamicScheduler& ds);
  ~WorkerThread();

  ///
  /// called from a remote thread, taking work
  DynamicTask* steal(){
    DynamicTask* t = NULL;
#ifdef WORKERTHREAD_INJECT
    if(!_injectQueue.empty())
      t=_injectQueue.pop_bottom();
#endif
    if(t==NULL)
      t = _deque.pop_bottom();
    return t;
  }
  
  ///
  /// called from a remote thread, giving work
  void inject(DynamicTask* t){
#ifdef WORKERTHREAD_INJECT
    _injectQueue.push_bottom(t);
#else
    JASSERT(false)(t).Text("support for WorkerThread::inject() not compiled in");
#endif
  }
  
  ///
  /// called on WorkerThread::self(), taking work
  DynamicTask* popLocal(){
    DynamicTask* t = NULL;
#ifdef WORKERTHREAD_ONDECK
    if(_ondeck != NULL){
      t = _ondeck;
      _ondeck=NULL;
      return t;
    }
#endif
    t = _deque.pop_top();
#ifdef WORKERTHREAD_INJECT
    if(t==NULL && !_injectQueue.empty())
      t=_injectQueue.pop_top();
#endif
    return t;
  }
  
  ///
  /// called on WorkerThread::self(), giving work
  void pushLocal(DynamicTask* t){
#ifdef WORKERTHREAD_ONDECK
    if(_ondeck == NULL){
      _ondeck = t;
      return;
    }
#endif
    _deque.push_top(t);
  }
  
  ///
  /// Racy count of the number of items of work left
  int workCount() const {
    //ondeck is excluded purposefully (it cant be stolen)
    return (int)_deque.size()
#ifdef WORKERTHREAD_INJECT
         + (int)injectQueue.size()
#endif
    ;
  }
  
  ///
  /// thread local random number generator
  int threadRandInt() const;

  ///
  /// A single iteration of the main loop
  void popAndRunOneTask(int stealLimit);

  ///
  /// Main loop for worker threads
  void mainLoop();

  int id() const { return _id; }
  WorkerThreadPool& pool() { return _pool; }
  const WorkerThreadPool& pool() const { return _pool; }
#ifdef DEBUG
  bool isWorking() const { return _isWorking; }
#endif
private:
  int _id;
#ifdef WORKERTHREAD_ONDECK
  DynamicTask* _ondeck; // a special queue for the *first* task pushed (improves locality)
#endif
  THEDeque<DynamicTask*> _deque;
#ifdef WORKERTHREAD_INJECT
  LockingDeque<DynamicTask*> _injectQueue;
#endif
  mutable struct { int z; int w; } _randomNumState;
  WorkerThreadPool& _pool;
#ifdef DEBUG
  bool _isWorking;
#endif
} __attribute__ ((aligned (CACHE_LINE_SIZE)));

/**
 * A pool of worker threads
 * This data structure becomes inefficient when/after removals occur
 */
class WorkerThreadPool {
public:
  //
  // constructor 
  WorkerThreadPool();

  //
  // insert a new thread into the pool in a lock-free way
  void insert(WorkerThread* thread);
  
  //
  // remove a thread from the pool in a lock-free way
  void remove(WorkerThread* thread);

  //
  // pick a random thread from the pool
  WorkerThread* getRandom(const WorkerThread* caller = WorkerThread::self());
  
  //
  // pick a thread from the pool, using i as a hint of the location
  WorkerThread* getFixed(int i=0);


  void debugPrint() const;
  void debugPrint(jalib::JAssert&) const;
private:
  WorkerThread* _pool[MAX_NUM_WORKERS];
  int _count;
};


/**
 * A task that aborts all threads in the pool
 */
class AbortTask : public DynamicTask {
public:
  AbortTask(int totalThreads, bool shouldExit = false);
  DynamicTaskPtr run();
private:
  jalib::JMutex _lock;
  jalib::AtomicT _numLive;
  jalib::AtomicT _numAborting;
  bool _shutdown;
};

}

#endif
