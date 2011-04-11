#include "audiotap_loop.h"
#include "audiotap.h"
#include "audiotap_callback.h"

static struct audiotap *audiotap_interruptible = NULL;

void audiotap_interrupt(){
  if(audiotap_interruptible)
    audiotap_terminate(audiotap_interruptible);
}

void audiotap_loop(struct audiotap *audiotap_in
                   ,struct audiotap *audiotap_out
                   ,struct audiotap *interruptible){
   enum audiotap_status status = AUDIOTAP_OK;
   unsigned int datalen = 0;
   int currlen;

   audiotap_interruptible = interruptible;
   while(status == AUDIOTAP_OK){
     uint32_t pulse, raw_pulse;

     if ((datalen++) % 10000 == 9999){
       currlen = audio2tap_get_current_pos(audiotap_in);
       statusbar_update(datalen);
     }
     status = audio2tap_get_pulses(audiotap_in, &pulse, &raw_pulse);
     if (status != AUDIOTAP_OK)
       break;

     status = tap2audio_set_pulse(audiotap_out, pulse);
   }
   statusbar_update(datalen);
   statusbar_exit();
   if (status == AUDIOTAP_INTERRUPTED)
     warning_message("Interrupted");
   else if(status != AUDIOTAP_OK && status != AUDIOTAP_EOF)
     error_message("Something went wrong");
   audiotap_interruptible = NULL;
   tap2audio_close(audiotap_out);
   audio2tap_close(audiotap_in);
}
