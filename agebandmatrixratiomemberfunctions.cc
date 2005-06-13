#include "agebandmatrixratio.h"
#include "agebandmatrixratioptrvector.h"
#include "errorhandler.h"
#include "gadget.h"

extern ErrorHandler handle;

void AgeBandMatrixRatio::updateAndTagLoss(const AgeBandMatrix& Total, const DoubleVector& tagloss) {

  int numTagExperiments = this->numTagExperiments();
  int minlen, maxlen, age, length, tag;
  if (numTagExperiments > 0) {
    for (age = minage; age < minage + nrow; age++) {
      minlen = this->minLength(age);
      maxlen = this->maxLength(age);
      for (length = minlen; length < maxlen; length++) {
        for (tag = 0; tag < numTagExperiments; tag++) {
          (this->operator[](age))[length][tag].R *= tagloss[tag];
          *((this->operator[](age))[length][tag].N) = (this->operator[](age))[length][tag].R * Total[age][length].N;
        }
      }
    }
  }
}

void AgeBandMatrixRatio::updateNumbers(const AgeBandMatrix& Total) {

  int numTagExperiments = this->numTagExperiments();
  int minlen, maxlen, age, length, tag;
  double number, ratio;

  if (numTagExperiments > 0) {
    for (age = minage; age < minage + nrow; age++) {
      minlen = this->minLength(age);
      maxlen = this->maxLength(age);
      for (length = minlen; length < maxlen; length++) {
        for (tag = 0; tag < numTagExperiments; tag++) {
          number = Total[age][length].N;
          ratio = (this->operator[](age))[length][tag].R;
          if (number < verysmall || ratio < verysmall) {
            *((this->operator[](age))[length][tag].N) = 0.0;
            (this->operator[](age))[length][tag].R = 0.0;
          } else {
            *((this->operator[](age))[length][tag].N) = ratio * number;
          }
        }
      }
    }
  }
}

void AgeBandMatrixRatio::updateRatio(const AgeBandMatrix& Total) {

  int numTagExperiments = this->numTagExperiments();
  int minlen, maxlen, age, length, tag;
  double tagnum, totalnum;

  if (numTagExperiments > 0) {
    for (age = minage; age < minage + nrow; age++) {
      minlen = this->minLength(age);
      maxlen = this->maxLength(age);
      for (length = minlen; length < maxlen; length++) {
        for (tag = 0; tag < numTagExperiments; tag++) {
          tagnum = *((this->operator[](age))[length][tag].N);
          totalnum = Total[age][length].N;
          if (tagnum < verysmall || totalnum < verysmall) {
            *((this->operator[](age))[length][tag].N) = 0.0;
            (this->operator[](age))[length][tag].R = 0.0;
          } else {
            (this->operator[](age))[length][tag].R = tagnum / totalnum;
          }
        }
      }
    }
  }
}

void AgeBandMatrixRatio::IncrementAge(const AgeBandMatrix& Total) {

  int numTagExperiments = this->numTagExperiments();
  int i, j, j1, j2, tag;

  if (nrow <= 1)
    return;

  if (numTagExperiments > 0) {
    i = nrow - 1;
    j1 = max(v[i]->minCol(), v[i - 1]->minCol());
    j2 = min(v[i]->maxCol(), v[i - 1]->maxCol());
    //For the highest age group
    for (j = j1; j < j2; j++)
      for (tag = 0; tag < numTagExperiments; tag++)
        (*(*v[i])[j][tag].N) += (*(*v[i - 1])[j][tag].N);

    for (j = v[i - 1]->minCol(); j < v[i - 1]->maxCol(); j++)
      for (tag = 0; tag < numTagExperiments; tag++) {
        (*(*v[i - 1])[j][tag].N) = 0.0;
        (*v[i - 1])[j][tag].R = 0.0;
      }

    //For the other age groups.
    //At the end of each for (i=nrow-2...) loop, the intersection of v[i-1] with
    //v[i] has been copied from v[i-1] to v[i] and v[i-1] has been set to 0.
    for (i = nrow - 2; i > 0; i--) {
      j1 = max(v[i]->minCol(), v[i - 1]->minCol());
      j2 = min(v[i]->maxCol(), v[i - 1]->maxCol());
      for (j = v[i - 1]->minCol(); j < j1; j++) {
        for (tag = 0; tag < numTagExperiments; tag++) {
          (*(*v[i - 1])[j][tag].N) = 0.0;
          (*v[i - 1])[j][tag].R = 0.0;
        }
      }

      for (j = j1; j < j2; j++) {
        for (tag = 0; tag < numTagExperiments; tag++) {
          (*(*v[i])[j][tag].N) = (*(*v[i - 1])[j][tag].N);
          (*(*v[i - 1])[j][tag].N) = 0.0;
          (*v[i - 1])[j][tag].R = 0.0;
        }
      }

      for (j = j2; j < v[i - 1]->maxCol(); j++) {
        for (tag = 0; tag < numTagExperiments; tag++) {
          (*(*v[i - 1])[j][tag].N) = 0.0;
          (*v[i - 1])[j][tag].R = 0.0;
        }
      }
    }

    //set number in age zero to zero.
    for (j = v[0]->minCol(); j < v[0]->maxCol(); j++)
      for (tag = 0; tag < numTagExperiments; tag++)
        (*(*v[0])[j][tag].N) = 0;

    this->updateRatio(Total);
  }
}

void AgebandmratioAdd(AgeBandMatrixRatioPtrVector& Alkeys, int AlkeysArea,
  const AgeBandMatrixRatioPtrVector& Addition, int AdditionArea,
  const ConversionIndex &CI, double ratio, int minage, int maxage) {

  minage =  max(Alkeys[AlkeysArea].minAge(), Addition[AdditionArea].minAge(), minage);
  maxage =  min(Alkeys[AlkeysArea].maxAge(), Addition[AdditionArea].maxAge(), maxage);
  if (maxage < minage)
    return;

  int age, minl, maxl, i, l, tagid, numtags, offset;
  double numfish;

  numtags = Addition.numTagExperiments();
  if (numtags > Alkeys.numTagExperiments())
    handle.logMessage(LOGFAIL, "Error in agebandmatrixratio - wrong number of tagging experiments");

  if (numtags > 0) {
    IntVector tagconversion(numtags);
    for (i = 0; i < numtags; i++) {
      tagconversion[i] = Alkeys.getTagID(Addition.getTagName(i));
      if (tagconversion[i] < 0)
        handle.logMessage(LOGFAIL, "Error in agebandmatrixratio - unrecognised tagging experiment", Addition.getTagName(i));

    }

    numfish = 0.0;
    if (CI.isSameDl()) { //Same dl on length distributions
      offset = CI.getOffset();
      for (age = minage; age <= maxage; age++) {
        minl = max(Alkeys[AlkeysArea].minLength(age), Addition[AdditionArea].minLength(age) + offset);
        maxl = min(Alkeys[AlkeysArea].maxLength(age), Addition[AdditionArea].maxLength(age) + offset);
        for (l = minl; l < maxl; l++) {
          for (tagid = 0; tagid < numtags; tagid++) {
            numfish = *(Addition[AdditionArea][age][l - offset][tagid].N);
            numfish *= ratio;
            (*Alkeys[AlkeysArea][age][l][tagconversion[tagid]].N) += numfish;
          }
        }
      }

    } else { //Not same dl.
      if (CI.isFiner()) {
        //Stock that is added to has finer division than the stock that is added to it.
        for (age = minage; age <= maxage; age++) {
          minl = max(Alkeys[AlkeysArea].minLength(age), CI.minPos(Addition[AdditionArea].minLength(age)));
          maxl = min(Alkeys[AlkeysArea].maxLength(age), CI.maxPos(Addition[AdditionArea].maxLength(age) - 1) + 1);
          for (l = minl; l < maxl; l++) {
            for (tagid = 0; tagid < numtags; tagid++) {
              numfish = *(Addition[AdditionArea][age][CI.getPos(l)][tagid].N);
              numfish *= ratio;
              if (isZero(CI.Nrof(l))) {
                if (handle.getLogLevel() >= LOGWARN)
                  handle.logMessage(LOGWARN, "Warning in agebandmatrixratio - divide by zero");
              } else
                numfish /= CI.Nrof(l);
              *(Alkeys[AlkeysArea][age][l][tagconversion[tagid]].N) += numfish;
            }
          }
        }

      } else {
        //Stock that is added to has coarser division than the stock that is added to it.
        for (age = minage; age <= maxage; age++) {
          minl = max(CI.minPos(Alkeys[AlkeysArea].minLength(age)), Addition[AdditionArea].minLength(age));
          maxl = min(CI.maxPos(Alkeys[AlkeysArea].maxLength(age) - 1) + 1, Addition[AdditionArea].maxLength(age));
          if (maxl > minl && CI.getPos(maxl - 1) < Alkeys[AlkeysArea].maxLength(age)
            && CI.getPos(minl) >= Alkeys[AlkeysArea].minLength(age)) {

            for (l = minl; l < maxl; l++) {
              for (tagid = 0; tagid < numtags; tagid++) {
                numfish = *(Addition[AdditionArea][age][l][tagid].N);
                numfish *= ratio;
                *(Alkeys[AlkeysArea][age][CI.getPos(l)][tagconversion[tagid]].N) += numfish;
              }
            }
          }
        }
      }
    }
  }
}

void AgeBandMatrixRatioPtrVector::Migrate(const DoubleMatrix& MI, const AgeBandMatrixPtrVector& Total) {

  DoubleVector tmp(size, 0.0);
  int i, j, age, length, tag;
  int numTagExperiments = tagID.Size();
  if (numTagExperiments > 0) {
    for (age = v[0]->minAge(); age <= v[0]->maxAge(); age++) {
      for (length = v[0]->minLength(age); length < v[0]->maxLength(age); length++) {
        for (tag = 0; tag < numTagExperiments; tag++) {
          for (j = 0; j < size; j++)
            tmp[j] = 0.0;

          for (j = 0; j < size; j++)
            for (i = 0; i < size; i++)
              tmp[j] += *((*v[i])[age][length][tag].N) * MI[j][i];

          for (j = 0; j < size; j++)
            *((*v[j])[age][length][tag].N) = tmp[j];
        }
      }
    }
    for (i = 0; i < size; i++)
      v[i]->updateRatio(Total[i]);
  }
}
