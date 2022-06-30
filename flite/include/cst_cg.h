/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                        Copyright (c) 2007                             */
/*                        All Rights Reserved.                           */
/*                                                                       */
/*  Permission is hereby granted, free of charge, to use and distribute  */
/*  this software and its documentation without restriction, including   */
/*  without limitation the rights to use, copy, modify, merge, publish,  */
/*  distribute, sublicense, and/or sell copies of this work, and to      */
/*  permit persons to whom this work is furnished to do so, subject to   */
/*  the following conditions:                                            */
/*   1. The code must retain the above copyright notice, this list of    */
/*      conditions and the following disclaimer.                         */
/*   2. Any modifications must be clearly marked as such.                */
/*   3. Original authors' names are not deleted.                         */
/*   4. The authors' names are not used to endorse or promote products   */
/*      derived from this software without specific prior written        */
/*      permission.                                                      */
/*                                                                       */
/*  CARNEGIE MELLON UNIVERSITY AND THE CONTRIBUTORS TO THIS WORK         */
/*  DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING      */
/*  ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT   */
/*  SHALL CARNEGIE MELLON UNIVERSITY NOR THE CONTRIBUTORS BE LIABLE      */
/*  FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES    */
/*  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN   */
/*  AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,          */
/*  ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF       */
/*  THIS SOFTWARE.                                                       */
/*                                                                       */
/*************************************************************************/
/*             Author:  Alan W Black (awb@cs.cmu.edu)                    */
/*               Date:  November 2007                                    */
/*************************************************************************/
/*                                                                       */
/*  clustergen db                                                        */
/*                                                                       */
/*  A statistical corpus based synthesizer.                              */
/*  See Black, A. (2006), CLUSTERGEN: A Statistical Parametric           */
/*  Synthesizer using Trajectory Modeling", Interspeech 2006 - ICSLP,    */
/*  Pittsburgh, PA.                                                      */
/*  http://www.cs.cmu.edu/~awb/papers/is2006/IS061394.PDF                */
/*                                                                       */
/*************************************************************************/
#ifndef _CST_CG_H__
#define _CST_CG_H__

#include "cst_cart.h"
#include "cst_track.h"
#include "cst_wave.h"
#include "cst_audio.h"
#include "cst_synth.h" /* for dur_stat */

typedef struct cst_cg_db_struct {
    /* Please do not change this structure, but if you do only add things
       to the end of the struct.  If you change please modify dump/load  
       voice too (in cst_cg_dump_voice and cst_cg_map) */
    const char *name;
    const char * const *types;
    int num_types;

    int sample_rate;

    float f0_mean, f0_stddev;

    /* Cluster trees */
    const cst_cart * const *f0_trees; 

    int num_param_models;
    const cst_cart *** param_trees;

    const cst_cart *spamf0_accent_tree; /* spam accent tree */
    const cst_cart *spamf0_phrase_tree; /* spam phrase tree */

    /* Model params e.g. mceps, deltas intersliced with stddevs */
    int *num_channels;
    int *num_frames;
    const unsigned short *** model_vectors;

    int num_channels_spamf0_accent;
    int num_frames_spamf0_accent;
    const float * const * spamf0_accent_vectors;

    /* Currently shared between different models */
    const float *model_min;    /* for vector coeffs encoding */
    const float *model_range;  /* for vector coeffs encoding */

    float frame_advance; 

    /* duration models (cart + phonedurs) */
    int num_dur_models;
    const dur_stat *** dur_stats;
    const cst_cart ** dur_cart;

    /* phone to states map */
    const char * const * const *phone_states;

    /* Other parameters */    
    int do_mlpg;  /* implies deltas are in the model_vectors */
    float *dynwin;
    int dynwinsize;

    float mlsa_alpha;
    float mlsa_beta;

    int multimodel;
    int mixed_excitation;

    /* filters for Mixed Excitation */
    int ME_num;
    int ME_order;
    const double * const *me_h;  

    int spamf0;
    float gain;

    int freeable;  /* doesn't get dumped, but 1 when this a freeable struct */

} cst_cg_db;

/* Access model parameters, unpacking them as required */
#define CG_MODEL_VECTOR(M,N,X,Y)                                        \
    (M->model_min[Y]+((float)(M->N[X][Y])/65535.0*M->model_range[Y]))

CST_VAL_USER_TYPE_DCLS(cg_db,cst_cg_db)
void delete_cg_db(cst_cg_db *db);

cst_utterance *cg_synth(cst_utterance *utt);
cst_wave *mlsa_resynthesis(const cst_track *t, 
                           const cst_track *str, 
                           cst_cg_db *cg_db,
                           cst_audio_streaming_info *asc);
cst_track *mlpg(const cst_track *param_track, cst_cg_db *cg_db);

cst_voice *cst_cg_load_voice(const char *voxdir,
                             const cst_lang lang_table[]);
int cst_cg_dump_voice(const cst_voice *v,const cst_string *filename);

#endif
