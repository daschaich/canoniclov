// -----------------------------------------------------------------
// Compute the staple
#include "pg_includes.h"
#include "../include/loopend.h"

void dsdu_qhb(int dir, int parity) {
  register int i, dir2, otherparity = 0;
  register site *s;
  msg_tag *tag0, *tag1, *tag2, *tag3;
  int start;
  matrix_f tmat;

  switch(parity) {
    case EVEN:       otherparity = ODD;        break;
    case ODD:        otherparity = EVEN;       break;
    case EVENANDODD: otherparity = EVENANDODD; break;
  }

  /* Loop over other directions, computing force from plaquettes in
     the dir,dir2 plane */
  start = 1; /* indicates staple sum not initialized */
  FORALLUPDIR(dir2) {
    if (dir2 == dir)
      continue;

    /* get link[dir2] from direction dir on other parity */
    tag0 = start_gather_site(F_OFFSET(linkf[dir2]), sizeof(matrix_f),
                             dir, otherparity, gen_pt[0]);

    /* get link[dir2] from direction dir */
    tag1 = start_gather_site(F_OFFSET(linkf[dir2]), sizeof(matrix_f),
                             dir, parity, gen_pt[1]);

    /* get link[dir] from direction dir2 */
    tag2 = start_gather_site(F_OFFSET(linkf[dir]), sizeof(matrix_f),
                             dir2, parity, gen_pt[2]);

    /* Lower staple (computed at backward site) */
    wait_gather(tag0);
    FORSOMEPARITY(i, s, otherparity) {
      mult_an_f(&(s->linkf[dir2]), &(s->linkf[dir]), &tmat);
      mult_nn_f(&tmat, (matrix_f *)gen_pt[0][i], &(s->tempmat));
    } END_LOOP
    cleanup_gather(tag0);
    /* get tempmat from direction -dir2 */
    tag3 = start_gather_site(F_OFFSET(tempmat), sizeof(matrix_f),
                             OPP_DIR(dir2), parity, gen_pt[3]);

    /* Upper staple */
    wait_gather(tag1);
    wait_gather(tag2);
    if (start) {  /* this is the first contribution to staple */
      FORSOMEPARITY(i, s, parity) {
        mult_nn_f(&(s->linkf[dir2]), (matrix_f *)gen_pt[2][i], &tmat);
        mult_na_f(&tmat, (matrix_f *)gen_pt[1][i], &(s->staple));
      } END_LOOP
      start = 0;
    }
    else {
      FORSOMEPARITY(i, s, parity) {
        mult_nn_f(&(s->linkf[dir2]), (matrix_f *)gen_pt[2][i], &tmat);
        mult_na_sum_f(&tmat, (matrix_f *)gen_pt[1][i], &(s->staple));
      } END_LOOP
    }
    cleanup_gather(tag1);
    cleanup_gather(tag2);

    /* Add lower staple */
    wait_gather(tag3);
    FORSOMEPARITY(i, s, parity) {
      sum_mat_f((matrix_f *)gen_pt[3][i], &(s->staple));
    } END_LOOP
    cleanup_gather(tag3);
  }
}
// -----------------------------------------------------------------
