/******************  cs_m_a_mat_f.c  (in su3.a) *************************
*									*
*  c_scalar_mult_add_su3mat_f( su3_matrix_f *ma, su3_matrix_f *m2,	*
*	complex *phase, su3_matrix_f *m3)				*
*  multiply an su3 matrix by a complex scalar and add it to another	*
*  matrix:   m3 <- m1 + number*m2 					*
*/
#include "../include/config.h"
#include "../include/complex.h"
#include "../include/su3.h"

void c_scalar_mult_add_su3mat_f( su3_matrix_f *m1, su3_matrix_f *m2,
	complex *phase, su3_matrix_f *m3){

#ifndef NATIVEDOUBLE
register int i,j;
complex t;
    for(i=0;i<NCOL;i++)for(j=0;j<NCOL;j++){
	t = cmul(&m2->e[i][j],phase);
	m3->e[i][j] = cadd(&m1->e[i][j],&t);
    }

#else
register int i,j;
register double sr,si,br,bi,cr,ci;

    sr = (*phase).real; si = (*phase).imag;

    for(i=0;i<NCOL;i++)for(j=0;j<NCOL;j++){
	br=m2->e[i][j].real; bi=m2->e[i][j].imag;

	cr = sr*br - si*bi;
	ci = sr*bi + si*br;

	m3->e[i][j].real = m1->e[i][j].real + cr;
	m3->e[i][j].imag = m1->e[i][j].imag + ci;
    }
#endif
}
