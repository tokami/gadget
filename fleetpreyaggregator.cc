#include "fleetpreyaggregator.h"
#include "areatime.h"
#include "errorhandler.h"
#include "readfunc.h"
#include "stock.h"
#include "stockprey.h"
#include "lengthpredator.h"
#include "fleet.h"
#include "readword.h"
#include "gadget.h"

extern ErrorHandler handle;

FleetPreyAggregator::FleetPreyAggregator(const FleetPtrVector& Fleets,
  const StockPtrVector& Stocks, LengthGroupDivision* const Lgrpdiv,
  const IntMatrix& Areas, const IntMatrix& Ages, int overcons)
  : fleets(Fleets), stocks(Stocks), LgrpDiv(Lgrpdiv),
    areas(Areas), ages(Ages), overconsumption(overcons) {

  int i, j, k, l = 0;
  maxcol.resize(ages.Nrow(), 0);
  mincol.resize(ages.Nrow(), 9999);
  maxrow = 0;
  minrow = 9999;
  int numlengths = LgrpDiv->numLengthGroups();

  for (i = 0; i < stocks.Size(); i++) {
    checkLengthGroupIsFiner(stocks[i]->returnPrey()->returnLengthGroupDiv(), LgrpDiv);
    CI.resize(1);
    CI[i] = new ConversionIndex(stocks[i]->returnPrey()->returnLengthGroupDiv(), LgrpDiv);

    //For convinience, use ap as shorthand for &(stocks[i]->getAgeLengthKeys(0))
    //Changed 25-9 2001 and the memberfunction Areas added to livesinareas
    const AgeBandMatrix* ap = &(stocks[i]->getAgeLengthKeys(stocks[i]->Areas()[0]));

    //Now, loop over all the possible ages in the Ages matrix,
    for (j = 0; j < ages.Nrow(); j++) {
      for (k = 0; k < ages.Ncol(j); k++) {
        l = ages[j][k];

        if (l >= ap->minAge() && l <= ap->maxAge()) {
          //l is within the stock age range

          if (j < minrow) //Update minrow if this age is in a lower row of the Ages matrix
            minrow = j;
          if (j > maxrow) //Update maxrow if this age is in a higher row of the Ages matrix
            maxrow = j;

          //If the stock minlength is not smaller than in the CI
          if (ap->minLength(l) >= CI[i]->minLength()) {
            //update mincol if the CI minlength is smaller than mincol
            if (CI[i]->Pos(ap->minLength(l)) < mincol[j])
              mincol[j] = CI[i]->Pos(ap->minLength(l));

          } else {
            //Else the stock minlength is smaller than in CI
            mincol[j] = 0;
          }

          //If the stock maxlength is not larger than in the CI
          if (ap->maxLength(l) - 1 < CI[i]->maxLength()) {
            //update maxcol if the CI minlength is larger than mincol
            if (CI[i]->Pos(ap->maxLength(l) - 1) > maxcol[j])
              maxcol[j] = CI[i]->Pos(ap->maxLength(l) - 1);

          } else {
            //Else the stock maxlength is larger than in CI
            maxcol[j] = numlengths - 1;
          }
        }
      }
    }
  }

  //Resize total using dummy variables tmppop and popmatrix.
  PopInfo tmppop;
  tmppop.N = 1.0;
  tmppop.W = 1.0;
  PopInfoMatrix popmatrix(ages.Nrow(), numlengths, tmppop);
  total.resize(areas.Nrow(), 0, 0, popmatrix);
}

FleetPreyAggregator::~FleetPreyAggregator() {
  int i;
  for (i = 0; i < CI.Size(); i++)
    delete CI[i];
}

void FleetPreyAggregator::Print(ofstream& outfile) const {
  int i, j, k;

  for (i = 0; i < total.Size(); i++) {
    outfile << "\tInternal areas " << i << endl;
    for (j = 0; j < total[i].Nrow(); j++) {
      outfile << TAB;
      for (k = 0; k < total[i].maxLength(j); k++) {
        outfile.width(smallwidth);
        outfile << total[i][j][k].N << sep;
      }
      outfile << endl;
    }
  }
  outfile << flush;
}

void FleetPreyAggregator::Reset() {
  int i, j, k;
  PopInfo nullpop;

  for (i = 0; i < total.Size(); i++)
    for (j = 0; j < total[i].Nrow(); j++)
      for (k = 0; k < total[i].maxLength(j); k++)
        total[i][j][k] = nullpop;
}

int FleetPreyAggregator::checkCatchData() {
  int i, j, k;
  double check = 0.0;

  for (i = 0; i < total.Size(); i++)
    for (j = 0; j < total[i].Nrow(); j++)
      for (k = 0; k < total[i].maxLength(j); k++)
        check += total[i][j][k].N;
  if (isZero(check))
    return 1;
  return 0;
}

void FleetPreyAggregator::Sum(const TimeClass* const TimeInfo) {

  int i, j, k, f, h, z;
  int aggrArea, aggrAge, area, age;
  double fleetscale;

  this->Reset();
  //Sum over the appropriate fleets, stocks, areas, ages and length groups.
  for (f = 0; f < fleets.Size(); f++) {
    LengthPredator* pred = fleets[f]->returnPredator();
    for (h = 0; h < stocks.Size(); h++) {
      //AJ 06.06.00 Typecast Prey which is returned from returnPrey to (StockPrey*)
      StockPrey* prey = (StockPrey*)stocks[h]->returnPrey();
      for (aggrArea = 0; aggrArea < areas.Nrow(); aggrArea++) {
        for (j = 0; j < areas.Ncol(aggrArea); j++) {
          area = areas[aggrArea][j];
          if (prey->isInArea(area) && fleets[f]->isInArea(area)) {
            fleetscale = fleets[f]->Amount(area, TimeInfo) * pred->Scaler(area);
            if (fleets[f]->Type() == LINEARFLEET)
              fleetscale *= TimeInfo->LengthOfCurrent() / TimeInfo->LengthOfYear();

            for (i = 0; i < pred->numPreys(); i++) {
              if (prey->getName() == pred->Preys(i)->getName()) {
                const DoubleIndexVector* suitptr = &pred->Suitability(i)[0];
                const AgeBandMatrix* alptr = &prey->AlkeysPriorToEating(area);
                for (aggrAge = 0; aggrAge < ages.Nrow(); aggrAge++) {
                  for (k = 0; k < ages.Ncol(aggrAge); k++) {
                    age = ages[aggrAge][k];
                    if ((alptr->minAge() <= age) && (age <= alptr->maxAge())) {

                      DoubleIndexVector Ratio = *suitptr;
                      if (overconsumption)
                        for (z = Ratio.minCol(); z < Ratio.maxCol(); z++)
                          Ratio[z] *= (prey->Ratio(area, z) > 1 ? 1.0 / prey->Ratio(area, z) : 1.0);

                      total[aggrArea][aggrAge].Add((*alptr)[age], *CI[h], fleetscale, Ratio);
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}
