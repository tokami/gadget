#include "stock.h"
#include "keeper.h"
#include "areatime.h"
#include "naturalm.h"
#include "lennaturalm.h"
#include "cannibalism.h"
#include "grower.h"
#include "stockpredator.h"
#include "initialcond.h"
#include "migration.h"
#include "readfunc.h"
#include "errorhandler.h"
#include "maturitya.h"
#include "maturityb.h"
#include "maturityc.h"
#include "maturityd.h"
#include "renewal.h"
#include "transition.h"
#include "print.h"
#include "spawner.h"
#include "readword.h"
#include "readaggregation.h"
#include "checkconversion.h"
#include "lenstock.h"
#include "readfunc.h"
#include "gadget.h"

LenStock::LenStock(CommentStream& infile, const char* givenname,
  const AreaClass* const Area, const TimeClass* const TimeInfo,
  Keeper* const keeper) : Stock(givenname), cann_vec(0) {

  //Written by kgf 16/7 98
  year = -1;
  haslgr = 1;  //fixme: read this from data file?
  ErrorHandler handle;
  int i, tmpint;
  char text[MaxStrLength];
  strncpy(text, "", MaxStrLength);
  keeper->SetString(this->Name());

  //Read the area data
  infile >> ws;
  if (infile.eof())
    handle.Eof();
  infile >> text;
  intvector tmpareas;
  i = 0;
  if (strcasecmp(text, "livesonareas") == 0) {
    infile >> ws;
    while (isdigit(infile.peek()) && !infile.eof() && (i < Area->NoAreas())) {
      tmpareas.resize(1);
      infile >> tmpint >> ws;
      if ((tmpareas[i] = Area->InnerArea(tmpint)) == -1)
        handle.UndefinedArea(tmpint);
      i++;
    }
  } else
    handle.Unexpected("livesonareas", text);

  this->LetLiveOnAreas(tmpareas);

  //Read the stock age and length data
  int minage;
  int maxage;
  double minlength;
  double maxlength;
  double dl;
  ReadWordAndVariable(infile, "minage", minage);
  ReadWordAndVariable(infile, "maxage", maxage);
  ReadWordAndVariable(infile, "minlength", minlength);
  ReadWordAndVariable(infile, "maxlength", maxlength);
  ReadWordAndVariable(infile, "dl", dl);

  //JMB need to read the location of the reference weights file
  char refweight[MaxStrLength];
  strncpy(refweight, "", MaxStrLength);
  ReadWordAndValue(infile, "refweightfile", refweight);

  LgrpDiv = new LengthGroupDivision(minlength, maxlength, dl);
  if (LgrpDiv->Error())
    LengthGroupPrintError(minlength, maxlength, dl, keeper);

  //JMB need to set the lowerlgrp and size vectors to a default
  //value to allow the whole range of lengths to be calculated
  tmpint = int((maxlength - minlength) / dl);
  intvector lowerlgrp(maxage - minage + 1, 0);
  intvector size(maxage - minage + 1, tmpint);

  Alkeys.resize(areas.Size(), minage, lowerlgrp, size);
  tagAlkeys.resize(areas.Size(), minage, lowerlgrp, size);
  for (i = 0; i < Alkeys.Size(); i++)
    Alkeys[i].SettoZero();

  //Read the length group division used in Grower and in Predator
  doublevector grlengths;
  charptrvector grlenindex;
  char aggfilename[MaxStrLength];
  strncpy(aggfilename, "", MaxStrLength);
  ifstream datafile;
  CommentStream subdata(datafile);

  ReadWordAndValue(infile, "growthandeatlengths", aggfilename);
  datafile.open(aggfilename);
  CheckIfFailure(datafile, aggfilename);
  handle.Open(aggfilename);
  i = ReadLengthAggregation(subdata, grlengths, grlenindex);
  handle.Close();
  datafile.close();
  datafile.clear();

  LengthGroupDivision* GrowLgrpDiv = new LengthGroupDivision(grlengths);
  if (GrowLgrpDiv->Error())
    LengthGroupPrintError(grlengths, keeper);

  CheckLengthGroupIsFiner(LgrpDiv, GrowLgrpDiv, this->Name(), "growth and eat lengths");

  //Read the growth function data
  ReadWordAndVariable(infile, "doesgrow", doesgrow);
  if (doesgrow)
    grower = new Grower(infile, LgrpDiv, GrowLgrpDiv, areas, TimeInfo, keeper, refweight, Area, grlenindex);
  else
    grower = 0;

  //Read the prey data.
  infile >> text;
  if (strcasecmp(text, "iscaught") == 0)
    infile >> iscaught >> ws >> text;
  else
    iscaught = 0;

  if (strcasecmp(text, "iseaten") == 0)
    infile >> iseaten >> ws;
  else
    handle.Unexpected("iseaten", text);

  if (iseaten)
    prey = new MortPrey(infile, areas, this->Name(), minage, maxage, keeper, LgrpDiv);
  else
    prey = 0;

  if (iseaten) //check to see if cannibalism is included
    ReadWordAndVariable(infile, "cannibalism", cannibalism);
  else
    cannibalism = 0; //cannibalism is true only for stocks that are
                     //subject to cannibalism, ie immature stocks.
  if (cannibalism) {
    cann = new Cannibalism(infile, prey->ReturnLengthGroupDiv(), TimeInfo, keeper);
    cann_vec.resize(prey->ReturnLengthGroupDiv()->NoLengthGroups());
  } else
    cann = 0;

  //Read the predator data
  ReadWordAndVariable(infile, "doeseat", doeseat);
  if (doeseat) { //must be a new predator type for multispecies purpose
    //Predator not allowed in single species case.
    cerr << "Error - predator not allowed for single species model\n";
    exit(EXIT_FAILURE);
  } else
    predator = 0;

  if (iseaten && cann_vec.Size() == 0)
    cann_vec.resize(prey->ReturnLengthGroupDiv()->NoLengthGroups(), 0);

  //Read the length dependant natural mortality data
  infile >> text;
  if (strcasecmp(text, "lennaturalm") == 0) {
    if (iseaten) //len_natm is normally added to total mortality in prey
      len_natm = new LenNaturalM(infile, prey->ReturnLengthGroupDiv(), keeper);
    else {
      if (predator != 0)
        len_natm = new LenNaturalM(infile, predator->ReturnLengthGroupDiv(), keeper);
      else
        len_natm = new LenNaturalM(infile, LgrpDiv, keeper);
    }
  } else
    handle.Unexpected("lennaturalm", text);

  //Read the initial condition data
  infile >> text;
  if (strcasecmp(text, "initialconditions") == 0)
    initial = new InitialCond(infile, areas, keeper, refweight, Area);
  else
    handle.Unexpected("initialconditions", text);

  //Read the migration data
  ReadWordAndVariable(infile, "doesmigrate", doesmigrate);
  if (doesmigrate) {
    ReadWordAndVariable(infile, "agedependentmigration", AgeDepMigration);
    infile >> text;
    if (strcasecmp(text, "migrationfile") == 0) {
      infile >> text;
      ifstream subfile(text);
      CommentStream subcomment(subfile);
      CheckIfFailure(subfile, text);
      handle.Open(text);
      migration = new Migration(subcomment, AgeDepMigration,
        areas, Area, TimeInfo, keeper);
      handle.Close();
    } else
      handle.Unexpected("migrationfile", text);

  } else
    migration = 0;

  //Read the maturation data
  ReadWordAndVariable(infile, "doesmature", doesmature);
  if (doesmature) {
    ReadWordAndVariable(infile, "maturitytype", tmpint);
    infile >> text;
    if (strcasecmp(text, "maturityfile") == 0) {
      infile >> text;
      ifstream subfile(text);
      CommentStream subcomment(subfile);
      CheckIfFailure(subfile, text);
      handle.Open(text);

      switch (tmpint) {
        case 1:
          maturity = new MaturityA(subcomment, TimeInfo, keeper,
            minage, lowerlgrp, size, areas, LgrpDiv, 3);
          break;
        case 2:
          maturity = new MaturityB(subcomment, TimeInfo, keeper,
            minage, lowerlgrp, size, areas, LgrpDiv);
          break;
        case 3:
          maturity = new MaturityC(subcomment, TimeInfo, keeper,
            minage, lowerlgrp, size, areas, LgrpDiv, 4);
          break;
        case 4:
          maturity = new MaturityD(subcomment, TimeInfo, keeper,
            minage, lowerlgrp, size, areas, LgrpDiv, 4);
          break;
        default:
          handle.Message("Maturity type is expected to be 1, 2, 3 or 4");
      }

      handle.Close();
      subfile.close();
      subfile.clear();
    } else
      handle.Unexpected("maturityfile", text);

    if (!doesgrow)
      handle.Warning("The stock does not grow, so it is unlikely to mature!");

  } else
    maturity = 0;

  /*JMB code removed from here - see RemovedCode.txt for details*/
  //Read the movement data
  ReadWordAndVariable(infile, "doesmove", doesmove);
  if (doesmove) {
    //transition handles the movements of the age group maxage:
    transition = new Transition(infile, areas, maxage,
      lowerlgrp[maxage - minage], size[maxage - minage], keeper);

  } else
    transition = 0;

  //Read the renewal data
  ReadWordAndVariable(infile, "doesrenew", doesrenew);
  if (doesrenew) {
    infile >> text;
    if (strcasecmp(text, "renewaldatafile") == 0) {
      infile >> text;
      ifstream subfile(text);
      CommentStream subcomment(subfile);
      CheckIfFailure(subfile, text);
      handle.Open(text);
      renewal = new RenewalData(subcomment, areas, Area, TimeInfo, keeper);
      handle.Close();
      subfile.close();
      subfile.clear();
    } else
      handle.Unexpected("renewaldatafile", text);

  } else
    renewal = 0;

  //Read the spawning data
  ReadWordAndVariable(infile, "doesspawn", doesspawn);
  if (doesspawn) {
    infile >> text;
    if (strcasecmp(text, "spawnfile") == 0) {
      infile >> text;
      ifstream subfile(text);
      CommentStream subcomment(subfile);
      CheckIfFailure(subfile, text);
      handle.Open(text);
      spawner = new Spawner(subcomment, minage, maxage, Area, TimeInfo, keeper);
      handle.Close();
      subfile.close();
      subfile.clear();
    } else
      handle.Unexpected("spawnfile", text);

  } else
    spawner = 0;

  //Read the filter data
  ReadWordAndVariable(infile, "filter", filter);

  //Set the birthday for the stock
  birthdate = TimeInfo->StepsInYear();

  //Finished reading from infile - resize objects and clean up
  NumberInArea.AddRows(areas.Size(), LgrpDiv->NoLengthGroups());
  int nrofyears = TimeInfo->LastYear() - TimeInfo->FirstYear() + 1;
  for (i = 0; i < areas.Size(); i++) {
    F.resize(1, new doublematrix(maxage - minage + 1, nrofyears, 0));
    M1.resize(1, new doublematrix(maxage - minage + 1, nrofyears, 0));
    M2.resize(1, new doublematrix(maxage - minage + 1, nrofyears, 0));
    Nbar.resize(1, new doublematrix(maxage - minage + 1, nrofyears, 0));
    Nsum.resize(1, new doublematrix(maxage - minage + 1, nrofyears, 0));
    bio.resize(1, new doublematrix(maxage - minage + 1, nrofyears, 0));
  }
  C.resize(maxage - minage + 1);
  D1.resize(maxage - minage + 1);
  D2.resize(maxage - minage + 1);
  N.resize(maxage - minage + 1);
  E.resize(maxage - minage + 1);
  Z.resize(maxage - minage + 1);

  delete GrowLgrpDiv;
  keeper->ClearAll();
}

LenStock::~LenStock() {
  delete len_natm;
  delete cann;
}

void LenStock::Reset(const TimeClass* const TimeInfo) {

  int i;
  if (TimeInfo->CurrentTime() == 1) {
    this->Clear();
    initial->Initialize(Alkeys);
    if (iseaten)
      prey->Reset();
    if (doesmature)
      maturity->Precalc(TimeInfo);
    if (doesrenew)
      renewal->Reset();
    if (doesgrow)
      grower->Reset();
    if (doesmigrate)
      migration->Reset();
    len_natm->NatCalc();
    year = -1;
    for (i = 0; i < Nsum.Size(); i++)
      Nsum[i]->setElementsTo(0);
  }
  if (doeseat)
    predator->Reset(TimeInfo);
}

Prey* LenStock::ReturnPrey() const {
  return prey;
}

Predator* LenStock::ReturnPredator() const {
  return predator;
}

void LenStock::Print(ofstream& outfile) const {
  int i;

  outfile << "\nLenStock\nName" << sep << this->Name() << "\niseaten" << sep
    << iseaten << "\ndoeseat" << sep << doeseat << "\ndoesmove" << sep
    << doesmove << "\ndoesspawn" << sep << doesspawn << "\ndoesmature" << sep
    << doesmature << "\ndoesrenew" << sep << doesrenew << "\ndoesgrow" << sep
    << doesgrow << "\ndoesmigrate" << sep << doesmigrate << "\nInner areas";

  for (i = 0; i < areas.Size(); i++)
    outfile << sep << areas[i];
  outfile << endl;

  initial->Print(outfile);
  len_natm->Print(outfile);
  if (cannibalism)
    cann->Print(outfile);
  if (doesmature)
    maturity->Print(outfile);
  if (iseaten)
    prey->Print(outfile);
  if (doeseat)
    predator->Print(outfile);
  if (doesmove)
    transition->Print(outfile);
  if (doesrenew)
    renewal->Print(outfile);
  if (doesgrow)
    grower->Print(outfile);
  if (doesmigrate)
    migration->Print(outfile);

  outfile <<"\nAgelength keys\n\nCurrent status\n";
  for (i = 0; i < areas.Size(); i++){
    outfile << "Inner Area " << areas[i] << "\nNumbers\n";
    Printagebandm(outfile, Alkeys[i]);
    outfile << "Mean weights\n";
    PrintWeightinagebandm(outfile, Alkeys[i]);
  }
}

void LenStock::CalcEat(int area,
  const AreaClass* const Area, const TimeClass* const TimeInfo) {

  int i, nrofpredators;

  if (doeseat)
    predator->Eat(area, TimeInfo->LengthOfCurrent(),
      Area->Temperature(area, TimeInfo->CurrentTime()), Area->Size(area),
      TimeInfo->CurrentSubstep(), TimeInfo->NrOfSubsteps());

  if (cannibalism) {
    nrofpredators = cann->nrOfPredators();
    cann_vec.setElementsTo(0.0);
    for (i = 0; i < nrofpredators; i++) {
      // add cannibalism mortality on this prey from each predator in turn
      cann_vec += cann->Mortality(Alkeys[AreaNr[area]],
        cannPredators[i]->Agelengthkeys(area), LgrpDiv,
        cannPredators[i]->ReturnLengthGroupDiv(), TimeInfo, i, len_natm->NatMortality());
    }

    for (i = 0; i < nrofpredators; i++) {
      // calculate consumption of this prey by each predator in turn
      ((MortPrey*)prey)->setConsumption(area, i, cann->getCons(i));
      ((MortPrey*)prey)->setAgeMatrix(i, area, cann->getAgeGroups(i));
    }
    prey->SetCannibalism(area, cann_vec);
  }
}

void LenStock::CheckEat(int area,
  const AreaClass* const Area, const TimeClass* const TimeInfo) {

  if (iseaten) {
    prey->calcZ(area, len_natm->NatMortality()); //calculate total mortality
    prey->calcMeanN(area);
  }
}

void LenStock::AdjustEat(int area,
  const AreaClass* const Area, const TimeClass* const TimeInfo) {
}

void LenStock::ReducePop(int area,
  const AreaClass* const Area, const TimeClass* const TimeInfo) {

  //JMB - do we need to account for tagging in here????

  //written by kgf 31/7 98
  //Apply total mortality over present substep
  if (!iseaten && (predator == 0)) //no mortality applied
    return;
  if (iseaten) //propSurv is meaningsfull only for mortality models
    prey->Multiply(Alkeys[AreaNr[area]], ((MortPrey*)prey)->propSurv(area));
  else //apply only natural mortality on stock
    predator->Multiply(Alkeys[AreaNr[area]], len_natm->NatMortality());
}

void LenStock::CalcNumbers(int area,
  const AreaClass* const Area, const TimeClass* const TimeInfo) {

  calcDone = 0;
  int inarea = AreaNr[area];
  popinfo nullpop;

  int i;
  for (i = 0; i < NumberInArea[inarea].Size(); i++)
    NumberInArea[inarea][i] = nullpop;

  if (doesrenew)
    this->Renewal(area, TimeInfo);

  Alkeys[inarea].Colsum(NumberInArea[inarea]);

  if (doesgrow)
    grower->Sum(NumberInArea[inarea], area);
  if (iseaten)
    prey->Sum(Alkeys[inarea], area, TimeInfo->CurrentSubstep());
  if (doeseat)
    ((MortPredLength*)predator)->Sum(NumberInArea[inarea], area);

  N.setElementsTo(0.0);
  int row, col;
  for (row = Alkeys[area].Minage(); row <= Alkeys[area].Maxage(); row++)
    for (col = Alkeys[area].Minlength(row); col < Alkeys[area].Maxlength(row); col++)
      N[row-Alkeys[area].Minage()] += Alkeys[area][row][col].N;

  if (TimeInfo->CurrentStep() == 1) {
    for (row = Alkeys[area].Minage(); row <= Alkeys[area].Maxage(); row++)
      for (col = Alkeys[area].Minlength(row); col < Alkeys[area].Maxlength(row); col++)
        (*Nsum[inarea])[row - Alkeys[area].Minage()][year + 1] += Alkeys[area][row][col].N;
    calcBiomass(year + 1, area);
  }
}

void LenStock::calcBiomass(int yr, int area) {
  int inarea = AreaNr[area];
  int row, col;
  for (row = Alkeys[area].Minage(); row <= Alkeys[area].Maxage(); row++)
    for (col = Alkeys[area].Minlength(row); col < Alkeys[area].Maxlength(row); col++)
      (*bio[inarea])[row - Alkeys[area].Minage()][yr] +=
        Alkeys[area][row][col].N * Alkeys[area][row][col].W;
}

void LenStock::Grow(int area,
  const AreaClass* const Area, const TimeClass* const TimeInfo) {

  if (!doesgrow && doesmature) {
    cerr << "Error in " << this->Name() << " Maturation with no growth is not implemented\n";
    exit(EXIT_FAILURE);
  }

  if (!doesgrow)
    return;

  int inarea = AreaNr[area];
  grower->GrowthCalc(area, Area, TimeInfo);

  if (grower->getGrowthType()!=6 && grower->getGrowthType()!=7) {
    //New weights at length are calculated
    grower->GrowthImplement(area, NumberInArea[inarea], LgrpDiv);
    if (doesmature) {
      if (maturity->IsMaturationStep(area, TimeInfo)) {
        Alkeys[inarea].Grow(grower->LengthIncrease(area), grower->WeightIncrease(area), maturity, TimeInfo, Area, LgrpDiv, area);
        tagAlkeys[inarea].Grow(grower->LengthIncrease(area), Alkeys[inarea], maturity, TimeInfo, Area, LgrpDiv, area, tagAlkeys.NrOfTagExp());
      } else {
        Alkeys[inarea].Grow(grower->LengthIncrease(area), grower->WeightIncrease(area));
        tagAlkeys[inarea].Grow(grower->LengthIncrease(area), Alkeys[inarea], tagAlkeys.NrOfTagExp());
      }
    } else {
      Alkeys[inarea].Grow(grower->LengthIncrease(area), grower->WeightIncrease(area));
      tagAlkeys[inarea].Grow(grower->LengthIncrease(area), Alkeys[inarea], tagAlkeys.NrOfTagExp());
    }

  } else { //GrowthCalcF || GrowthCalcG
    grower->GrowthImplement(area, LgrpDiv);
    if (doesmature) {
      if (maturity->IsMaturationStep(area, TimeInfo)) {
        Alkeys[inarea].Grow(grower->LengthIncrease(area), grower->getWeight(area), maturity, TimeInfo, Area, LgrpDiv, area);
        tagAlkeys[inarea].Grow(grower->LengthIncrease(area), Alkeys[inarea], maturity, TimeInfo, Area, LgrpDiv, area, tagAlkeys.NrOfTagExp());
      } else {
        Alkeys[inarea].Grow(grower->LengthIncrease(area), grower->getWeight(area));
        tagAlkeys[inarea].Grow(grower->LengthIncrease(area), Alkeys[inarea], tagAlkeys.NrOfTagExp());
      }
    } else {
      Alkeys[inarea].Grow(grower->LengthIncrease(area), grower->getWeight(area));
      tagAlkeys[inarea].Grow(grower->LengthIncrease(area), Alkeys[inarea], tagAlkeys.NrOfTagExp());
    }
  }
  Alkeys[inarea].FilterN(filter); //mnaa
}

void LenStock::calcForPrinting(int area, const TimeClass& time) {
  if (!iseaten || !stockType() == LENSTOCK_TYPE || calcDone)
    return;
  int row, col, mcol;
  const Agebandmatrix& mean_n = ((MortPrey*)prey)->getMeanN(area);
  const doublevector& cons = prey->getCons(area);

  if (time.CurrentStep() == 1)
    year = time.CurrentYear() - time.FirstYear();

  C.setElementsTo(0.0);
  D1.setElementsTo(0.0);
  D2.setElementsTo(0.0);

  for (row = mean_n.Minage(); row <= mean_n.Maxage(); row++)
    for (col = 0; col < cons.Size(); col++) {
      mcol = col + mean_n.Minlength(row);
      C[row - mean_n.Minage()] += mean_n[row][mcol].N*cons[col];
      D1[row - mean_n.Minage()] += mean_n[row][mcol].N*len_natm->NatMortality()[col];
      D2[row - mean_n.Minage()] += mean_n[row][mcol].N*cann_vec[col];
      (*Nbar[area])[row - mean_n.Minage()][year] += mean_n[row][mcol].N;
    }

  /*JMB code removed from here - see RemovedCode.txt for details*/
  for (row = 0; row < E.Size(); row++) {
    E[row] = C[row] + D1[row] + D2[row];
    if (N[row] > E[row]) {
      Z[row] = log(N[row] / (N[row] - E[row]));
      (*(F[area]))[row][year] += C[row] * Z[row] / E[row];
      (*(M1[area]))[row][year] += D1[row] * Z[row] / E[row];
      (*(M2[area]))[row][year] += D2[row] * Z[row] / E[row];
    }
  }
  calcDone = 1;
}

void LenStock::SecondSpecialTransactions(int area,
  const AreaClass* const Area, const TimeClass* const TimeInfo) {

  if (doesmature)
    if (maturity->IsMaturationStep(area, TimeInfo))
      maturity->Move(area, TimeInfo);
}

void LenStock::SetStock(Stockptrvector& stockvec) {
  int i, j, found;
  int minage, maxage, tmpsize;

  if (doesmature)
    maturity->SetStock(stockvec);
  if (doesmove)
    transition->SetStock(stockvec);

  if (iseaten && cannibalism) {
    ((MortPrey*)prey)->cannIsTrue(1);
    ((MortPrey*)prey)->setDimensions(areas.Size(), cann->nrOfPredators());

    for (i = 0; i < cann->nrOfPredators(); i++) {
      found = 0;
      for (j = 0; j < stockvec.Size(); j++)
        if (strcasecmp(stockvec[j]->Name(), cann->predatorName(i)) == 0) {
          found = 1;
          cannPredators.resize(1, stockvec[j]);
        }

      if (found == 0) {
        cout << "Error in Cannibalism on prey " << Name() << " - predator " << cann->predatorName(i) << " not found\n";
        exit(EXIT_FAILURE);
      }

      minage = cann->getMinPredAge(i);
      maxage = cann->getMaxPredAge(i);
      tmpsize = maxage - minage + 1;

      if (cannPredators[i]->Minage() != minage || cannPredators[i]->Maxage() != maxage) {
        cout << "Error in Cannibalism on prey " << Name() << " - predatorages in cannibalism does not match predators\n";
        exit(EXIT_FAILURE);
      }

      ((MortPrey*)prey)->addCannPredName(cann->predatorName(i));
      ((MortPrey*)prey)->setMinPredAge(minage);
      ((MortPrey*)prey)->setMaxPredAge(maxage);

      bandmatrix tmp(0, prey->ReturnLengthGroupDiv()->NoLengthGroups(), minage, tmpsize, 0.0);
      doublematrix* stmp = new doublematrix(areas.Size(), tmpsize, 0.0);
      ((MortPrey*)prey)->addConsMatrix(i, tmp);
      ((MortPrey*)prey)->addAgeGroupMatrix(stmp);
    }
  }
}