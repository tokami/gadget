#include "intvector.h"
#include "gadget.h"

#ifndef INLINE_VECTORS
#include "intvector.icc"
#endif

intvector::intvector(int sz) {
  size = (sz > 0 ? sz : 0);
  if (size > 0)
    v = new int[size];
  else
    v = 0;
}

intvector::intvector(int sz, int value) {
  size = (sz > 0 ? sz : 0);
  int i;
  if (size > 0) {
    v = new int[size];
    for (i = 0; i < size; i++)
      v[i] = value;
  } else
    v = 0;
}

intvector::intvector(const intvector& initial) {
  size = initial.size;
  int i;
  if (size > 0) {
    v = new int[size];
    for (i = 0; i < size; i++)
      v[i] = initial.v[i];
  } else
    v = 0;
}

//The function resize add addsize elements to a intvector and fills it vith value.
void intvector::resize(int addsize, int value) {
  int oldsize = size;
  this->resize(addsize);
  int i;
  if (addsize > 0)
    for (i = oldsize; i < size; i++)
      v[i] = value;
}

void intvector::resize(int addsize) {
  int i;
  if (v == 0) {
    size = addsize;
    v = new int[size];
  } else if (addsize > 0) {
    int* vnew = new int[addsize + size];
    for (i = 0; i < size; i++)
      vnew[i] = v[i];
    delete[] v;
    v = vnew;
    size += addsize;
  }
}

void intvector::Delete(int pos) {
  assert(size > 0);
  assert(0 <= pos && pos < size);
  int* vnew = new int[size - 1];
  int i;
  for (i = 0; i < pos; i++)
    vnew[i] = v[i];
  for (i = pos; i < size - 1; i++)
    vnew[i] = v[i + 1];
  delete[] v;
  v = vnew;
  size--;
}