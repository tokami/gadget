#ifndef lennaturalm_h
#define lennaturalm_h

#include "commentstream.h"
#include "conversion.h"
#include "formulavector.h"

class LenNaturalM;
class Keeper;
class TimeClass;

class LenNaturalM {
public:
  LenNaturalM(CommentStream& infile, const LengthGroupDivision * lenp, Keeper* const keeper);
  ~LenNaturalM();
  void Print(ofstream& outfile);
  inline const doublevector& NatMortality() { return natmort; };
  void NatCalc();
protected:
  double Hyperbola(double start, double end, double startm, double endm, double len);
  Formulavector parammort;
  doublevector xparammort;
  doublevector natmort;
  LengthGroupDivision * lengroup;
};

#endif