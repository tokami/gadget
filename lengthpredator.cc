#include "lengthpredator.h"

LengthPredator::LengthPredator(const char* givenname, const intvector& Areas,
  const LengthGroupDivision* const OtherLgrpDiv,
  const LengthGroupDivision* const GivenLgrpDiv, double multi)
  : PopPredator(givenname, Areas, OtherLgrpDiv, GivenLgrpDiv) {

  scaler.resize(Areas.Size(), 0);
  Multiplicative = multi;
}

LengthPredator::~LengthPredator() {
}

void LengthPredator::Sum(const popinfovector& NumberInArea, int area) {
  int inarea = AreaNr[area];
  int i;
  for (i = 0; i < Prednumber[inarea].Size(); i++)
    Prednumber[inarea][i].N = 0.0;
  Prednumber[inarea].Sum(&NumberInArea, *CI);
}