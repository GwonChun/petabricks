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
#ifndef PETABRICKSCHOICEDEPGRAPH_H
#define PETABRICKSCHOICEDEPGRAPH_H


#include "matrixdef.h"
#include "matrixdependency.h"
#include "region.h"
#include "rulechoice.h"

#include "common/jrefcounted.h"
#include "common/srcpos.h"

#include <list>
#include <map>
#include <vector>

namespace petabricks {
class Transform;
class ChoiceGrid;
class CodeGenerator;
typedef jalib::JRef<ChoiceGrid> ChoiceGridPtr;

class ChoiceDepGraphNode;
typedef jalib::JRef<ChoiceDepGraphNode> ChoiceDepGraphNodePtr;

class ChoiceDepGraphNodeList : public std::vector< ChoiceDepGraphNodePtr > {
public:
  void removeDimensionFromRegions(MatrixDefPtr matrix, size_t dimension);
  void fixVersionedRegionsType();
};

typedef std::map<ChoiceDepGraphNode*,ChoiceDepGraphNode*> ChoiceDepGraphNodeRemapping;

class ChoiceDepGraphNodeSet : public std::set<ChoiceDepGraphNode*> {
public:
  void applyRemapping(const petabricks::ChoiceDepGraphNodeRemapping& map);
  bool overlaps(const ChoiceDepGraphNodeSet& that){
    const_iterator i;
    for(i=begin(); i!=end(); ++i){
      if(that.find(*i)!=that.end())
        return true;
    }
    return false;
  }
};

struct DependencyInformation {
  RuleSet rules;
  DependencyDirection direction;

  ///
  /// Add that to this, true if information was updates
  bool merge(const DependencyInformation& that){
    size_t nRules = rules.size();
    DependencyDirection oldDir = direction;
    rules.insert(that.rules.begin(), that.rules.end());
    direction.addDirection(that.direction);
    return rules.size()!=nRules || oldDir!=direction;
  }

  bool contains(const RulePtr& p) const {
    return rules.find(p) != rules.end();
  }

};

class ScheduleDependencies : public std::map<ChoiceDepGraphNode*, DependencyInformation>{
public:
  void merge(const ScheduleDependencies& that){
    for(const_iterator i=that.begin(); i!=that.end(); ++i)
      operator[](i->first).merge(i->second);
  }

  void applyRemapping(const petabricks::ChoiceDepGraphNodeRemapping& map);
};

class ChoiceDepGraphNode : public jalib::JRefCounted,
                     public jalib::JPrintable,
                     public jalib::SrcPosTaggable,
                     public RuleChoiceConsumer {
public:
  ///
  /// Constructor
  ChoiceDepGraphNode();

  ///
  /// Add a dependency edge
  void addDependency(ChoiceDepGraphNode* n, const RulePtr& r, const DependencyDirection& dir){
    JASSERT(_directDependsRemapped.empty());
    _directDependsOriginal[n].rules.insert(r);
    _directDependsOriginal[n].direction.addDirection(dir);
  }

  ///
  /// Print this node name in graphviz/dot format
  void printNode(std::ostream& os) const;

  ///
  /// Print this node's edges in graphviz/dot format
  void printEdges(std::ostream& os) const;

  ///
  /// Name of this node as it appears in graphs
  std::string nodename() const { return "n"+jalib::XToString(_id); }


  ///
  /// Generate code for executing this node
  virtual void generateCode(Transform& trans,
                            CodeGenerator& o,
                            RuleFlavor flavor,
                            const RuleChoiceAssignment& choice,
                            int copyFromGpu = 0) = 0;
  virtual void generateCodeForSlice(Transform& trans,
                                    CodeGenerator& o,
                                    int dimension,
                                    const FormulaPtr& pos,
                                    RuleFlavor flavor,
                                    const RuleChoiceAssignment& choice,
                                    int copyFromGpu = 0) = 0;

  virtual const MatrixDefPtr&    matrix() const = 0;
  virtual const SimpleRegionPtr& region() const = 0;
  virtual const RuleSet&  choices() const = 0;

  ScheduleDependencies& directDepends() {
    return _directDependsRemapped.empty() ? _directDependsOriginal : _directDependsRemapped;
  }
  const ScheduleDependencies& directDepends() const {
    return _directDependsRemapped.empty() ? _directDependsOriginal : _directDependsRemapped;
  }

  const ScheduleDependencies& directDependsOriginal() const { return _directDependsOriginal; }
  const ScheduleDependencies& directDependsRemapped() const { return _directDependsRemapped; }
  ScheduleDependencies& indirectDepends() { return _indirectDepends; }
  const ScheduleDependencies& indirectDepends() const { return _indirectDepends; }

  ///
  /// Run one iteration to update indirectDepends, return count of changes
  int updateIndirectDepends();

  ChoiceDepGraphNodeSet getStronglyConnectedComponent();
  
  ChoiceDepGraphNodeSet getMultioutputComponent();

  void applyRemapping(const ChoiceDepGraphNodeRemapping& map);
  void applyChoiceRemapping(const RuleChoiceAssignment& map);

  void resetRemapping();

  bool isInput() const { return _isInput; }
  bool isOutput() const { return _isOutput; }
  void markInput() { _isInput = true; }
  void markOutput() { _isOutput = true; }
  void markLast() { _isLast = true;  }

  std::string getChoicePrefix(Transform& t);

  virtual bool findValidSchedule(const RuleChoiceAssignment&) { return true; }

#ifdef HAVE_OPENCL
  virtual RegionList getFromRegionOnCpu(const RuleChoiceAssignment& choice) const = 0;
  virtual int numOutMatrixOnGpu(const RuleChoiceAssignment& choice, MatrixDefPtr matrix) = 0;
  virtual void print(const RuleChoiceAssignment& choice) = 0;
#endif
protected:
  int _id;
  bool _isInput;
  bool _isOutput;
  bool _isLast;
  ScheduleDependencies _directDependsOriginal;
  ScheduleDependencies _directDependsRemapped;
  ScheduleDependencies _indirectDepends;
  int _choiceId;
};

class BasicChoiceDepGraphNode : public ChoiceDepGraphNode {
public:
  BasicChoiceDepGraphNode(const MatrixDefPtr& m, const SimpleRegionPtr& r, const ChoiceGridPtr& choices);

  const MatrixDefPtr&    matrix() const { return _matrix; }
  const SimpleRegionPtr& region() const { return _region; }
  const RuleSet&        choices() const { return _choices; }

  void print(std::ostream& o) const {
    o << _matrix->name() << ".region(" << _region << ")";
  }

  void generateCode(Transform& trans, CodeGenerator& o, RuleFlavor flavor,
                            const RuleChoiceAssignment& choice, int copyFromGpu = 0);
  void generateCodeForSlice(Transform& trans, CodeGenerator& o, int dimension, const FormulaPtr& pos, RuleFlavor flavor,
                            const RuleChoiceAssignment& choice, int copyFromGpu = 0);
  void removeDimensionFromRegions(MatrixDefPtr matrix, size_t dimension);
  void fixVersionedRegionsType();

#ifdef HAVE_OPENCL
  RegionList getFromRegionOnCpu(const RuleChoiceAssignment& choice) const;
  int numOutMatrixOnGpu(const RuleChoiceAssignment& choice, MatrixDefPtr matrix);
  void print(const RuleChoiceAssignment& choice){
    std::cout << "BasicChoiceDepGraphNode " << this << ":" << std::endl;
    std::cout << "matrix = " << _matrix->name() << std::endl;
    RulePtr rule = choice.find(this)->second;
    std::cout << "rule = " << rule->isEnabledGpuRule() << std::endl << std::endl;
  }
#endif
private:
  MatrixDefPtr      _matrix;
  SimpleRegionPtr   _region;
  RuleSet           _choices;
};

class MetaChoiceDepGraphNode : public ChoiceDepGraphNode {
public:
  MetaChoiceDepGraphNode(const ChoiceDepGraphNodeSet& set);

  const MatrixDefPtr&    matrix()  const { JASSERT(false); return MatrixDefPtr::null();    }
  const SimpleRegionPtr& region()  const { JASSERT(false); return SimpleRegionPtr::null(); }
  const RuleSet& choices() const { static RuleSet empty; return empty; }

  void print(std::ostream& o) const {
    o << classname() << ":";
    for(ChoiceDepGraphNodeSet::const_iterator i=_originalNodes.begin(); i!=_originalNodes.end(); ++i)
      o << "\\n " << **i;
  }
  
  void generateCode(Transform&,
                    CodeGenerator&,
                    RuleFlavor,
                    const RuleChoiceAssignment&,
                    int /*copyFromGpu = false*/){
    UNIMPLEMENTED();
  }
  void generateCodeForSlice(Transform&,
                            CodeGenerator&,
                            int,
                            const FormulaPtr&,
                            RuleFlavor,
                            const RuleChoiceAssignment&,
                            int /*copyFromGpu = false*/){
    UNIMPLEMENTED();
  }

#ifdef HAVE_OPENCL
  RegionList getFromRegionOnCpu(const RuleChoiceAssignment& /*choice*/) const { 
    UNIMPLEMENTED(); 
    return RegionList();
  }
  int numOutMatrixOnGpu(const RuleChoiceAssignment& /*choice*/, MatrixDefPtr /*matrix*/){
    UNIMPLEMENTED(); 
    return 0;
  }
  void print(const RuleChoiceAssignment& choice){
    std::cout << "MetaChoiceDepGraphNode " << this << "--------" << std::endl;
    for(ChoiceDepGraphNodeSet::iterator i = _originalNodes.begin(); i != _originalNodes.end(); ++i)
      (*i)->print(choice);
    std::cout << "---------------" << this << std::endl << std::endl;;
  }
#endif
protected:
  virtual const char* classname() const { return "MetaChoiceDepGraphNode"; }

protected:
  ChoiceDepGraphNodeSet _originalNodes;
};

class MultiOutputChoiceDepGraphNode : public MetaChoiceDepGraphNode {
public:
  MultiOutputChoiceDepGraphNode(const ChoiceDepGraphNodeSet& set)
    : MetaChoiceDepGraphNode(set)
  {}
  bool findValidSchedule(const RuleChoiceAssignment& choice);
  
  void generateCode(Transform& trans, CodeGenerator& o, RuleFlavor flavor,
                            const RuleChoiceAssignment& choice, int copyFromGpu = 0);

#ifdef HAVE_OPENCL
  RegionList getFromRegionOnCpu(const RuleChoiceAssignment& choice) const;
  int numOutMatrixOnGpu(const RuleChoiceAssignment& choice, MatrixDefPtr matrix);
  void print(const RuleChoiceAssignment& choice){
    std::cout << "MultiOutputChoiceDepGraphNode " << this << "--------" << std::endl;
    for(ChoiceDepGraphNodeSet::iterator i = _originalNodes.begin(); i != _originalNodes.end(); ++i)
      (*i)->print(choice);
    std::cout << "---------------" << this << std::endl << std::endl;;
  }
#endif
protected:
  virtual const char* classname() const { return "MultiOutputChoiceDepGraphNode"; }

};


class SlicedChoiceDepGraphNode : public MetaChoiceDepGraphNode {
public:
  SlicedChoiceDepGraphNode(const ChoiceDepGraphNodeSet& set);
  

  bool findValidSchedule(const RuleChoiceAssignment& choice);

  void generateCode(Transform& trans, CodeGenerator& o, RuleFlavor flavor,
                            const RuleChoiceAssignment& choice, int copyFromGpu = 0);

#ifdef HAVE_OPENCL
  RegionList getFromRegionOnCpu(const RuleChoiceAssignment& choice) const;
  int numOutMatrixOnGpu(const RuleChoiceAssignment& choice, MatrixDefPtr matrix);
  void print(const RuleChoiceAssignment& choice){
    std::cout << "SlicedChoiceDepGraphNode " << this << "--------" << std::endl;
    for(ChoiceDepGraphNodeSet::iterator i = _originalNodes.begin(); i != _originalNodes.end(); ++i)
      (*i)->print(choice);
    std::cout << "---------------" << this << std::endl << std::endl;;
  }
#endif
protected:
  const char* classname() const { return "SlicedChoiceDepGraphNode"; }
private:
  int _dimension;
  bool _forward;
  FormulaPtr _begin;
  FormulaPtr _end;
};


}

#endif


