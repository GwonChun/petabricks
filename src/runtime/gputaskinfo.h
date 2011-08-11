/*****************************************************************************
 *  Copyright (C) 2008-2011 Massachusetts Institute of Technology            *
 *                                                                           *
 *  Permission is hereby granted, free of charge, to any person obtaining    *
 *  a copy of this software and associated documentation files (the          *
 *  "Software"), to deal in the Software without restriction, including      *
 *  without limitation the rights to use, copy, modify, merge, publish,      *
 *  distribute, sublicense, and/or sell copies of the Software, and to       *
 *  permit persons to whom the Software is furnished to do so, subject       *
 *  to the following conditions:                                             *
 *                                                                           *
 *  The above copyright notice and this permission notice shall be included  *
 *  in all copies or substantial portions of the Software.                   *
 *                                                                           *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY                *
 *  KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE               *
 *  WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND      *
 *  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE   *
 *  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION   *
 *  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION    *
 *  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE           *
 *                                                                           *
 *  This source code is part of the PetaBricks project:                      *
 *    http://projects.csail.mit.edu/petabricks/                              *
 *                                                                           *
 *****************************************************************************/
#ifdef HAVE_OPENCL

#ifndef PETABRICKSGPUTASKINFO_H
#define PETABRICKSGPUTASKINFO_H

#include "common/jmutex.h"

#include "matrixstorage.h"
#include "openclutil.h"

#include <oclUtils.h>

namespace petabricks {

class GpuTaskInfo;
typedef jalib::JRef<GpuTaskInfo> GpuTaskInfoPtr;

class GpuTaskInfo: public jalib::JRefCounted {
public:

  /// constructor
  GpuTaskInfo(int copy);

  void addToMatrix(MatrixStorageInfoPtr info) { _to.push_back(info); }
  void addFromMatrix(MatrixStorageInfoPtr info) { _from.push_back(info); }
  int copyBack() { return _copyOut; }
  void print() {
    std::cout << "GpuTaskInfo " << this << " : _copyOut = " << _copyOut << std::endl;
  }

  std::vector<MatrixStorageInfoPtr> _from;
  std::vector<MatrixStorageInfoPtr> _to;

private:
  cl_command_queue _queue;
  cl_kernel _kernel;
  int _numcomplete;
  int _copyOut;
};

}

#endif
#endif
