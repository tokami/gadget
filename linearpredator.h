#ifndef linearpredator_h
#define linearpredator_h

#include "lengthpredator.h"
#include "commentstream.h"

class LinearPredator;
class LengthGroupDivision;
class Keeper;

class LinearPredator : public LengthPredator {
public:
  LinearPredator(CommentStream& infile, const char* givenname,
    const intvector& areas, const LengthGroupDivision* const OtherLgrpDiv,
    const LengthGroupDivision* const GivenLgrpDiv, const TimeClass* const TimeInfo,
    Keeper* const keeper, double Multiplicative);
  virtual ~LinearPredator();
  virtual void Eat(int area, double LengthOfStep, double Temperature, double Areasize,
    int CurrentSubstep, int NrOfSubsteps);
  virtual void AdjustConsumption(int area, int NrOfSubsteps, int CurrentSubstep);
  virtual void Print(ofstream& infile) const;
  virtual const popinfovector& NumberPriortoEating (int area, const char* preyname) const;
};

#endif