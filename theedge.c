#include "wav2prg_api.h"

static uint16_t theedge_thresholds[]={469};
static uint16_t theedge_ideal_pulse_lengths[]={360, 520};

static const struct wav2prg_plugin_conf theedge =
{
  msbf,
  wav2prg_xor_checksum,/*ignored*/
  wav2prg_do_not_compute_checksum,
  2,
  theedge_thresholds,
  theedge_ideal_pulse_lengths,
  wav2prg_pilot_tone_made_of_0_bits_followed_by_1,
  0x55,
  0,/*ignored*/
  NULL,/*ignored*/
  1000,
  first_to_last,
  NULL
};

static const struct wav2prg_plugin_conf* theedge_get_new_state(void) {
  return &theedge;
}

static enum wav2prg_sync_result theedge_get_sync(struct wav2prg_context* context, const struct wav2prg_functions* functions, struct wav2prg_plugin_conf* conf)
{
  struct theedge_private_state* state = (struct theedge_private_state*)conf->private_state;
  uint32_t num_of_pilot_bits_found = 0, old_num_of_pilot_bits_found;
  uint8_t byte = 0;
  uint32_t i;
  uint8_t bit;
  enum wav2prg_sync_result res;
  
  res = functions->get_sync_sequence(context, functions, conf);
  if (res != wav2prg_sync_success)
    return res;
  if (functions->get_bit_func(context, functions, conf, &bit) == wav2prg_false)
    return wav2prg_wrong_pulse_when_syncing;
  if (bit != 0)
    return wav2prg_sync_failure;
  if (functions->get_bit_func(context, functions, conf, &bit) == wav2prg_false)
    return wav2prg_wrong_pulse_when_syncing;
  if (bit != 1)
    return wav2prg_sync_failure;
  if (functions->get_bit_func(context, functions, conf, &bit) == wav2prg_false)
    return wav2prg_wrong_pulse_when_syncing;
  if (bit != 0)
    return wav2prg_sync_failure;
  if (functions->get_bit_func(context, functions, conf, &bit) == wav2prg_false)
    return wav2prg_wrong_pulse_when_syncing;
  if (bit != 1)
    return wav2prg_sync_failure;
  if (functions->get_bit_func(context, functions, conf, &bit) == wav2prg_false)
    return wav2prg_wrong_pulse_when_syncing;
  if (bit != 0)
    return wav2prg_sync_failure;
  if (functions->get_bit_func(context, functions, conf, &bit) == wav2prg_false)
    return wav2prg_wrong_pulse_when_syncing;
  if (bit != 1)
    return wav2prg_sync_failure;
  if (functions->get_bit_func(context, functions, conf, &bit) == wav2prg_false)
    return wav2prg_wrong_pulse_when_syncing;
  if (bit != 0)
    return wav2prg_sync_failure;
  if (functions->get_byte_func(context, functions, conf, &byte) == wav2prg_false)
    return wav2prg_wrong_pulse_when_syncing;
  for(i = 0; i < 9 && byte != 0; i++){
    if (functions->get_bit_func(context, functions, conf, &bit) == wav2prg_false)
      return wav2prg_wrong_pulse_when_syncing;
    byte = (byte << 1) | bit;
  }
  return byte == 0 ? wav2prg_sync_success : wav2prg_sync_failure;
}

static enum wav2prg_bool theedge_get_block_info(struct wav2prg_context *context, const struct wav2prg_functions* functions, struct wav2prg_plugin_conf* conf, struct wav2prg_block_info *info)
{
  uint32_t i;
  uint8_t byte;

  for(i = 0; i < 256; i++)
    if(functions->get_byte_func(context, functions, conf, &byte) == wav2prg_false)
      return wav2prg_false;
  if(functions->get_word_func(context, functions, conf, &info->start) == wav2prg_false)
    return wav2prg_false;
  if(functions->get_word_func(context, functions, conf, &info->end) == wav2prg_false)
    return wav2prg_false;
  return functions->get_sync(context, functions, conf);
}

static const struct wav2prg_plugin_functions theedge_functions =
{
  NULL,
  NULL,
  theedge_get_sync,
  NULL,
  theedge_get_block_info,
  NULL,
  theedge_get_new_state,
  NULL,
  NULL,
  NULL,
  NULL
};

PLUGIN_ENTRY(theedge)
{
  register_loader_func(&theedge_functions, "The Edge");
}

