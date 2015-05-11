#ifndef UTILS_H
#define UTILS_H

unsigned short file_exists(const char*);
unsigned short create_directory(const char*);
unsigned short createXmlFile(const char*, const char*);
char **explode(char*, const char*, unsigned short*);
unsigned short regex_verification(const char *, unsigned short);
unsigned short *char_to_bin(const char *, unsigned int *);

char *base64_encode(const unsigned char *, size_t, size_t *);
char *base64_decode(const unsigned char *, size_t, size_t *);
char *build_decoding_table();

time_t getWeekStart(time_t*);
time_t getWeekEnd(time_t*);

#endif // UTILS_H
