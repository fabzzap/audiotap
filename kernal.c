#include "wav2prg_api.h"

struct kernal_state {
  uint8_t headerchunk_necessary_bytes[21];
};

static uint16_t kernal_thresholds[]={448, 576};
static uint16_t kernal_ideal_pulse_lengths[]={384, 512, 688};
static uint8_t kernal_1stcopy_pilot_sequence[]={137,136,135,134,133,132,131,130,129};
static uint8_t kernal_2ndcopy_pilot_sequence[]={9,8,7,6,5,4,3,2,1};
static struct wav2prg_size_of_private_state size_of_private_state = {sizeof(struct kernal_state)};

const struct wav2prg_plugin_conf kernal_headerchunk_first_copy =
{
  lsbf,
  wav2prg_xor_checksum,
  3,
  kernal_thresholds,
  kernal_ideal_pulse_lengths,
  wav2prg_synconbyte,
  0,/*ignored, overriding get_first_sync*/
  9,
  kernal_1stcopy_pilot_sequence,
  &size_of_private_state
};

const struct wav2prg_plugin_conf kernal_headerchunk_second_copy =
{
  lsbf,
  wav2prg_xor_checksum,
  3,
  kernal_thresholds,
  kernal_ideal_pulse_lengths,
  wav2prg_synconbyte,
  0,/*ignored, overriding get_first_sync*/
  9,
  kernal_2ndcopy_pilot_sequence,
  &size_of_private_state
};

static enum wav2prg_return_values kernal_get_bit_func(struct wav2prg_context* context, const struct wav2prg_functions* functions, struct wav2prg_plugin_conf* conf, uint8_t* bit)
{
  uint8_t pulse1, pulse2;
  if (functions->get_pulse_func(context, functions, conf, &pulse1) == wav2prg_invalid)
    return wav2prg_invalid;
  if (functions->get_pulse_func(context, functions, conf, &pulse2) == wav2prg_invalid)
    return wav2prg_invalid;
  if (pulse1 == 0 && pulse2 == 1) {
    *bit = 0;
    return wav2prg_ok;
  }
  if (pulse1 == 1 && pulse2 == 0) {
    *bit = 1;
    return wav2prg_ok;
  }
  return wav2prg_invalid;
}

static enum
{
  byte_found,
  eof_marker_found,
  could_not_sync
}sync_with_byte_and_get_it(struct wav2prg_context* context, const struct wav2prg_functions* functions, struct wav2prg_plugin_conf* conf, uint8_t* byte, uint8_t allow_short_pulses_at_first)
{
  uint8_t pulse;
  uint8_t parity;
  uint8_t test;

  while(1){
    if (functions->get_pulse_func(context, functions, conf, &pulse) == wav2prg_invalid)
      return could_not_sync;
    if(pulse==2)
      break;
    if(pulse!=0 || !allow_short_pulses_at_first)
      return could_not_sync;
  }
  if (functions->get_pulse_func(context, functions, conf, &pulse) == wav2prg_invalid)
    return could_not_sync;
  switch(pulse){
  case 1:
    break;
  case 0:
    return eof_marker_found;
  default:
    return could_not_sync;
  }
  if(functions->get_byte_func(context, functions, conf, byte) == wav2prg_invalid)
    return could_not_sync;
  if(kernal_get_bit_func(context, functions, conf, &parity) == wav2prg_invalid)
    return could_not_sync;
  for(test=1; test; test<<=1)
    parity^=(*byte & test)?1:0;
  return parity?byte_found:could_not_sync;
}

enum wav2prg_sync_return_values kernal_get_first_sync(struct wav2prg_context* context, const struct wav2prg_functions* functions, struct wav2prg_plugin_conf* conf, uint8_t* byte)
{
  return sync_with_byte_and_get_it(context, functions, conf, byte, 1)==byte_found?wav2prg_synced_and_one_byte_got:wav2prg_notsynced;
}

enum wav2prg_return_values kernal_get_byte(struct wav2prg_context* context, const struct wav2prg_functions* functions, struct wav2prg_plugin_conf* conf, uint8_t* byte)
{
  return sync_with_byte_and_get_it(context, functions, conf, byte, 0)==byte_found?wav2prg_ok:wav2prg_invalid;
}

enum wav2prg_return_values kernal_headerchunk_get_block_info(struct wav2prg_context* context, const struct wav2prg_functions* functions, struct wav2prg_plugin_conf* conf, char* name, uint16_t* start, uint16_t* end)
{
  uint16_t skipped_at_beginning;
  uint8_t i;
  struct kernal_state *headerchunk_necessary_bytes = (struct kernal_state *)conf->private_state;
  uint16_t num_of_necessary_bytes = sizeof(headerchunk_necessary_bytes->headerchunk_necessary_bytes);

  functions->enable_checksum_func(context);

  if (functions->get_block_func(context, functions, conf, headerchunk_necessary_bytes->headerchunk_necessary_bytes, &num_of_necessary_bytes, &skipped_at_beginning) == wav2prg_invalid)
  {
    return wav2prg_invalid;
  }
  if (headerchunk_necessary_bytes->headerchunk_necessary_bytes[0] != 1
   && headerchunk_necessary_bytes->headerchunk_necessary_bytes[0] != 3){
    return wav2prg_invalid;
  }
  for(i = 0; i < 16; i++)
    name[i] = headerchunk_necessary_bytes->headerchunk_necessary_bytes[i + 5];

  *start=828;
  *end=1020;

  return wav2prg_ok;
}

/* This differs from the default implementation, functions->get_block_func(), because it is
   aware that sync_with_byte_and_get_it() can return eof_marker_found. In that case,
   the block is shorter than expected but still valid */
static enum wav2prg_return_values kernal_get_block(struct wav2prg_context* context, const struct wav2prg_functions* functions, struct wav2prg_plugin_conf* conf, uint8_t* block, uint16_t* block_size, uint16_t* skipped_at_beginning)
{
  uint16_t bytes_received = 0;
  enum wav2prg_return_values ret = wav2prg_ok;

  *skipped_at_beginning = 0;
  for(bytes_received = 0; bytes_received != *block_size; bytes_received++) {
    switch(sync_with_byte_and_get_it(context, functions, conf, block + bytes_received, 0)){
    case byte_found:
      break;
    case could_not_sync:
      ret = wav2prg_invalid;
      /*fallback*/
    case eof_marker_found:
      *block_size = bytes_received;
    }
  }
  return ret;
}

static enum wav2prg_return_values kernal_datachunk_get_block(struct wav2prg_context* context, const struct wav2prg_functions* functions, struct wav2prg_plugin_conf* conf, uint8_t* block, uint16_t* block_size, uint16_t* skipped_at_beginning)
{
  enum wav2prg_return_values ret;
  (*block_size)++;
  ret = kernal_get_block(context, functions, conf, block, block_size, skipped_at_beginning);
  if(ret == wav2prg_ok)
    (*block_size)--;
  return ret;
}

static enum wav2prg_return_values kernal_headerchunk_get_block(struct wav2prg_context* context, const struct wav2prg_functions* functions, struct wav2prg_plugin_conf* conf, uint8_t* block, uint16_t* block_size, uint16_t* skipped_at_beginning)
{
  enum wav2prg_return_values ret;
  struct kernal_state *headerchunk_necessary_bytes = (struct kernal_state *)conf->private_state;
  uint16_t block_size_to_get_now = *block_size - sizeof(headerchunk_necessary_bytes->headerchunk_necessary_bytes);
  uint8_t* already_got = headerchunk_necessary_bytes->headerchunk_necessary_bytes;

  for (; *block_size > block_size_to_get_now; (*block_size)--)
  {
    *block++ = *already_got++;
  }
  ret = kernal_datachunk_get_block(context, functions, conf, block, block_size, skipped_at_beginning);
  if(ret == wav2prg_ok)
    (*block_size) += sizeof(headerchunk_necessary_bytes->headerchunk_necessary_bytes);
  return ret;
}

enum wav2prg_return_values kernal_get_loaded_checksum(struct wav2prg_context* context, const struct wav2prg_functions* functions, struct wav2prg_plugin_conf* conf, uint8_t* byte)
{
  *byte = 0;
  return wav2prg_ok;
}

static const struct wav2prg_plugin_conf* kernal_firstcopy_get_new_state(void) {
  return &kernal_headerchunk_first_copy;
}

static const struct wav2prg_plugin_conf* kernal_secondcopy_get_new_state(void) {
  return &kernal_headerchunk_second_copy;
}

static const struct wav2prg_plugin_functions kernal_headerchunk_firstcopy_functions =
{
  kernal_get_bit_func,
  kernal_get_byte,
  kernal_get_first_sync,
  kernal_headerchunk_get_block_info,
  kernal_headerchunk_get_block,
  kernal_firstcopy_get_new_state,
  NULL,
  kernal_get_loaded_checksum
};

static const struct wav2prg_plugin_functions kernal_headerchunk_secondcopy_functions =
{
  kernal_get_bit_func,
  kernal_get_byte,
  kernal_get_first_sync,
  kernal_headerchunk_get_block_info,
  kernal_headerchunk_get_block,
  kernal_secondcopy_get_new_state,
  NULL,
  kernal_get_loaded_checksum
};

PLUGIN_ENTRY(kernal)
{
  register_loader_func(&kernal_headerchunk_firstcopy_functions, "Kernal header chunk 1st copy");
  register_loader_func(&kernal_headerchunk_secondcopy_functions, "Kernal header chunk 2nd copy");
}
