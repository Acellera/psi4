/*
 *@BEGIN LICENSE
 *
 * PSI4: an ab initio quantum chemistry software package
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *@END LICENSE
 */

/*! \file
**  \ingroup DETCI
**  \brief Routines to compute sigma = H * c
**
** Here collect the stuff to calculate the sigma vector within the
** framework of the CI vector class.
** Rewrote lots of stuff to handle the three out-of-core cases better.
** 
** C. David Sherrill
** Center for Computational Quantum Chemistry
** University of Georgia
** February 1996
**
*/


#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <libciomr/libciomr.h>
#include <libqt/qt.h>
#include <libmints/mints.h>
#include "structs.h"
#define EXTERN
#include "globals.h"
#include "civect.h"
#include "ciwave.h"

namespace psi { namespace detci {

extern void transp_sigma(double **a, int rows, int cols, int phase);
//extern void H0block_gather(double **mat, int al, int bl, int cscode, 
//   int mscode, int phase);
extern void b2brepl(unsigned char **occs, int *Jcnt, int **Jij, int **Joij,
   int **Jridx, signed char **Jsgn, struct olsen_graph *Graph,
   int Ilist, int Jlist, int len, struct calcinfo *Cinfo);
extern void b2brepl_test(unsigned char ***occs, int *Jcnt, int **Jij, 
   int **Joij, int **Jridx, signed char **Jsgn, struct olsen_graph *Graph,
   struct calcinfo *Cinfo);
extern void s3_block_bz(int Ialist, int Iblist, int Jalist, 
   int Jblist, int nas, int nbs, int cnas, 
   double *tei, double **C, double **S, double **Cprime, double **Sprime,
   struct calcinfo *CalcInfo, int ***OV);
extern void set_row_ptrs(int rows, int cols, double **matrix);

extern void s1_block_vfci_thread(struct stringwr **alplist, 
   struct stringwr **betlist,
   double **C, double **S, double *oei, double *tei, double *F,
   int nlists, int nas, int nbs, int Ib_list, int Jb_list, 
   int Jb_list_nbs);
extern void s1_block_vfci(struct stringwr **alplist, 
   struct stringwr **betlist,
   double **C, double **S, double *oei, double *tei, double *F,
   int nlists, int nas, int nbs, int Ib_list, int Jb_list, 
   int Jb_list_nbs);
extern void s1_block_vras(struct stringwr **alplist, 
   struct stringwr **betlist, 
   double **C, double **S, double *oei, double *tei, double *F, 
   int nlists, int nas, int nbs, int sbc, int cbc, int cnbs);
extern void s1_block_vras_thread(struct stringwr **alplist, 
   struct stringwr **betlist, 
   double **C, double **S, double *oei, double *tei, double *F, 
   int nlists, int nas, int nbs, int sbc, int cbc, int cnbs);
extern void s1_block_vras_rotf(int *Cnt[2], int **Ij[2], int **Oij[2],
   int **Ridx[2], signed char **Sgn[2], unsigned char **Toccs,
   double **C, double **S,
   double *oei, double *tei, double *F, int nlists, int nas, int nbs,
   int Ib_list, int Jb_list, int Jb_list_nbs);
extern void s2_block_vfci_thread(struct stringwr **alplist, 
   struct stringwr **betlist, 
   double **C, double **S, double *oei, double *tei, double *F,
   int nlists, int nas, int nbs, int Ia_list, int Ja_list, 
   int Ja_list_nas);
extern void s2_block_vfci(struct stringwr **alplist, 
   struct stringwr **betlist, 
   double **C, double **S, double *oei, double *tei, double *F,
   int nlists, int nas, int nbs, int Ia_list, int Ja_list, 
   int Ja_list_nas);
extern void s2_block_vras(struct stringwr **alplist, 
   struct stringwr **betlist, 
   double **C, double **S, double *oei, double *tei, double *F,
   int nlists, int nas, int nbs, int sac, int cac, int cnas);
extern void s2_block_vras_thread(struct stringwr **alplist, 
   struct stringwr **betlist, 
   double **C, double **S, double *oei, double *tei, double *F,
   int nlists, int nas, int nbs, int sac, int cac, int cnas);
extern void s2_block_vras_rotf(int *Cnt[2], int **Ij[2], int **Oij[2],
   int **Ridx[2], signed char **Sgn[2], unsigned char **Toccs,
   double **C, double **S,
   double *oei, double *tei, double *F, int nlists, int nas, int nbs,
   int Ia_list, int Ja_list, int Ja_list_nbs);
extern void s3_block_vdiag(struct stringwr *alplist,
   struct stringwr *betlist,
   double **C, double **S, double *tei, int nas, int nbs, int cnas,
   int Ib_list, int Ja_list, int Jb_list, int Ib_sym, int Jb_sym,
   double **Cprime, double *F, double *V, double *Sgn, int *L, int *R);
extern void s3_block_v(struct stringwr *alplist,struct stringwr *betlist,
   double **C, double **S, double *tei, int nas, int nbs, int cnas,
   int Ib_list, int Ja_list, int Jb_list, int Ib_sym, int Jb_sym,
   double **Cprime, double *F, double *V, double *Sgn, int *L, int *R);
extern void s3_block_vrotf(int *Cnt[2], int **Ij[2], int **Ridx[2],
   signed char **Sn[2], double **C, double **S, 
   double *tei, int nas, int nbs, int cnas,
   int Ib_list, int Ja_list, int Jb_list, int Ib_sym, int Jb_sym,
   double **Cprime, double *F, double *V, double *Sgn, int *L, int *R);
extern void s3_block_vdiag_rotf(int *Cnt[2], int **Ij[2], int **Ridx[2],
   signed char **Sn[2], double **C, double **S, 
   double *tei, int nas, int nbs, int cnas,
   int Ib_list, int Ja_list, int Jb_list, int Ib_sym, int Jb_sym,
   double **Cprime, double *F, double *V, double *Sgn, int *L, int *R);

extern unsigned char ***Occs;
//extern struct olsen_graph *AlphaG;
//extern struct olsen_graph *BetaG;

extern int cc_reqd_sblocks[CI_BLK_MAX];

/* FUNCTION PROTOS THIS MODULE */

void sigma_block(struct stringwr **alplist, struct stringwr **betlist,
      double **cmat, double **smat, double *oei, double *tei, int fci, 
      int cblock, int sblock, int nas, int nbs, int sac, int sbc, 
      int cac, int cbc, int cnas, int cnbs, int cnac, int cnbc, 
      int sbirr, int cbirr, int Ms0);



/* GLOBALS THIS MODULE */

double *F;
int **Jij[2];
int **Joij[2];
int **Jridx[2];
signed char **Jsgn[2];
int *Jcnt[2];
unsigned char **Toccs;
double **transp_tmp = NULL;
double **cprime = NULL, **sprime = NULL;
double *V, *Sgn;
int *L, *R;


/*
** sigma_init()
**
** This function initializes all the globals associated with calculating
** the sigma vector.
**
*/
void CIWavefunction::sigma_init(CIvect& C, CIvect &S, struct stringwr **alplist, 
      struct stringwr **betlist)
{
   int i,j;
   int maxcols=0, maxrows=0;
   int nsingles, max_dim=0;
   unsigned long int bufsz=0;

   if (CalcInfo_->sigma_initialized) {
      printf("(sigma_init): sigma_initialized already set to 1\n");
      return;
      }

   for (i=0; i<C.num_blocks; i++) {
      if (C.Ib_size[i] > max_dim) max_dim = C.Ib_size[i];
      if (C.Ia_size[i] > max_dim) max_dim = C.Ia_size[i];
      }
   F = init_array(max_dim);

   Sgn = init_array(max_dim);
   V = init_array(max_dim);
   L = init_int_array(max_dim);
   R = init_int_array(max_dim);

   if (Parameters_->repl_otf) {
      max_dim += AlphaG_->num_el_expl;
      nsingles = AlphaG_->num_el_expl * AlphaG_->num_orb;
      for (i=0; i<2; i++) {
         Jcnt[i] = init_int_array(max_dim);
         Jij[i] = init_int_matrix(max_dim, nsingles);
         Joij[i] = init_int_matrix(max_dim, nsingles);
         Jridx[i] = init_int_matrix(max_dim, nsingles);
         Jsgn[i] = (signed char **) malloc (max_dim * sizeof(signed char *));
         for (j=0; j<max_dim; j++) { 
            Jsgn[i][j] = (signed char *) malloc (nsingles * 
               sizeof(signed char));
            }
         }

      Toccs = (unsigned char **) malloc (sizeof(unsigned char *) * nsingles);

      /* test out the on-the-fly replacement routines */
      /*
      b2brepl_test(Occs,Jcnt[0],Jij[0],Joij[0],Jridx[0],Jsgn[0],AlphaG);
      */
      }

   /* figure out which C blocks contribute to s */
   s1_contrib_ = init_int_matrix(S.num_blocks, C.num_blocks);
   s2_contrib_ = init_int_matrix(S.num_blocks, C.num_blocks);
   s3_contrib_ = init_int_matrix(S.num_blocks, C.num_blocks);
   if (Parameters_->repl_otf)
      sigma_get_contrib_rotf(C, S, s1_contrib_, s2_contrib_, s3_contrib_,
         Jcnt, Jij, Joij, Jridx, Jsgn, Toccs);
   else  
      sigma_get_contrib(alplist, betlist, C, S, s1_contrib_, s2_contrib_,
         s3_contrib_);

   if ((C.icore==2 && C.Ms0 && CalcInfo_->ref_sym != 0) || (C.icore==0 &&
         C.Ms0)) {
     for (i=0, maxrows=0, maxcols=0; i<C.num_blocks; i++) {
       if (C.Ia_size[i] > maxrows) maxrows = C.Ia_size[i];
       if (C.Ib_size[i] > maxcols) maxcols = C.Ib_size[i];
     }
     if (maxcols > maxrows) maxrows = maxcols;
     transp_tmp = (double **) malloc (maxrows * sizeof(double *));
     if (transp_tmp == NULL) {
       printf("(sigma_init): Trouble with malloc'ing transp_tmp\n");
     }
     bufsz = C.get_max_blk_size();
     transp_tmp[0] = init_array(bufsz);
     if (transp_tmp[0] == NULL) {
       printf("(sigma_init): Trouble with malloc'ing transp_tmp[0]\n");
     }
   }

   /* make room for cprime and sprime if necessary */
   for (i=0, maxrows=0; i<C.num_blocks; i++) {
     if (C.Ia_size[i] > maxrows) maxrows = C.Ia_size[i];
     if (C.Ib_size[i] > maxcols) maxcols = C.Ib_size[i];
   }
   if ((C.icore==2 && C.Ms0 && CalcInfo_->ref_sym != 0) || (C.icore==0 &&
         C.Ms0)) {
     if (maxcols > maxrows) maxrows = maxcols;
   }
   bufsz = C.get_max_blk_size();

   cprime = (double **) malloc (maxrows * sizeof(double *));
   if (cprime == NULL) {
      printf("(sigma_init): Trouble with malloc'ing cprime\n");
   }
   if (C.icore==0 && C.Ms0 && transp_tmp != NULL && transp_tmp[0] != NULL) 
     cprime[0] = transp_tmp[0];
   else 
     cprime[0] = init_array(bufsz);

   if (cprime[0] == NULL) {
     printf("(sigma_init): Trouble with malloc'ing cprime[0]\n");
   }

   if (Parameters_->bendazzoli) {
     sprime = (double **) malloc (maxrows * sizeof(double *));
     if (sprime == NULL) {
       printf("(sigma_init): Trouble with malloc'ing sprime\n");
     }
     sprime[0] = init_array(bufsz);
     if (sprime[0] == NULL) {
       printf("(sigma_init): Trouble with malloc'ing sprime[0]\n");
     }
   }
 
   CalcInfo_->sigma_initialized = 1;
}


/*
** sigma()
**
** Routine to get the sigma vector using the CI vector class
**
** Changed into a master function which calls the appropriate subfunction
**
*/
void CIWavefunction::sigma(struct stringwr **alplist, struct stringwr **betlist,
      CIvect& C, CIvect& S, double *oei, double *tei, int fci, int ivec)
{
   if (!CalcInfo_->sigma_initialized) sigma_init(C, S, alplist, betlist);

   switch (C.icore) {
      case 0: 
         sigma_a(alplist, betlist, C, S, oei, tei, fci, ivec);
         break;
      case 1:
         sigma_b(alplist, betlist, C, S, oei, tei, fci, ivec);
         break;
      case 2:
         sigma_c(alplist, betlist, C, S, oei, tei, fci, ivec);
         break;
      default:
         outfile->Printf( "(sigma): Error, invalid icore option\n");
         break;
      } 

}


      
/*
** sigma_a(): This function computes the sigma vector for a given C vector
**    using the CIvector class.   Is somewhat intelligent about constructing
**    the sigma vector blockwise; will attempt to reduce I/O for out-of-core
**    cases.  This version is for icore==0.
**
** Parameters:
**    alplist  = list of alpha strings with replacements
**    betlist  = same for beta strings
**    C        = current ci vector
**    S        = sigma vector to be computed 
**    oei      = one-electron integrals
**    tei      = two-electron integrals
**    fci      = full-ci flag (helps determine which sigma1 routine called)
**    ivec     = sigma vector number (for write call)
**    
** Notes: assumes M_s = 0 for now
*/
void CIWavefunction::sigma_a(struct stringwr **alplist, struct stringwr **betlist,
      CIvect& C, CIvect& S, double *oei, double *tei, int fci, int ivec)
{

   int buf, cbuf;
   int sblock, cblock, cblock2;  /* id of sigma and C blocks */
   int i,j,k;
   int sac, sbc, nas, nbs;
   int cac, cbc, cnas, cnbs;
   int do_cblock, do_cblock2;
   int cairr, cbirr, sbirr;
   int did_sblock = 0;
   int phase;

   if (!Parameters_->Ms0) phase = 1;
   else phase = ((int) Parameters_->S % 2) ? -1 : 1;

   /* this does a sigma subblock at a time: icore==0 */
   for (buf=0; buf<S.buf_per_vect; buf++) {
      S.zero(); 
      did_sblock = 0;
      sblock = S.buf2blk[buf];
      sac = S.Ia_code[sblock];
      sbc = S.Ib_code[sblock];
      nas = S.Ia_size[sblock];
      nbs = S.Ib_size[sblock];
      sbirr = sbc / BetaG_->subgr_per_irrep;
      if (sprime != NULL) set_row_ptrs(nas, nbs, sprime);

      for (cbuf=0; cbuf<C.buf_per_vect; cbuf++) {
         do_cblock=0; do_cblock2=0;
         cblock=C.buf2blk[cbuf];
         cblock2 = -1;
         cac = C.Ia_code[cblock];
         cbc = C.Ib_code[cblock];
         cbirr = cbc / BetaG_->subgr_per_irrep;
         cairr = cac / AlphaG_->subgr_per_irrep;
         if (C.Ms0) cblock2 = C.decode[cbc][cac];
         cnas = C.Ia_size[cblock];
         cnbs = C.Ib_size[cblock];
         if (s1_contrib_[sblock][cblock] || s2_contrib_[sblock][cblock] ||
             s3_contrib_[sblock][cblock]) do_cblock = 1;
         if (C.buf_offdiag[cbuf] && (s1_contrib_[sblock][cblock2] || 
             s2_contrib_[sblock][cblock2] || s3_contrib_[sblock][cblock2])) 
            do_cblock2 = 1;
         if (C.check_zero_block(cblock)) do_cblock = 0;
         if (cblock2 >= 0 && C.check_zero_block(cblock2)) do_cblock2 = 0;
         if (!do_cblock && !do_cblock2) continue;

         C.read(C.cur_vect, cbuf);

         if (do_cblock) {
            if (cprime != NULL) set_row_ptrs(cnas, cnbs, cprime);
            sigma_block(alplist, betlist, C.blocks[cblock], S.blocks[sblock], 
               oei, tei, fci, cblock, sblock, nas, nbs, sac, sbc, cac, cbc, 
               cnas, cnbs, C.num_alpcodes, C.num_betcodes, sbirr, cbirr,
               S.Ms0);
            did_sblock = 1;
            }

         /* what's with this bcopy stuff?  what's going on?  -DS 6/11/96 */
         /* I think I should copy to cblock2 not cblock */
         if (do_cblock2) {
            C.transp_block(cblock, transp_tmp);
//          bcopy((char *) transp_tmp[0], (char *) C.blocks[cblock][0], 
//            cnas * cnbs * sizeof(double));
//          bcopy is non-ANSI.  memcpy reverses the arguments.
            memcpy((void *) C.blocks[cblock][0], (void *) transp_tmp[0],
              cnas * cnbs * sizeof(double));
            /* set_row_ptrs(cnbs, cnas, C.blocks[cblock]); */
            if (cprime != NULL) set_row_ptrs(cnbs, cnas, cprime);
            sigma_block(alplist, betlist, C.blocks[cblock2], S.blocks[sblock], 
               oei, tei, fci, cblock2, sblock, nas, nbs, sac, sbc, 
               cbc, cac, cnbs, cnas, C.num_alpcodes, C.num_betcodes, sbirr,
               cairr, S.Ms0);
            did_sblock = 1;
            }

         } /* end loop over c buffers */

      if (did_sblock) {
         S.set_zero_block(sblock, 0);
         if (S.Ms0) S.set_zero_block(S.decode[sbc][sac], 0);
         }

      if (S.Ms0 && (sac==sbc)) 
         transp_sigma(S.blocks[sblock], nas, nbs, phase);

      H0block_gather(S.blocks[sblock], sac, sbc, 1, Parameters_->Ms0, phase);

      if (S.Ms0) {
         if ((int) Parameters_->S % 2) S.symmetrize(-1.0, sblock);
         else S.symmetrize(1.0, sblock);
         }
      S.write(ivec, buf);

      } /* end loop over sigma buffers */

}



/*
** sigma_b(): This function computes the sigma vector for a given C vector
**    using the CIvector class.   Is somewhat intelligent about constructing
**    the sigma vector blockwise; will attempt to reduce I/O for out-of-core
**    cases.  This version is for icore=1 (whole vector in-core)
**
** Parameters:
**    alplist  = list of alpha strings with replacements
**    betlist  = same for beta strings
**    C        = current ci vector
**    S        = sigma vector to be computed 
**    oei      = one-electron integrals
**    tei      = two-electron integrals
**    fci      = full-ci flag (helps determine which sigma1 routine called)
**    ivec     = sigma vector number (for write call)
**    
** Notes: I think I removed the M_s = 0 assumption from this one
*/
void CIWavefunction::sigma_b(struct stringwr **alplist, struct stringwr **betlist,
      CIvect& C, CIvect& S, double *oei, double *tei, int fci, int ivec)
{

   int sblock, cblock;  /* id of sigma and C blocks */
   int sac, sbc, nas, nbs;
   int cac, cbc, cnas, cnbs;
   int sbirr, cbirr;
   int did_sblock = 0;
   int phase;

   if (!Parameters_->Ms0) phase = 1;
   else phase = ((int) Parameters_->S % 2) ? -1 : 1;

   S.zero(); 
   C.read(C.cur_vect, 0);

   /* loop over unique sigma subblocks */ 
   for (sblock=0; sblock<S.num_blocks; sblock++) {
      if (Parameters_->cc && !cc_reqd_sblocks[sblock]) continue;
      did_sblock = 0;
      sac = S.Ia_code[sblock];
      sbc = S.Ib_code[sblock];
      nas = S.Ia_size[sblock];
      nbs = S.Ib_size[sblock];
      if (nas==0 || nbs==0) continue;
      if (S.Ms0 && sbc > sac) continue;
      sbirr = sbc / BetaG_->subgr_per_irrep;
      if (sprime != NULL) set_row_ptrs(nas, nbs, sprime);

      for (cblock=0; cblock<C.num_blocks; cblock++) {
         if (C.check_zero_block(cblock)) continue;
         cac = C.Ia_code[cblock];
         cbc = C.Ib_code[cblock];
         cnas = C.Ia_size[cblock];
         cnbs = C.Ib_size[cblock];
         cbirr = cbc / BetaG_->subgr_per_irrep;
         if (s1_contrib_[sblock][cblock] || s2_contrib_[sblock][cblock] ||
             s3_contrib_[sblock][cblock]) {
            if (cprime != NULL) set_row_ptrs(cnas, cnbs, cprime);
            sigma_block(alplist, betlist, C.blocks[cblock], S.blocks[sblock], 
               oei, tei, fci, cblock, sblock, nas, nbs, sac, sbc, 
               cac, cbc, cnas, cnbs, C.num_alpcodes, C.num_betcodes, sbirr,
               cbirr, S.Ms0);
            did_sblock = 1;
            }
         } /* end loop over c blocks */

      if (did_sblock) S.set_zero_block(sblock, 0);

      if (S.Ms0 && (sac==sbc)) 
         transp_sigma(S.blocks[sblock], nas, nbs, phase);
      H0block_gather(S.blocks[sblock], sac, sbc, 1, Parameters_->Ms0, 
         phase);
      } /* end loop over sigma blocks */

      if (S.Ms0) {
         if ((int) Parameters_->S % 2) S.symmetrize(-1.0, 0);
         else S.symmetrize(1.0, 0);
         }

   S.write(ivec, 0);

}


/*
** sigma_c(): This function computes the sigma vector for a given C vector
**    using the CIvector class.   Is somewhat intelligent about constructing
**    the sigma vector blockwise; will attempt to reduce I/O for out-of-core
**    cases.  This version is for icore=2 (irrep at a time)
**
** Parameters:
**    alplist  = list of alpha strings with replacements
**    betlist  = same for beta strings
**    C        = current ci vector
**    S        = sigma vector to be computed 
**    oei      = one-electron integrals
**    tei      = two-electron integrals
**    fci      = full-ci flag (helps determine which sigma1 routine called)
**    ivec     = sigma vector number (for write call)
**    
** Notes: tried to remove Ms=0 assumption
*/
void CIWavefunction::sigma_c(struct stringwr **alplist, struct stringwr **betlist,
      CIvect& C, CIvect& S, double *oei, double *tei, int fci, int ivec)
{

   int buf, cbuf;
   int sblock, cblock, cblock2;  /* id of sigma and C blocks */
   int sairr;                    /* irrep of alpha string for sigma block */
   int cairr;                    /* irrep of alpha string for C block */
   int sbirr, cbirr;
   int i,j,k;
   int sac, sbc, nas, nbs;
   int cac, cbc, cnas, cnbs;
   int did_sblock = 0;
   int phase;

   if (!Parameters_->Ms0) phase = 1;
   else phase = ((int) Parameters_->S % 2) ? -1 : 1;


   for (buf=0; buf<S.buf_per_vect; buf++) {
      sairr = S.buf2blk[buf];
      sbirr = sairr ^ CalcInfo_->ref_sym;
      S.zero();
      for (cbuf=0; cbuf<C.buf_per_vect; cbuf++) {
         C.read(C.cur_vect, cbuf); /* go ahead and assume it will contrib */
         cairr = C.buf2blk[cbuf];
         cbirr = cairr ^ CalcInfo_->ref_sym;

         for (sblock=S.first_ablk[sairr];sblock<=S.last_ablk[sairr];sblock++){
            sac = S.Ia_code[sblock];
            sbc = S.Ib_code[sblock];
            nas = S.Ia_size[sblock];
            nbs = S.Ib_size[sblock];
            did_sblock = 0;

            if (S.Ms0 && (sac < sbc)) continue;
            if (sprime != NULL) set_row_ptrs(nas, nbs, sprime);

            for (cblock=C.first_ablk[cairr]; cblock <= C.last_ablk[cairr];
                  cblock++) {

               cac = C.Ia_code[cblock];
               cbc = C.Ib_code[cblock];
               cnas = C.Ia_size[cblock];
               cnbs = C.Ib_size[cblock];

               if ((s1_contrib_[sblock][cblock] || s2_contrib_[sblock][cblock] ||
                    s3_contrib_[sblock][cblock]) && 
                    !C.check_zero_block(cblock)) {
		  if (cprime != NULL) set_row_ptrs(cnas, cnbs, cprime);
                  sigma_block(alplist, betlist, C.blocks[cblock], 
                     S.blocks[sblock], oei, tei, fci, cblock,
                     sblock, nas, nbs, sac, sbc, cac, cbc, cnas, cnbs, 
                     C.num_alpcodes, C.num_betcodes, sbirr, cbirr, S.Ms0);
                  did_sblock = 1;
                  }

               if (C.buf_offdiag[cbuf]) {
                  cblock2 = C.decode[cbc][cac];
                  if ((s1_contrib_[sblock][cblock2] || 
                       s2_contrib_[sblock][cblock2] ||
                       s3_contrib_[sblock][cblock2]) &&
                      !C.check_zero_block(cblock2)) {
                     C.transp_block(cblock, transp_tmp);
		     if (cprime != NULL) set_row_ptrs(cnbs, cnas, cprime);
                     sigma_block(alplist, betlist, transp_tmp,S.blocks[sblock],
                        oei, tei, fci, cblock2, sblock, nas, nbs, sac, sbc, 
                        cbc, cac, cnbs, cnas, C.num_alpcodes, C.num_betcodes,
                        sbirr, cairr, S.Ms0);
                     did_sblock = 1;
                     }
                  }
               } /* end loop over C blocks in this irrep */

            if (did_sblock) S.set_zero_block(sblock, 0); 
            } /* end loop over sblock */

         } /* end loop over cbuf */

      /* transpose the diagonal sigma subblocks in this irrep */
      for (sblock=S.first_ablk[sairr];sblock<=S.last_ablk[sairr];sblock++){
         sac = S.Ia_code[sblock];
         sbc = S.Ib_code[sblock];
         nas = S.Ia_size[sblock];
         nbs = S.Ib_size[sblock];
         if (S.Ms0 && (sac==sbc)) transp_sigma(S.blocks[sblock], nas, nbs, 
            phase);

         /* also gather the contributions from sigma to the H0block */
         if (!S.Ms0 || sac >= sbc) {
            H0block_gather(S.blocks[sblock], sac, sbc, 1, Parameters_->Ms0,
               phase);
            }
         }

      if (S.Ms0) {
         if ((int) Parameters_->S % 2) S.symmetrize(-1.0, sairr);
         else S.symmetrize(1.0, sairr);
         }
     S.write(ivec, buf);

     } /* end loop over sigma irrep */
}


/* 
** sigma_block()
**
** Calculate the contribution to sigma block sblock from C block cblock
**
*/
void CIWavefunction::sigma_block(struct stringwr **alplist, struct stringwr **betlist,
      double **cmat, double **smat, double *oei, double *tei, int fci, 
      int cblock, int sblock, int nas, int nbs, int sac, int sbc, 
      int cac, int cbc, int cnas, int cnbs, int cnac, int cnbc, 
      int sbirr, int cbirr, int Ms0)
{

   /* SIGMA2 CONTRIBUTION */
  if (s2_contrib_[sblock][cblock]) {

    detci_time.s2_before_time = wall_time_new();

      if (fci) {
          if (Parameters_->nthreads > 1)
              s2_block_vfci_thread(alplist, betlist, cmat, smat, oei, tei, F, 
                 cnac, nas, nbs, sac, cac, cnas);
          else
              s2_block_vfci(alplist, betlist, cmat, smat, oei, tei, F, cnac,
                            nas, nbs, sac, cac, cnas);
        }
      else {
          if (Parameters_->repl_otf) {
              s2_block_vras_rotf(Jcnt, Jij, Joij, Jridx, Jsgn,
                                 Toccs, cmat, smat, oei, tei, F, cnac, 
                                 nas, nbs, sac, cac, cnas);
            }
          else if (Parameters_->nthreads > 1) {
            s2_block_vras_thread(alplist, betlist, cmat, smat, 
                            oei, tei, F, cnac, nas, nbs, sac, cac, cnas);  
            }
          else {
              s2_block_vras(alplist, betlist, cmat, smat, 
                            oei, tei, F, cnac, nas, nbs, sac, cac, cnas);
            }
        }
    detci_time.s2_after_time = wall_time_new();
    detci_time.s2_total_time += detci_time.s2_after_time - detci_time.s2_before_time;

    } /* end sigma2 */

   
   if (Parameters_->print_lvl > 3) {
     outfile->Printf( "s2: Contribution to sblock=%d from cblock=%d\n",
        sblock, cblock);
     print_mat(smat, nas, nbs, "outfile");
   }

   /* SIGMA1 CONTRIBUTION */
   if (!Ms0 || (sac != sbc)) {
    detci_time.s1_before_time = wall_time_new();

      if (s1_contrib_[sblock][cblock]) {
          if (fci) { 
              if (Parameters_->nthreads > 1)
                  s1_block_vfci_thread(alplist, betlist, cmat, smat, oei, tei, F, cnbc,
                                       nas, nbs, sbc, cbc, cnbs);
              else
                  s1_block_vfci(alplist, betlist, cmat, smat, 
                                oei, tei, F, cnbc, nas, nbs, sbc, cbc, cnbs);
            } 
         else {
            if (Parameters_->repl_otf) {
               s1_block_vras_rotf(Jcnt, Jij, Joij, Jridx, Jsgn,
                  Toccs, cmat, smat, oei, tei, F, cnbc, nas, nbs,
                  sbc, cbc, cnbs);
               }
            else if (Parameters_->nthreads > 1) {
               s1_block_vras_thread(alplist, betlist, cmat, smat, oei, tei, F, cnbc, 
                  nas, nbs, sbc, cbc, cnbs);
              }
            else {
               s1_block_vras(alplist, betlist, cmat, smat, oei, tei, F, cnbc, 
                  nas, nbs, sbc, cbc, cnbs);
               }
            } 
         }
          detci_time.s1_after_time = wall_time_new();
          detci_time.s1_total_time += detci_time.s1_after_time - detci_time.s1_before_time;

      } /* end sigma1 */

   if (Parameters_->print_lvl > 3) {
     outfile->Printf( "s1: Contribution to sblock=%d from cblock=%d\n",
        sblock, cblock);
     print_mat(smat, nas, nbs, "outfile");
   }

   /* SIGMA3 CONTRIBUTION */
   if (s3_contrib_[sblock][cblock]) {
      detci_time.s3_before_time = wall_time_new();

      /* zero_mat(smat, nas, nbs); */

      if (!Ms0 || (sac != sbc)) {
         if (Parameters_->repl_otf) {
            b2brepl(Occs[sac], Jcnt[0], Jij[0], Joij[0], Jridx[0],
               Jsgn[0], AlphaG_, sac, cac, nas, &CalcInfo);  
            b2brepl(Occs[sbc], Jcnt[1], Jij[1], Joij[1], Jridx[1],
               Jsgn[1], BetaG_, sbc, cbc, nbs, &CalcInfo);  
            s3_block_vrotf(Jcnt, Jij, Jridx, Jsgn, cmat, smat, tei, nas, nbs,
               cnas, sbc, cac, cbc, sbirr, cbirr, cprime, F, V, Sgn, L, R);
            }      
         else {
            s3_block_v(alplist[sac], betlist[sbc], cmat, smat, tei,
               nas, nbs, cnas, sbc, cac, cbc, sbirr, cbirr, 
               cprime, F, V, Sgn, L, R);
            }
         }

      else if (Parameters_->bendazzoli) {
         s3_block_bz(sac, sbc, cac, cbc, nas, nbs, cnas, tei, cmat, smat, 
            cprime, sprime, CalcInfo_, OV_);
         }

      else {
         if (Parameters_->repl_otf) {
            b2brepl(Occs[sac], Jcnt[0], Jij[0], Joij[0], Jridx[0],
               Jsgn[0], AlphaG_, sac, cac, nas, &CalcInfo);  
            b2brepl(Occs[sbc], Jcnt[1], Jij[1], Joij[1], Jridx[1],
               Jsgn[1], BetaG_, sbc, cbc, nbs, &CalcInfo);  
            s3_block_vdiag_rotf(Jcnt, Jij, Jridx, Jsgn, cmat, smat, tei, 
               nas, nbs, cnas, sbc, cac, cbc, sbirr, cbirr, cprime, F, V,
               Sgn, L, R);
            }      
         else {
            s3_block_vdiag(alplist[sac], betlist[sbc], cmat, smat, tei,
               nas, nbs, cnas, sbc, cac, cbc, sbirr, cbirr, 
               cprime, F, V, Sgn, L, R);
            }
         }

      if (Parameters_->print_lvl > 3) {
        outfile->Printf( "s3: Contribution to sblock=%d from cblock=%d\n",
           sblock, cblock);
        print_mat(smat, nas, nbs, "outfile");
      }
      
      detci_time.s3_after_time = wall_time_new();
      detci_time.s3_total_time += 
         detci_time.s3_after_time - detci_time.s3_before_time;

      } /* end sigma3 */
}

void CIWavefunction::sigma_get_contrib(struct stringwr **alplist, struct stringwr **betlist,
      CIvect &C, CIvect &S, int **s1_contrib, int **s2_contrib,
      int **s3_contrib)
{

   int sblock,cblock;
   int sac, sbc, cac, cbc;
   int nas, nbs;
   struct stringwr *Ib, *Ia, *Kb, *Ka;
   unsigned int Ibidx, Iaidx, Kbidx, Kaidx, Ib_ex, Ia_ex;
   unsigned int Ibcnt, Iacnt, *Ibridx, *Iaridx;
   int Kb_list, Ka_list;
   int found,i,j;

   for (sblock=0; sblock<S.num_blocks; sblock++) {
      sac = S.Ia_code[sblock];
      sbc = S.Ib_code[sblock];
      nas = S.Ia_size[sblock];
      nbs = S.Ib_size[sblock];
      for (cblock=0; cblock<C.num_blocks; cblock++) {
         cac = C.Ia_code[cblock];
         cbc = C.Ib_code[cblock];


         /* does this c block contribute to sigma1? */
         if (sac == cac) {
            for (Ib=betlist[sbc], Ibidx=0, found=0; Ibidx < nbs && !found;
               Ibidx++, Ib++) {
               /* loop over excitations E^b_{kl} from |B(I_b)> */
               for (Kb_list=0; Kb_list < S.num_betcodes && !found; Kb_list++) {
                  Ibcnt = Ib->cnt[Kb_list];
                  Ibridx = Ib->ridx[Kb_list];
                  for (Ib_ex=0; Ib_ex < Ibcnt; Ib_ex++) {
                     Kbidx = *Ibridx++;
                     Kb = betlist[Kb_list] + Kbidx;
                     if (Kb->cnt[cbc]) { found=1;  break; }
                     }
                  }
               }
            if (found) s1_contrib[sblock][cblock] = 1;
            }

         /* does this c block contribute to sigma2? */
         if (sbc == cbc) {
         for (Ia=alplist[sac], Iaidx=0, found=0; Iaidx < nas && !found;
               Iaidx++, Ia++) {
               /* loop over excitations E^a_{kl} from |A(I_a)> */
               for (Ka_list=0; Ka_list < S.num_alpcodes && !found; Ka_list++) {
                  Iacnt = Ia->cnt[Ka_list];
                  Iaridx = Ia->ridx[Ka_list];
                  for (Ia_ex=0; Ia_ex < Iacnt; Ia_ex++) {
                     Kaidx = *Iaridx++;
                     Ka = alplist[Ka_list] + Kaidx;
                     if (Ka->cnt[cac]) { found=1;  break; }
                     }
                  }
               }
            if (found) s2_contrib[sblock][cblock] = 1;
            }

         /* does this c block contribute to sigma3? */
         for (Iaidx=0,found=0; Iaidx<S.Ia_size[sblock]; Iaidx++) {
            if (alplist[sac][Iaidx].cnt[cac]) found=1;
            }
         if (found) { /* see if beta is ok */
            found=0;
            for (Ibidx=0; Ibidx<S.Ib_size[sblock]; Ibidx++) {
               if (betlist[sbc][Ibidx].cnt[cbc]) found=1;
               }
            if (found)
               s3_contrib[sblock][cblock] = 1;
            }

         } /* end loop over c blocks */
      } /* end loop over sigma blocks */

   if (Parameters.print_lvl > 4) {
     printf("\nSigma 1:\n");
     for (i=0; i<S.num_blocks; i++) {
       outfile->Printf( "Contributions to sigma block %d\n", i);
       for (j=0; j<C.num_blocks; j++) {
         if (s1_contrib[i][j]) outfile->Printf( "%3d ", j);
       }
       outfile->Printf( "\n");
     }

     printf("\n\nSigma 2:\n");
     for (i=0; i<S.num_blocks; i++) {
       outfile->Printf( "Contributions to sigma block %d\n", i);
       for (j=0; j<C.num_blocks; j++) {
         if (s2_contrib[i][j]) outfile->Printf( "%3d ", j);
       }
       outfile->Printf( "\n");
     }

     printf("\n\nSigma 3:\n");
     for (i=0; i<S.num_blocks; i++) {
       outfile->Printf( "Contributions to sigma block %d\n", i);
       for (j=0; j<C.num_blocks; j++) {
         if (s3_contrib[i][j]) outfile->Printf( "%3d ", j);
       }
       outfile->Printf( "\n");
     }
   }

}

void CIWavefunction::sigma_get_contrib_rotf(CIvect &C, CIvect &S,
      int **s1_contrib, int **s2_contrib, int **s3_contrib,
      int *Cnt[2], int **Ij[2], int **Oij[2], int **Ridx[2],
      signed char **Sgn[2], unsigned char **Toccs)
{

   int sblock,cblock;
   int sac, sbc, cac, cbc;
   int nas, nbs;
   int Ibidx, Iaidx, Ib_ex, Ia_ex;
   int Ibcnt, Iacnt;
   int Kb_list, Ka_list;
   int found,i,j;

   for (sblock=0; sblock<S.num_blocks; sblock++) {
      sac = S.Ia_code[sblock];
      sbc = S.Ib_code[sblock];
      nas = S.Ia_size[sblock];
      nbs = S.Ib_size[sblock];
      for (cblock=0; cblock<C.num_blocks; cblock++) {
         cac = C.Ia_code[cblock];
         cbc = C.Ib_code[cblock];


         /* does this c block contribute to sigma1? */
         if (sac == cac) {
            found = 0;
            for (Kb_list=0; Kb_list < S.num_betcodes && !found; Kb_list++) {
               b2brepl(Occs[sbc], Cnt[0], Ij[0], Oij[0], Ridx[0],
                  Sgn[0], BetaG, sbc, Kb_list, nbs, &CalcInfo);
               for (Ibidx=0; Ibidx < nbs && !found; Ibidx++) {
                  Ibcnt = Cnt[0][Ibidx];
                  if (Ibcnt) {
                     for (i=0; i<Ibcnt; i++) {
                        j = Ridx[0][Ibidx][i];
                        Toccs[i] = Occs[Kb_list][j];
                        }
                     b2brepl(Toccs, Cnt[1], Ij[1], Oij[1], Ridx[1], Sgn[1],
                        BetaG, Kb_list, cbc, Ibcnt, &CalcInfo);
                     for (Ib_ex=0; Ib_ex < Ibcnt; Ib_ex++) {
                        if (Cnt[1][Ib_ex]) { found=1; break; }
                        }
                     }
                  }
               }
            if (found) s1_contrib[sblock][cblock] = 1;
            }

         /* does this c block contribute to sigma2? */
         if (sbc == cbc) {
            found = 0;
            for (Ka_list=0; Ka_list < S.num_alpcodes && !found; Ka_list++) {
               b2brepl(Occs[sac], Cnt[0], Ij[0], Oij[0], Ridx[0],
                  Sgn[0], AlphaG, sac, Ka_list, nas, &CalcInfo);
               for (Iaidx=0; Iaidx < nas && !found; Iaidx++) {
                  Iacnt = Cnt[0][Iaidx];
                  if (Iacnt) {
                     for (i=0; i<Iacnt; i++) {
                        j = Ridx[0][Iaidx][i];
                        Toccs[i] = Occs[Ka_list][j];
                        }
                     b2brepl(Toccs, Cnt[1], Ij[1], Oij[1], Ridx[1], Sgn[1],
                        AlphaG, Ka_list, cac, Iacnt, &CalcInfo);
                     for (Ia_ex=0; Ia_ex < Iacnt; Ia_ex++) {
                        if (Cnt[1][Ia_ex]) { found=1; break; }
                        }
                     }
                  }
               }
            if (found) s2_contrib[sblock][cblock] = 1;
            }

         /* does this c block contribute to sigma3? */
         b2brepl(Occs[sac], Cnt[0], Ij[0], Oij[0], Ridx[0],
            Sgn[0], AlphaG, sac, cac, nas, &CalcInfo);
         for (Iaidx=0,found=0; Iaidx<S.Ia_size[sblock]; Iaidx++) {
            if (Cnt[0][Iaidx]) found=1;
            }
         if (found) { /* see if beta is ok */
            found=0;
            b2brepl(Occs[sbc], Cnt[0], Ij[0], Oij[0], Ridx[0],
               Sgn[0], BetaG, sbc, cbc, nbs, &CalcInfo);
            for (Ibidx=0; Ibidx<S.Ib_size[sblock]; Ibidx++) {
               if (Cnt[0][Ibidx]) found=1;
               }
            if (found) s3_contrib[sblock][cblock] = 1;
            }
         } /* end loop over c blocks */
      } /* end loop over sigma blocks */

   if (Parameters.print_lvl > 3) {
     printf("\nSigma 1:\n");
     for (i=0; i<S.num_blocks; i++) {
       outfile->Printf( "Contributions to sigma block %d\n", i);
       for (j=0; j<C.num_blocks; j++) {
         if (s1_contrib[i][j]) outfile->Printf( "%3d ", j);
       }
       outfile->Printf( "\n");
     }

     printf("\n\nSigma 2:\n");
     for (i=0; i<S.num_blocks; i++) {
       outfile->Printf( "Contributions to sigma block %d\n", i);
       for (j=0; j<C.num_blocks; j++) {
         if (s2_contrib[i][j]) outfile->Printf( "%3d ", j);
       }
       outfile->Printf( "\n");
     }

     printf("\n\nSigma 3:\n");
     for (i=0; i<S.num_blocks; i++) {
       outfile->Printf( "Contributions to sigma block %d\n", i);
       for (j=0; j<C.num_blocks; j++) {
         if (s3_contrib[i][j]) outfile->Printf( "%3d ", j);
       }
       outfile->Printf( "\n");
     }
   }

}
 

}} // namespace psi::detci

