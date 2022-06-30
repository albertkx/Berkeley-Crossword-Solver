/* Generated automatically from make_voice_list */

#include "flite.h"

cst_voice *register_cmu_us_slt(const char *voxdir);

cst_val *flite_set_voice_list(const char *voxdir)
{
   flite_voice_list = cons_val(voice_val(register_cmu_us_slt(voxdir)),flite_voice_list);
   flite_voice_list = val_reverse(flite_voice_list);
   return flite_voice_list;
}

