#include "linearpredator.h"
#include "keeper.h"
#include "conversion.h"
#include "prey.h"
#include "mathfunc.h"
#include "gadget.h"

LinearPredator::LinearPredator(CommentStream& infile, const char* givenname,
  const intvector& Areas, const LengthGroupDivision* const OtherLgrpDiv,
  const LengthGroupDivision* const GivenLgrpDiv, const TimeClass* const TimeInfo,
  Keeper* const keeper, double multi)
  : LengthPredator(givenname, Areas, OtherLgrpDiv, GivenLgrpDiv, multi) {

  keeper->AddString("predator");
  keeper->AddString(givenname);

  ReadSuitabilityMatrix(infile, "amount", TimeInfo, keeper);

  keeper->ClearLast();
  keeper->ClearLast();
  //Predator::SetPrey will call ResizeObjects.
}

LinearPredator::~LinearPredator() {
}

void LinearPredator::Eat(int area, double LengthOfStep, double Temperature,
  double Areasize, int CurrentSubstep, int NrOfSubsteps) {

  //The parameters Temperature and Areasize will not be used.
  const int inarea = AreaNr[area];
  int prey, predl, preyl;
  double tmp;

  for (predl = 0; predl < LgrpDiv->NoLengthGroups(); predl++) {
    if (CurrentSubstep == 1) {
      totalconsumption[inarea][predl] = 0;
      overconsumption[inarea][predl] = 0;
      for (prey = 0; prey < NoPreys(); prey++) {
        if (Preys(prey)->IsInArea(area)) {
          for (preyl = Suitability(prey)[predl].Mincol();
              preyl < Suitability(prey)[predl].Maxcol(); preyl++) {
            consumption[inarea][prey][predl][preyl] = 0;
          }
        }
      }
    }
  }

  for (predl = 0; predl < LgrpDiv->NoLengthGroups(); predl++)
    totalcons[inarea][predl] = 0;

  scaler[inarea] = Multiplicative;  //take a look
  tmp = Multiplicative * LengthOfStep / NrOfSubsteps;

  for (prey = 0; prey < NoPreys(); prey++) {
    if (Preys(prey)->IsInArea(area)) {
      if (Preys(prey)->Biomass(area) > 0) {
        for (predl = 0; predl < LgrpDiv->NoLengthGroups(); predl++) {
          for (preyl = Suitability(prey)[predl].Mincol();
              preyl < Suitability(prey)[predl].Maxcol(); preyl++) {

            cons[inarea][prey][predl][preyl] = tmp *
              Suitability(prey)[predl][preyl] * Preys(prey)->Biomass(area, preyl) *
              Prednumber[inarea][predl].N * Prednumber[inarea][predl].W;

            totalcons[inarea][predl] += cons[inarea][prey][predl][preyl];
          }
        }

      } else {
        for (predl = 0; predl < LgrpDiv->NoLengthGroups(); predl++) {
          for (preyl = Suitability(prey)[predl].Mincol();
              preyl < Suitability(prey)[predl].Maxcol(); preyl++) {
            cons[inarea][prey][predl][preyl] = 0;
          }
        }
      }
    }
  }

  //Inform the preys of the consumption.
  for (prey = 0; prey < NoPreys(); prey++) {
    if (Preys(prey)->IsInArea(area)) {
      if (Preys(prey)->Biomass(area) > 0) {
        for (predl = 0; predl < LgrpDiv->NoLengthGroups(); predl++)
          Preys(prey)->AddConsumption(area, cons[inarea][prey][predl]);
      }
    }
  }
}

void LinearPredator::AdjustConsumption(int area, int NrOfSubsteps, int CurrentSubstep) {

  double MaxRatioConsumed = pow(MAX_RATIO_CONSUMED, NrOfSubsteps);
  int prey, predl, preyl;
  int AnyPreyEatenUp = 0;
  double ratio, tmp;
  const int inarea = AreaNr[area];

  for (predl = 0; predl < LgrpDiv->NoLengthGroups(); predl++)
    overcons[inarea][predl] = 0;

  for (prey = 0; prey < NoPreys(); prey++) {
    if (Preys(prey)->IsInArea(area)) {
      if (Preys(prey)->Biomass(area) > 0) {
        if (Preys(prey)->TooMuchConsumption(area) == 1) {
          AnyPreyEatenUp = 1;
          for (predl = 0; predl < LgrpDiv->NoLengthGroups(); predl++) {
            for (preyl = Suitability(prey)[predl].Mincol();
                preyl < Suitability(prey)[predl].Maxcol(); preyl++) {
              ratio = Preys(prey)->Ratio(area, preyl);
              if (ratio > MaxRatioConsumed) {
                tmp = MaxRatioConsumed / ratio;
                overcons[inarea][predl] += (1 - tmp) * cons[inarea][prey][predl][preyl];
                cons[inarea][prey][predl][preyl] *= tmp;
              }
            }
          }
        }
      }
    }

    if (AnyPreyEatenUp == 1)
      for (predl = 0; predl < LgrpDiv->NoLengthGroups(); predl++)
        totalcons[inarea][predl] -= overcons[inarea][predl];
  }

  //Changes after division of timestep in substeps was possible.
  for (predl = 0; predl < LgrpDiv->NoLengthGroups(); predl++)
    totalconsumption[inarea][predl] += totalcons[inarea][predl];

  for (prey = 0; prey < NoPreys(); prey++) {
    if (Preys(prey)->IsInArea(area)) {
      if (Preys(prey)->Biomass(area) > 0){
        for (predl = 0; predl < LgrpDiv->NoLengthGroups(); predl++) {
          for (preyl = Suitability(prey)[predl].Mincol();
              preyl < Suitability(prey)[predl].Maxcol(); preyl++) {
            consumption[inarea][prey][predl][preyl] +=
              cons[inarea][prey][predl][preyl];
          }
        }
      }
    }
  }
}

void LinearPredator::Print(ofstream& outfile) const {
  outfile << "LinearPredator\n";
  PopPredator::Print(outfile);
}

const popinfovector& LinearPredator::NumberPriortoEating(int area, const char* preyname) const {
  int prey;
  for (prey = 0; prey < NoPreys(); prey++)
    if (strcasecmp(Preyname(prey), preyname) == 0)
      return Preys(prey)->NumberPriortoEating(area);

  cerr << "Predator " << this->Name() << " was asked for consumption\n"
    << "of prey " << preyname << " which he does not eat\n";
  exit(EXIT_FAILURE);
}


