#ifndef suits_h
#define suits_h

#include "charptrvector.h"
#include "suitfuncptrvector.h"
#include "doublematrixptrvector.h"
#include "bandmatrix.h"
#include "keeper.h"

class Predator;

class Suits {
public:
  Suits();
  ~Suits();
  Suits(const Suits& initial, Keeper* const keeper);
  void addPrey(const char* preyname, SuitFunc* suitf);
  void addPrey(const char* preyname, double multiplication,
    const DoubleMatrix& suitabilities, Keeper* const keeper);
  void DeletePrey(int prey, Keeper* const keeper);
  const char* Preyname(int prey) const;
  int numFuncPreys() const;
  int numPreys() const;
  const BandMatrix& Suitable(int prey) const;
  void Reset(const Predator* const pred, const TimeClass* const TimeInfo);
  int DidChange(int prey, const TimeClass* const TimeInfo) const;
  SuitFunc* FuncPrey(int prey);
protected:
  void DeleteFuncPrey(int prey, Keeper* const keeper);
  void DeleteMatrixPrey(int prey, Keeper* const keeper);
  CharPtrVector FuncPreynames;
  CharPtrVector MatrixPreynames;
  DoubleVector Multiplication;
  SuitFuncPtrVector SuitFunction;
  DoubleMatrixPtrVector MatrixSuit;
  BandMatrixVector PrecalcSuitability;
};

#endif
