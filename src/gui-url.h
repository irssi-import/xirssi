#ifndef __GUI_URL_H
#define __GUI_URL_H

#define GALEON_OPEN "exec - -nosh -quiet galeon $0"
#define MOZILLA_OPEN "exec - -nosh mozilla $0"
#define LFTP_OPEN "exec - -nosh xterm -e lftp $0"
#define MUTT_SEND "exec - -nosh xterm -e mutt $0"

void gui_urls_init(void);
void gui_urls_deinit(void);

#endif
