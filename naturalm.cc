#include "naturalm.h"
#include "readfunc.h"
#include "areatime.h"
#include "keeper.h"
#include "errorhandler.h"
#include "doubleindexvector.h"
#include "gadget.h"

//A simple beginning, only one mortality for each yearclass.
NaturalM::NaturalM(CommentStream& infile, int minage, int maxage, const TimeClass* const TimeInfo,
  Keeper* const keeper) : mortality(maxage - minage + 1, minage),
  lengthofsteps(TimeInfo->StepsInYear(), 0), proportion(maxage - minage + 1, 0) {

  ErrorHandler handle;
  keeper->AddString("naturalm");
  mortality.Read(infile, TimeInfo, keeper);
  this->Reset(TimeInfo);
  keeper->ClearLast();
}

void NaturalM::Reset(const TimeClass* const TimeInfo) {
  mortality.Update(TimeInfo);
  double timeratio;
  int age;

  if (mortality.DidChange(TimeInfo) || TimeInfo->SizeOfStepDidChange()) {
    const int shift = mortality.Mincol();
    timeratio = TimeInfo->LengthOfCurrent() / TimeInfo->LengthOfYear();
    for (age = mortality.Mincol(); age < mortality.Maxcol(); age++)
      if (mortality[age] > 0)
        proportion[age - shift] = exp(-mortality[age] * timeratio);
      else
        proportion[age - shift] = 1.0; //i.e. use mortality rate 0
  }
}

const doublevector& NaturalM::ProportionSurviving(const TimeClass* const TimeInfo) const {
  return proportion;
}

void NaturalM::Print(ofstream& outfile) {
  int i;
  outfile << "Natural mortality\n";
  for (i = mortality.Mincol(); i < mortality.Maxcol(); i++)
    outfile << sep << mortality[i];
  outfile << endl;
}

const doubleindexvector& NaturalM::getMortality() const {
  doubleindexvector* tmpvec  = new doubleindexvector(mortality.Size(), mortality.Mincol());
  int i;
  for (i = tmpvec->Mincol(); i < tmpvec->Maxcol(); i++)
    (*tmpvec)[i] = mortality[i];
  return *tmpvec;
}