#ifndef _FTP_MESSAGE_H_
#define _FTP_MESSAGE_H_
/**
 * ftp_message.h
 * define the operations to analyse ftp's messages
 */

int _find_first_of(char*, char);
char* _substring(char* buf, int i, int len);
int _split(char*, char, char***);
int _all_ch_in_string(char*, char, int**);
char* _joint(char** array, char ch, int size);
void _transfer_str_ip_port(char* buf, char*ip[], int* port);
char* _transfer_ip_port_str(char* ip, int port);
char* parseInt2String(int num);
int findStr(char* buf, char* str, int start);
void my_strcpy(char *array,const char* array2);
void my_strcat(char* array, const char* array2);

#endif