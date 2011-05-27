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
#include "petabricks.h"

using namespace petabricks;

PetabricksRuntime::Main* petabricksMainTransform(){
  return NULL;
}
PetabricksRuntime::Main* petabricksFindTransform(const std::string& ){
  return NULL;
}


int main(int /*argc*/, const char** /*argv*/){
  using namespace petabricks::distributed;

  MatrixRegion2D A = MatrixIO("testdata/Rand2Da","r").readDistributed<2>();
  A.print();

  MatrixRegion2D B = (MatrixRegion2D) A;
  MatrixIO().write(B);

  IndexT s1[] = {0,0};
  IndexT e1[] = {16,16};
  MatrixRegion2D C = A.region(s1, e1);
  MatrixIO().write((MatrixRegion2D) C);


  IndexT s2[] = {1,1};
  IndexT e2[] = {5,5};
  MatrixRegion2D D = A.region(s2, e2);
  MatrixIO().write((MatrixRegion2D) D);

  MatrixRegion1D E = D.row(2);
  MatrixIO().write((MatrixRegion1D) E);

  E.dataHosts();

  return 0;
}

