#include "surveyindices.h"
#include "readfunc.h"
#include "readword.h"
#include "readaggregation.h"
#include "areatime.h"
#include "errorhandler.h"
#include "stock.h"
#include "sibylengthonstep.h"
#include "sibyageonstep.h"
#include "sibyfleetonstep.h"
#include "gadget.h"

extern ErrorHandler handle;

SurveyIndices::SurveyIndices(CommentStream& infile, const AreaClass* const Area,
  const TimeClass* const TimeInfo, double weight, const char* name)
  : Likelihood(SURVEYINDICESLIKELIHOOD, weight, name) {

  char text[MaxStrLength];
  strncpy(text, "", MaxStrLength);
  int i, j;

  int overcons = 0;
  IntMatrix ages;
  DoubleVector lengths;
  CharPtrVector ageindex;
  CharPtrVector lenindex;

  char datafilename[MaxStrLength];
  char aggfilename[MaxStrLength];
  char sitype[MaxStrLength];
  strncpy(datafilename, "", MaxStrLength);
  strncpy(aggfilename, "", MaxStrLength);
  strncpy(sitype, "", MaxStrLength);

  ifstream datafile;
  CommentStream subdata(datafile);

  readWordAndValue(infile, "datafile", datafilename);
  readWordAndValue(infile, "sitype", sitype);

  //read in area aggregation from file
  readWordAndValue(infile, "areaaggfile", aggfilename);
  datafile.open(aggfilename, ios::in);
  handle.checkIfFailure(datafile, aggfilename);
  handle.Open(aggfilename);
  i = readAggregation(subdata, areas, areaindex);
  handle.Close();
  datafile.close();
  datafile.clear();

  //Check if we read correct input
  if (areas.Nrow() != 1)
    handle.Message("Error in surveyindex - there should be only one area");

  //Must change from outer areas to inner areas.
  for (i = 0; i < areas.Nrow(); i++)
    for (j = 0; j < areas.Ncol(i); j++)
      if ((areas[i][j] = Area->InnerArea(areas[i][j])) == -1)
        handle.UndefinedArea(areas[i][j]);

  if (strcasecmp(sitype, "lengths") == 0) {
    readWordAndValue(infile, "lenaggfile", aggfilename);
    datafile.open(aggfilename, ios::in);
    handle.checkIfFailure(datafile, aggfilename);
    handle.Open(aggfilename);
    i = readLengthAggregation(subdata, lengths, lenindex);
    handle.Close();
    datafile.close();
    datafile.clear();

  } else if (strcasecmp(sitype, "ages") == 0) {
    readWordAndValue(infile, "ageaggfile", aggfilename);
    datafile.open(aggfilename, ios::in);
    handle.checkIfFailure(datafile, aggfilename);
    handle.Open(aggfilename);
    i = readAggregation(subdata, ages, ageindex);
    handle.Close();
    datafile.close();
    datafile.clear();

  } else if (strcasecmp(sitype, "fleets") == 0) {
    readWordAndValue(infile, "lenaggfile", aggfilename);
    datafile.open(aggfilename, ios::in);
    handle.checkIfFailure(datafile, aggfilename);
    handle.Open(aggfilename);
    i = readLengthAggregation(subdata, lengths, lenindex);
    handle.Close();
    datafile.close();
    datafile.clear();

    //read in the fleetnames
    i = 0;
    infile >> text >> ws;
    if (!(strcasecmp(text, "fleetnames") == 0))
      handle.Unexpected("fleetnames", text);
    infile >> text;
    while (!infile.eof() && !(strcasecmp(text, "overconsumption") == 0)) {
      fleetnames.resize(1);
      fleetnames[i] = new char[strlen(text) + 1];
      strcpy(fleetnames[i++], text);
      infile >> text >> ws;
    }
    infile >> overcons >> ws;

  } else if (strcasecmp(sitype, "ageandlengths") == 0) {
    handle.Warning("The ageandlengths surveyindex likelihood component is no longer supported\nUse the surveydistribution likelihood component instead");

  } else
    handle.Unexpected("lengths, ages or fleets", sitype);

  infile >> text >> ws;
  if (!(strcasecmp(text, "stocknames") == 0))
    handle.Unexpected("stocknames", text);

  i = 0;
  infile >> text;
  //read in the stocknames
  while (!infile.eof() && !(strcasecmp(text, "fittype") == 0)) {
    stocknames.resize(1);
    stocknames[i] = new char[strlen(text) + 1];
    strcpy(stocknames[i++], text);
    infile >> text >> ws;
  }

  //We have now read in all the data from the main likelihood file
  if (strcasecmp(sitype, "lengths") == 0) {
    SI = new SIByLengthOnStep(infile, areas, lengths, areaindex,
      lenindex, TimeInfo, datafilename, this->Name());

  } else if (strcasecmp(sitype, "ages") == 0) {
    SI = new SIByAgeOnStep(infile, areas, ages, areaindex,
      ageindex, TimeInfo, datafilename, this->Name());

  } else if (strcasecmp(sitype, "fleets") == 0) {
    SI = new SIByFleetOnStep(infile, areas, lengths, areaindex,
      lenindex, TimeInfo, datafilename, overcons, this->Name());

  } else
    handle.Message("Error in surveyindex - unrecognised type", sitype);

  //prepare for next likelihood component
  infile >> ws;
  if (!infile.eof()) {
    infile >> text >> ws;
    if (!(strcasecmp(text, "[component]") == 0))
      handle.Unexpected("[component]", text);
  }

  //index ptrvectors are not required - free up memory
  for (i = 0; i < ageindex.Size(); i++)
    delete[] ageindex[i];
  for (i = 0; i < lenindex.Size(); i++)
    delete[] lenindex[i];
}

SurveyIndices::~SurveyIndices() {
  int i;
  for (i = 0; i < stocknames.Size(); i++)
    delete[] stocknames[i];
  for (i = 0; i < fleetnames.Size(); i++)
    delete[] fleetnames[i];
  for (i = 0; i < areaindex.Size(); i++)
    delete[] areaindex[i];
  delete SI;
}

void SurveyIndices::LikelihoodPrint(ofstream& outfile) {
  int i;
  outfile << "\nSurvey Indices " << this->Name() << "\n\nLikelihood " << likelihood
    << "\nWeight " << weight << "\nStock names:";
  for (i = 0; i < stocknames.Size(); i++)
    outfile << sep << stocknames[i];
  outfile << "\nAreas:";
  for (i = 0; i < areaindex.Size(); i++)
    outfile << sep << areaindex[i];
  outfile << endl;
  SI->LikelihoodPrint(outfile);
  outfile << endl;
  outfile.flush();
}

void SurveyIndices::addLikelihood(const TimeClass* const TimeInfo) {
  SI->Sum(TimeInfo);
  if (TimeInfo->CurrentTime() == TimeInfo->TotalNoSteps())
    likelihood += SI->calcRegression();
}

void SurveyIndices::setFleetsAndStocks(FleetPtrVector& Fleets, StockPtrVector& Stocks) {
  int i, j;
  int found = 0;
  StockPtrVector s;
  FleetPtrVector f;

  for (i = 0; i < stocknames.Size(); i++) {
    found = 0;
    for (j = 0; j < Stocks.Size(); j++)
      if (strcasecmp(stocknames[i], Stocks[j]->Name()) == 0) {
        found++;
        s.resize(1, Stocks[j]);
      }
    if (found == 0)
      handle.logFailure("Error in surveyindex - failed to match stock", stocknames[i]);

  }

  for (i = 0; i < fleetnames.Size(); i++) {
    found = 0;
    for (j = 0; j < Fleets.Size(); j++)
      if (strcasecmp(fleetnames[i], Fleets[j]->Name()) == 0) {
        found++;
        f.resize(1, Fleets[j]);
      }

    if (found == 0)
      handle.logFailure("Error in surveyindex - failed to match fleet", fleetnames[i]);
  }

  SI->setFleetsAndStocks(f, s);
}

void SurveyIndices::Reset(const Keeper* const keeper) {
  SI->Reset(keeper);
  Likelihood::Reset(keeper);
}

void SurveyIndices::Print(ofstream& outfile) const {
  int i;
  outfile << "\nSurvey Indices " << this->Name() << " - likelihood value " << likelihood << "\n\tStock names: ";
  for (i = 0; i < stocknames.Size(); i++)
    outfile << stocknames[i] << sep;
  outfile << endl;
  SI->Print(outfile);
}

void SurveyIndices::SummaryPrint(ofstream& outfile) {
  int area;

  //JMB - this is nasty hack since there is only one area
  for (area = 0; area < areaindex.Size(); area++) {
    outfile << "all   all " << setw(printwidth) << areaindex[area] << sep
      << setw(largewidth) << this->Name() << sep << setw(smallwidth) << weight
      << sep << setprecision(largeprecision) << setw(largewidth) << likelihood << endl;
  }
  outfile.flush();
}
