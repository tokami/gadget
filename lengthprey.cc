#include "lengthprey.h"

LengthPrey::LengthPrey(CommentStream& infile, const intvector& Areas,
  const char* givenname, Keeper* const keeper)
  : Prey(infile, Areas, givenname, keeper) {
}

LengthPrey::LengthPrey(const doublevector& lengths,
  const intvector& Areas, const char* givenname)
  : Prey(lengths, Areas, givenname) {
}

LengthPrey::~LengthPrey() {
}

/* Sum number in Prey length groups. Prey length division is not
 * alloved to be finer than stock length division.
 * Also, initialize variables to 0. */
void LengthPrey::SumUsingPopInfo(const popinfovector& NumberInArea, int area, int CurrentSubstep) {
  int inarea = AreaNr[area];
  int i;
  tooMuchConsumption[inarea] = 0;
  if (CurrentSubstep == 1) {
    for (i = 0; i < consumption.Ncol(inarea); i++)
      consumption[inarea][i] = 0.0;
    for (i = 0; i < Number[inarea].Size(); i++)
      Number[inarea][i].N = 0.0;
    Number[inarea].Sum(&NumberInArea, *CI);
    for (i = 0; i < Number[inarea].Size(); i++)
      numberPriortoEating[inarea][i] = Number[inarea][i];
    popinfo sum;
    for (i = 0; i < Number.Ncol(inarea); i++) {
      sum += Number[inarea][i];
      biomass[inarea][i] = Number[inarea][i].N * Number[inarea][i].W;
      total[inarea] = sum.N * sum.W;
    }
    for (i = 0; i < consumption.Ncol(inarea); i++)
      cons[inarea][i] = 0.0;
  }
}