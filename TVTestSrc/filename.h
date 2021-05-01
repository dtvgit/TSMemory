/*******************************************************************
                   filename treating interface
 *******************************************************************/
#ifndef FILENAME_H
#define FILENAME_H

#ifdef __cplusplus
extern "C" {
#endif

extern int   cut_suffix(char *filepath);
extern int   check_suffix(char *filepath, char *suffix);
extern char *read_suffix(char *filepath);

#ifdef __cplusplus
}
#endif

#endif
