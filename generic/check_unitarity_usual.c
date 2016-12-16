/*********************** check_unitarity.c ***************************/
/* MIMD version 7 */
/* Claude Bernard, original version */
/* Modified 7/96 DT to quit after finding violation */
/* 9/4/96 DT added Urs' row orthogonality checks, if STRONG defined */
/* 11/2/98 CD fixed return value of max deviation */
/* 01/20/00 UMH combined with Schroedinger functional version */

#define STRONG	/* check row orthogonality as well as norms */

/* Check unitarity of link matrices, quit if not unitary */

#include "generic_includes.h"

#define TOLERANCE (0.0001)
/*#define UNIDEBUG */
Real check_su3(su3_matrix_f *c);

Real check_unitarity() {
register int i,dir;
int ii,jj;
register site *s;
register su3_matrix_f *mat;
Real deviation,max_deviation;
double av_deviation;
 union {
   Real fval;
   int ival;
 } ifval;
 
 max_deviation=av_deviation=0;
 FORALLSITES(i,s){
#ifdef SCHROED_FUN
   for(dir=XUP; dir<=TUP; dir++ ) if(dir==TUP || s->t>0){
#else
   for(dir=XUP; dir<=TUP; dir++ ){
#endif
     mat = (su3_matrix_f *)&(s->linkf[dir]);
     deviation=check_su3( mat );
     if (deviation>TOLERANCE){
       printf("Unitarity problem on node %d, site %d, dir %d, deviation=%f\n",
	      mynode(),i,dir,deviation);
       printf("SU(N) matrix:\n");
       for(ii=0;ii<NCOL;ii++){
	 for(jj=0;jj<NCOL;jj++){
	   printf("%f ",(*mat).e[ii][jj].real); 
	   printf("%f ",(*mat).e[ii][jj].imag); 
	 }
	 printf("\n");
       }
       printf("repeat in hex:\n");
       for(ii=0;ii<NCOL;ii++){
	 for(jj=0;jj<NCOL;jj++){
	   ifval.fval = (*mat).e[ii][jj].real; 
	   printf("%08x ", ifval.ival); 
	   ifval.fval = (*mat).e[ii][jj].imag; 
	   printf("%08x ", ifval.ival); 
	 }
	 printf("\n");
       }
       printf("  \n \n");
       fflush(stdout); terminate(1);
     }
     if(max_deviation<deviation) max_deviation=deviation;
     av_deviation += deviation*deviation;
   }
 }
 av_deviation = sqrt(av_deviation/(4*i));
#ifdef UNIDEBUG
 printf("Deviation from unitarity on node %d: max %g, avrg %g\n",
	mynode(), max_deviation, av_deviation);
#endif
 if(max_deviation> TOLERANCE) 
   printf("Unitarity problem on node %d, maximum deviation=%f\n",
	  mynode(),max_deviation);
 return max_deviation;
}  /*check_unitarity() */

Real check_su3(su3_matrix_f *c) {
  register Real ar, ai, ari, max;
  register int i;
  
  /* first normalize row */
  for(i=0 , max=0.; i<NCOL; ++i) {
    ar = (*c).e[i][0].real * (*c).e[i][0].real +    /* sum of squares of row */
      (*c).e[i][0].imag * (*c).e[i][0].imag +
      (*c).e[i][1].real * (*c).e[i][1].real +
      (*c).e[i][1].imag * (*c).e[i][1].imag 
#if (NCOL>2)
     +(*c).e[i][2].real * (*c).e[i][2].real +
      (*c).e[i][2].imag * (*c).e[i][2].imag
#if (NCOL>3)
     +(*c).e[i][3].real * (*c).e[i][3].real +
      (*c).e[i][3].imag * (*c).e[i][3].imag
#endif
#endif
;
    ar =  fabs( sqrt((double)ar) - 1.);
    if(max<ar) max=ar;
  }
  
#ifdef STRONG
  
  /* Test orthogonality of row 0 and row 1 */
  ar = (*c).e[0][0].real * (*c).e[1][0].real +     /* real part of 0 dot 1 */
    (*c).e[0][0].imag * (*c).e[1][0].imag +
    (*c).e[0][1].real * (*c).e[1][1].real +
    (*c).e[0][1].imag * (*c).e[1][1].imag 
#if (NCOL>2)
   +(*c).e[0][2].real * (*c).e[1][2].real +
    (*c).e[0][2].imag * (*c).e[1][2].imag
#if (NCOL>3)
   +(*c).e[0][3].real * (*c).e[1][3].real +
    (*c).e[0][3].imag * (*c).e[1][3].imag
#endif
#endif
;
  ai = (*c).e[0][0].real * (*c).e[1][0].imag -     /* imag part of 0 dot 1 */
    (*c).e[0][0].imag * (*c).e[1][0].real +
    (*c).e[0][1].real * (*c).e[1][1].imag -
    (*c).e[0][1].imag * (*c).e[1][1].real 
#if (NCOL>2)
   +(*c).e[0][2].real * (*c).e[1][2].imag -
    (*c).e[0][2].imag * (*c).e[1][2].real
#if (NCOL>3)
   +(*c).e[0][3].real * (*c).e[1][3].imag -
    (*c).e[0][3].imag * (*c).e[1][3].real
#endif
#endif
;
  
  ari = sqrt((double)(ar*ar + ai*ai));
  if(max<ari) max=ari;
  
#if (NCOL>2)
  /* Test orthogonality of row 0 and row 2 */
  ar = (*c).e[0][0].real * (*c).e[2][0].real +     /* real part of 0 dot 2 */
    (*c).e[0][0].imag * (*c).e[2][0].imag +
    (*c).e[0][1].real * (*c).e[2][1].real +
    (*c).e[0][1].imag * (*c).e[2][1].imag +
    (*c).e[0][2].real * (*c).e[2][2].real +
    (*c).e[0][2].imag * (*c).e[2][2].imag
#if (NCOL>3)
   +(*c).e[0][3].real * (*c).e[2][3].real +
    (*c).e[0][3].imag * (*c).e[2][3].imag
#endif
;
  ai = (*c).e[0][0].real * (*c).e[2][0].imag -     /* imag part of 0 dot 2 */
    (*c).e[0][0].imag * (*c).e[2][0].real +
    (*c).e[0][1].real * (*c).e[2][1].imag -
    (*c).e[0][1].imag * (*c).e[2][1].real +
    (*c).e[0][2].real * (*c).e[2][2].imag -
    (*c).e[0][2].imag * (*c).e[2][2].real
#if (NCOL>3)
   +(*c).e[0][3].real * (*c).e[2][3].imag -
    (*c).e[0][3].imag * (*c).e[2][3].real
#endif
;
 
  ari = sqrt((double)(ar*ar + ai*ai));
  if(max<ari) max=ari;
  
  /* Test orthogonality of row 1 and row 2 */
  ar = (*c).e[1][0].real * (*c).e[2][0].real +     /* real part of 1 dot 2 */
    (*c).e[1][0].imag * (*c).e[2][0].imag +
    (*c).e[1][1].real * (*c).e[2][1].real +
    (*c).e[1][1].imag * (*c).e[2][1].imag +
    (*c).e[1][2].real * (*c).e[2][2].real +
    (*c).e[1][2].imag * (*c).e[2][2].imag
#if (NCOL>3)
   +(*c).e[1][3].real * (*c).e[2][3].real +
    (*c).e[1][3].imag * (*c).e[2][3].imag
#endif
;
  ai = (*c).e[1][0].real * (*c).e[2][0].imag -     /* imag part of 1 dot 2 */
    (*c).e[1][0].imag * (*c).e[2][0].real +
    (*c).e[1][1].real * (*c).e[2][1].imag -
    (*c).e[1][1].imag * (*c).e[2][1].real +
    (*c).e[1][2].real * (*c).e[2][2].imag -
    (*c).e[1][2].imag * (*c).e[2][2].real
#if (NCOL>3)
   +(*c).e[1][3].real * (*c).e[2][3].imag -
    (*c).e[1][3].imag * (*c).e[2][3].real
#endif
;
  
  ari = sqrt((double)(ar*ar + ai*ai));
  if(max<ari) max=ari;
  
#if (NCOL>3)
  /* Test orthogonality of row 0 and row 3 */
  ar = (*c).e[0][0].real * (*c).e[3][0].real +     /* real part of 0 dot 3 */
    (*c).e[0][0].imag * (*c).e[3][0].imag +
    (*c).e[0][1].real * (*c).e[3][1].real +
    (*c).e[0][1].imag * (*c).e[3][1].imag +
    (*c).e[0][2].real * (*c).e[3][2].real +
    (*c).e[0][2].imag * (*c).e[3][2].imag
   +(*c).e[0][3].real * (*c).e[3][3].real +
    (*c).e[0][3].imag * (*c).e[3][3].imag
;
  ai = (*c).e[0][0].real * (*c).e[3][0].imag -     /* imag part of 0 dot 3 */
    (*c).e[0][0].imag * (*c).e[3][0].real +
    (*c).e[0][1].real * (*c).e[3][1].imag -
    (*c).e[0][1].imag * (*c).e[3][1].real +
    (*c).e[0][2].real * (*c).e[3][2].imag -
    (*c).e[0][2].imag * (*c).e[3][2].real
   +(*c).e[0][3].real * (*c).e[3][3].imag -
    (*c).e[0][3].imag * (*c).e[3][3].real
;
 
  ari = sqrt((double)(ar*ar + ai*ai));
  if(max<ari) max=ari;
  
  /* Test orthogonality of row 1 and row 3 */
  ar = (*c).e[1][0].real * (*c).e[3][0].real +     /* real part of 1 dot 3 */
    (*c).e[1][0].imag * (*c).e[3][0].imag +
    (*c).e[1][1].real * (*c).e[3][1].real +
    (*c).e[1][1].imag * (*c).e[3][1].imag +
    (*c).e[1][2].real * (*c).e[3][2].real +
    (*c).e[1][2].imag * (*c).e[3][2].imag
   +(*c).e[1][3].real * (*c).e[3][3].real +
    (*c).e[1][3].imag * (*c).e[3][3].imag
;
  ai = (*c).e[1][0].real * (*c).e[3][0].imag -     /* imag part of 1 dot 3 */
    (*c).e[1][0].imag * (*c).e[3][0].real +
    (*c).e[1][1].real * (*c).e[3][1].imag -
    (*c).e[1][1].imag * (*c).e[3][1].real +
    (*c).e[1][2].real * (*c).e[3][2].imag -
    (*c).e[1][2].imag * (*c).e[3][2].real
   +(*c).e[1][3].real * (*c).e[3][3].imag -
    (*c).e[1][3].imag * (*c).e[3][3].real
;
  
  /* Test orthogonality of row 2 and row 3 */
  ar = (*c).e[2][0].real * (*c).e[3][0].real +     /* real part of 2 dot 3 */
    (*c).e[2][0].imag * (*c).e[3][0].imag +
    (*c).e[2][1].real * (*c).e[3][1].real +
    (*c).e[2][1].imag * (*c).e[3][1].imag +
    (*c).e[2][2].real * (*c).e[3][2].real +
    (*c).e[2][2].imag * (*c).e[3][2].imag
   +(*c).e[2][3].real * (*c).e[3][3].real +
    (*c).e[2][3].imag * (*c).e[3][3].imag
;
  ai = (*c).e[2][0].real * (*c).e[3][0].imag -     /* imag part of 2 dot 3 */
    (*c).e[2][0].imag * (*c).e[3][0].real +
    (*c).e[2][1].real * (*c).e[3][1].imag -
    (*c).e[2][1].imag * (*c).e[3][1].real +
    (*c).e[2][2].real * (*c).e[3][2].imag -
    (*c).e[2][2].imag * (*c).e[3][2].real
   +(*c).e[2][3].real * (*c).e[3][3].imag -
    (*c).e[2][3].imag * (*c).e[3][3].real
;
  
  ari = sqrt((double)(ar*ar + ai*ai));
  if(max<ari) max=ari;
#endif
#endif
  
#endif /*STRONG*/
  
  return(max);
  
} /* check_su3 */
