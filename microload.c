#include "wav2prg_api.h"

static enum wav2prg_bool microload_get_block_info(struct wav2prg_context* context, const struct wav2prg_functions* functions, struct wav2prg_plugin_conf* conf, struct wav2prg_block_info* info)
{
  uint16_t complement_of_length;

  if (functions->get_word_func(context, functions, conf, &info->start) == wav2prg_false)
    return wav2prg_false;
  if (functions->get_word_func(context, functions, conf, &complement_of_length) == wav2prg_false)
    return wav2prg_false;
  info->end = info->start - complement_of_length;
  return wav2prg_true;
}

static uint16_t microload_thresholds[]={0x14d};
static uint8_t microload_pilot_sequence[]={10,9,8,7,6,5,4,3,2,1};

static const struct wav2prg_plugin_conf microload =
{
  lsbf,
  wav2prg_xor_checksum,
  wav2prg_compute_and_check_checksum,
  2,
  microload_thresholds,
  NULL,
  wav2prg_pilot_tone_with_shift_register,
  160,
  sizeof(microload_pilot_sequence),
  microload_pilot_sequence,
  0,
  first_to_last,
  NULL
};

static const struct wav2prg_plugin_conf* microload_get_state(void)
{
  return &microload;
}
static const struct wav2prg_plugin_functions microload_functions = {
    NULL,
    NULL,
    NULL,
    NULL,
    microload_get_block_info,
    NULL,
    microload_get_state,
    NULL,
    NULL,
    NULL,
    NULL
};

PLUGIN_ENTRY(microload)
{
  register_loader_func(&microload_functions, "Microload");
}