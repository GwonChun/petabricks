/***************************************************************************
 *   Copyright (C) 2008 by Jason Ansel                                     *
 *   jansel@csail.mit.edu                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "learner.h"
#include "performancetester.h"

hecura::Learner::Learner() : _numIterations(0) {}


void hecura::Learner::onIterationBegin(){

}

hecura::RuleChoicePtr hecura::Learner::makeRuleChoice( const RuleSet& choices
                                                     , const MatrixDefPtr&
                                                     , const SimpleRegionPtr& )
{
  /*
   * This function must build a stack of RuleChoicePtr's from choices.
   * The field RuleChoice::_next forms a linked list of choices.
   * At runtime the first choice for which RuleChoice::_condition holds is used.
   */


  //split choices into recursive and base
  RuleSet recursive,base;
  for(RuleSet::const_iterator i=choices.begin(); i!=choices.end(); ++i){
    if((*i)->isRecursive())
      recursive.insert(*i);
    else
      base.insert(*i);
  }
  JASSERT(!base.empty())(base.size()).Text("no non-recursive choices exist");

  //default to base case
  RuleChoicePtr rv = new RuleChoice(*base.begin()); //the first rule

  if(!recursive.empty()){
    FormulaPtr condition = new FormulaGT(new FormulaVariable(INPUT_SIZE_STR), new FormulaInteger(1000));

    //add recursive case
    rv=new RuleChoice(*recursive.begin(), condition, rv);
  }
  return rv;
}

void hecura::Learner::onIterationEnd(){
  _numIterations++;
}

bool hecura::Learner::shouldIterateAgain(){
  //for testing... make learner always do 2 iterations
//   return _numIterations < 3; 
  return false;
}

void hecura::Learner::runTests(PerformanceTester& tester){
  double totalTime = tester.runAllTests();
  JTRACE("run PerformanceTester")(totalTime);
} 