#ifndef OFSL_FSAL_IO_H__
#define OFSL_FSAL_IO_H__

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

#include <ofsl/fsal/fsal.h>

#ifdef __cplusplus
extern "C" {
#endif

int fsal_fclose(FSAL_File* stream);
int fsal_fflush(FSAL_File* stream);

void fsal_setbuf(FSAL_File* stream, char* buffer);
int fsal_setvbuf(FSAL_File* stream, char* buffer, int mode, size_t size);

size_t fsal_fread(void* buffer, size_t size, size_t count, FSAL_File* stream);
size_t fsal_fwrite(const void* buffer, size_t size, size_t count, FSAL_File* stream);

int fsal_fgetc(FSAL_File* stream);
char* fsal_fgets(char* str, int count, FSAL_File* stream);
int fsal_fputc(int ch, FSAL_File* stream);
int fsal_fputs(const char* str, FSAL_File* stream);
int fsal_ungetc(int ch, FSAL_File* stream);

__attribute__((format(scanf, 2, 3)))
int fsal_fscanf(FSAL_File* stream, const char* format, ...);

__attribute__((format(printf, 2, 3)))
int fsal_fprintf(FSAL_File* stream, const char* format, ...);

ssize_t fsal_ftell(FSAL_File* stream);
int fsal_fgetpos(FSAL_File* stream, fpos_t* pos);
int fsal_fseek(FSAL_File* stream, ssize_t offset, int origin);
int fsal_fsetpos(FSAL_File* stream, const fpos_t* pos);
void fsal_rewind(FSAL_File* stream);

void fsal_clearerr(FSAL_File* stream);
int fsal_feof(FSAL_File* stream);
int fsal_ferror(FSAL_File* stream);

#ifdef __cplusplus
};
#endif

#endif
