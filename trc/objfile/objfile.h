#ifndef KIT_OBJFILE_OBJFILE_H_
#define KIT_OBJFILE_OBJFILE_H_

#include <stddef.h>
#include <stdint.h>

#include "identmap.h"

struct databuf;
struct objfile;
struct objfile_section;

enum target_platform {
  TARGET_PLATFORM_WIN_32BIT,
  TARGET_PLATFORM_LINUX_32BIT,
};

enum is_function {
  IS_FUNCTION_NO,
  IS_FUNCTION_YES,
};

enum is_static {
  IS_STATIC_NO,
  IS_STATIC_YES,
};

/* 1-based index into the section header table, for that matter. */
enum section {
  SECTION_DATA = 1,
  SECTION_RDATA = 2,
  SECTION_TEXT = 3,
};

struct sti {
  uint32_t value;
};

void objfile_alloc(struct objfile **p_out);
void objfile_free(struct objfile **p_ref);

struct objfile_section *objfile_data(struct objfile *f);
struct objfile_section *objfile_rdata(struct objfile *f);
struct objfile_section *objfile_text(struct objfile *f);

uint32_t objfile_section_size(struct objfile_section *s);

void objfile_section_append_dir32(struct objfile_section *s,
                                  struct sti SymbolTableIndex);
void objfile_section_append_rel32(struct objfile_section *s,
                                  struct sti SymbolTableIndex);
void objfile_section_append_raw(struct objfile_section *s,
                                const void *buf, size_t n);
void objfile_section_overwrite_raw(struct objfile_section *s,
                                   size_t offset,
                                   const void *buf, size_t n);
void objfile_section_align_dword(struct objfile_section *s);
void objfile_fillercode_align_double_quadword(struct objfile *f);
void objfile_section_append_zeros(struct objfile_section *s, size_t count);

void objfile_set_symbol_value(struct objfile *f,
                              struct sti SymbolTableIndex,
                              uint32_t value);

struct sti objfile_add_local_symbol(struct objfile *f,
                                    ident_value name,
                                    uint32_t value,
                                    enum section section,
                                    enum is_static is_static);

struct sti objfile_add_remote_symbol(struct objfile *f,
                                     ident_value name,
                                     enum is_function is_function);

int objfile_c_symbol_name(enum target_platform platform,
                          const void *name, size_t name_count,
                          void **c_name_out, size_t *c_name_count_out);

/* Utility. */
void append_zeros_to_align(struct databuf *d, size_t alignment);
void append_zeros(struct databuf *d, size_t count);

/* TODO: This should return a value in a wrapper struct? */
uint32_t strtab_add(struct databuf *d, const void *buf, size_t count);

#endif /* KIT_OBJFILE_OBJFILE_H_ */
