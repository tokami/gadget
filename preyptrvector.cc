#include "preyptrvector.h"
#include "gadget.h"

#ifndef INLINE_VECTORS
#include "preyptrvector.icc"
#endif

Preyptrvector::Preyptrvector(int sz) {
  size = (sz > 0 ? sz : 0);
  if (size > 0)
    v = new Prey*[size];
  else
    v = 0;
}

Preyptrvector::Preyptrvector(int sz, Prey* value) {
  size = (sz > 0 ? sz : 0);
  int i;
  if (size > 0) {
    v = new Prey*[size];
    for (i = 0; i < size; i++)
      v[i] = value;
  } else
    v = 0;
}

Preyptrvector::Preyptrvector(const Preyptrvector& initial) {
  size = initial.size;
  int i;
  if (size > 0) {
    v = new Prey*[size];
    for (i = 0; i < size; i++)
      v[i] = initial.v[i];
  } else
    v = 0;
}

//The function resize add addsize elements to a Preyptrvector and fills it vith value.
void Preyptrvector::resize(int addsize, Prey* value) {
  int oldsize = size;
  this->resize(addsize);
  int i;
  if (addsize > 0)
    for (i = oldsize; i < size; i++)
      v[i] = value;
}

void Preyptrvector::resize(int addsize) {
  int i;
  if (v == 0) {
    size = addsize;
    v = new Prey*[size];
  } else if (addsize > 0) {
    Prey** vnew = new Prey*[addsize + size];
    for (i = 0; i < size; i++)
      vnew[i] = v[i];
    delete[] v;
    v = vnew;
    size += addsize;
  }
}

void Preyptrvector::Delete(int pos) {
  assert(size > 0);
  assert(0 <= pos && pos < size);
  Prey** vnew = new Prey*[size - 1];
  int i;
  for (i = 0; i < pos; i++)
    vnew[i] = v[i];
  for (i = pos; i < size - 1; i++)
    vnew[i] = v[i + 1];
  delete[] v;
  v = vnew;
  size--;
}