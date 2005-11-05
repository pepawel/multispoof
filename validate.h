#ifndef _VALIDATE_H_
#define _VALIDATE_H_

char *get_std_ip_str (char *ip_str);
char *get_std_mac_str (char *mac_str);
char *mac_ntoa (u_char * mac);
u_char *libnet_hex_aton(char * s, int * len);

#endif
