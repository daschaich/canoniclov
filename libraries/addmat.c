// -----------------------------------------------------------------
// Add or subtract irrep matrices
// Unlike CSUM and CDIF, output is always last
// c <-- c + b
// c <-- c - b
// c <-- a + b
// c <-- a - b
#include "../include/config.h"
#include "../include/complex.h"
#include "../include/su3.h"

void sum_su3_matrix(su3_matrix *b, su3_matrix *c) {
  register int i, j;
  for (i = 0; i < DIMF; i++) {
    for (j = 0; j < DIMF; j++) {
      c->e[i][j].real += b->e[i][j].real;
      c->e[i][j].imag += b->e[i][j].imag;
    }
  }
}

void dif_su3_matrix(su3_matrix *b, su3_matrix *c) {
  register int i, j;
  for (i = 0; i < DIMF; i++) {
    for (j = 0; j < DIMF; j++) {
      c->e[i][j].real -= b->e[i][j].real;
      c->e[i][j].imag -= b->e[i][j].imag;
    }
  }
}

void add_su3_matrix(su3_matrix *a, su3_matrix *b, su3_matrix *c) {
  register int i, j;
  for (i = 0; i < DIMF; i++) {
    for (j = 0; j < DIMF; j++) {
      c->e[i][j].real = a->e[i][j].real + b->e[i][j].real;
      c->e[i][j].imag = a->e[i][j].imag + b->e[i][j].imag;
    }
  }
}

void sub_su3_matrix(su3_matrix *a, su3_matrix *b, su3_matrix *c) {
  register int i, j;
  for (i = 0; i < DIMF; i++) {
    for (j = 0; j < DIMF; j++) {
      c->e[i][j].real = a->e[i][j].real - b->e[i][j].real;
      c->e[i][j].imag = a->e[i][j].imag - b->e[i][j].imag;
    }
  }
}
// -----------------------------------------------------------------
