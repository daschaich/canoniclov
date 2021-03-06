/******************  adjoint.c  (in su3.a) **************************
*									*
* void adjoint( matrix *a, matrix *b )			*
* B  <- A_adjoint,  adjoint of an SU3 matrix 				*
*/
#include "../include/config.h"
#include "../include/complex.h"
#include "../include/su3.h"

/* adjoint of an SU3 matrix */
void adjoint( matrix *a, matrix *b ){
register int i,j;
    for(i=0;i<DIMF;i++)for(j=0;j<DIMF;j++){
	CONJG( a->e[j][i], b->e[i][j] );
    }
}
