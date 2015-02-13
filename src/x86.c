#include "x86.h"

#include <stdlib.h>

#include "arith.h"
#include "ast.h"
#include "slice.h"
#include "table.h"
#include "typecheck.h"
#include "win/objfile.h"


int help_sizealignof(struct name_table *nt, struct ast_typeexpr *type,
                     ident_value fieldstop, uint32_t *offsetof_out,
                     uint32_t *sizeof_out, uint32_t *alignof_out);
int help_rhs_sizealignof(struct name_table *nt, struct ast_rhs *rhs,
                         ident_value fieldstop, uint32_t *offsetof_out,
                         uint32_t *sizeof_out, uint32_t *alignof_out);

int unionfields_sizealignof(struct name_table *nt,
                            struct ast_vardecl *fields,
                            size_t fields_count,
                            ident_value fieldstop,
                            uint32_t *offsetof_out,
                            uint32_t *sizeof_out,
                            uint32_t *alignof_out) {
  uint32_t max_size = 0;
  uint32_t max_alignment = 1;
  for (size_t i = 0; i < fields_count; i++) {
    uint32_t invalid_offsetof_param;
    uint32_t size;
    uint32_t alignment;
    help_sizealignof(nt, &fields[i].type,
                     IDENT_VALUE_INVALID,
                     &invalid_offsetof_param, &size, &alignment);

    if (fields[i].name.value == fieldstop) {
      CHECK(fieldstop != IDENT_VALUE_INVALID);
      *offsetof_out = 0;
      *sizeof_out = size;
      *alignof_out = 0;
      return 1;
    }

    if (max_size < size) {
      max_size = size;
    }
    if (max_alignment < alignment) {
      max_alignment = alignment;
    }
  }
  CHECK(fieldstop == IDENT_VALUE_INVALID);
  uint32_t final_size = uint32_ceil_aligned(max_size, max_alignment);
  *offsetof_out = 0;
  *sizeof_out = final_size;
  *alignof_out = max_alignment;
  return 0;
}

/* X86 WINDOWS */
/* This is a bi-use function -- if fieldstop is IDENT_VALUE_INVALID,
then *sizeof_out and *alignof_out are initialized with the size and
alignment of the type, and zero is returned.  Otherwise, *offsetof_out
and *sizeof_out are initialized with the offset and size of the type's
field named fieldstop, and 1 is returned.  If the field name is not
found, crashes. */
/* TODO: This return code is fucking stupid. */
int help_sizealignof(struct name_table *nt, struct ast_typeexpr *type,
                     ident_value fieldstop, uint32_t *offsetof_out,
                     uint32_t *sizeof_out, uint32_t *alignof_out) {
  switch (type->tag) {
  case AST_TYPEEXPR_NAME: {
    struct deftype_entry *ent;
    if (!name_table_lookup_deftype(nt, type->u.name.value,
                                   no_param_list_arity(),
                                   &ent)) {
      CRASH("Type name should be found, it was not.");
    }
    CHECK(ent->arity.value == ARITY_NO_PARAMLIST);
    if (ent->is_primitive) {
      CHECK(fieldstop == IDENT_VALUE_INVALID);
      *offsetof_out = 0;
      *sizeof_out = ent->primitive_sizeof;
      *alignof_out = ent->primitive_alignof;
      return 0;
    } else {
      struct ast_deftype *deftype = ent->deftype;
      CHECK(!deftype->generics.has_type_params);
      return help_rhs_sizealignof(nt, &deftype->rhs, fieldstop,
                                  offsetof_out, sizeof_out, alignof_out);
    }
  } break;
  case AST_TYPEEXPR_APP: {
    struct deftype_entry *ent;
    if (!name_table_lookup_deftype(nt, type->u.app.name.value,
                                   param_list_arity(type->u.app.params_count),
                                   &ent)) {
      CRASH("Type app name should be found, it was not.");
    }
    CHECK(ent->arity.value == type->u.app.params_count);
    if (ent->is_primitive) {
      CHECK(fieldstop == IDENT_VALUE_INVALID);
      *offsetof_out = 0;
      *sizeof_out = ent->primitive_sizeof;
      *alignof_out = ent->primitive_alignof;
      return 0;
    } else {
      struct ast_deftype *deftype = ent->deftype;
      CHECK(deftype->generics.has_type_params
            && deftype->generics.params_count == type->u.app.params_count);
      struct ast_rhs substituted;
      do_replace_rhs_generics(&deftype->generics,
                              type->u.app.params,
                              &deftype->rhs,
                              &substituted);
      int ret = help_rhs_sizealignof(nt, &substituted, fieldstop,
                                     offsetof_out, sizeof_out, alignof_out);
      ast_rhs_destroy(&substituted);
      return ret;
    }
  } break;
  case AST_TYPEEXPR_STRUCTE: {
    uint32_t count = 0;
    uint32_t max_alignment = 1;
    for (size_t i = 0, e = type->u.structe.fields_count; i < e; i++) {
      uint32_t invalid_offsetof_param;
      uint32_t size;
      uint32_t alignment;
      help_sizealignof(nt, &type->u.structe.fields[i].type,
                       IDENT_VALUE_INVALID,
                       &invalid_offsetof_param, &size, &alignment);
      count = uint32_ceil_aligned(count, alignment);

      if (type->u.structe.fields[i].name.value == fieldstop) {
        CHECK(fieldstop != IDENT_VALUE_INVALID);
        *offsetof_out = count;
        *sizeof_out = size;
        *alignof_out = 0;
        return 1;
      }

      if (max_alignment < alignment) {
        max_alignment = alignment;
      }
      count = uint32_add(count, size);
    }
    CHECK(fieldstop == IDENT_VALUE_INVALID);
    count = uint32_ceil_aligned(count, max_alignment);
    *offsetof_out = 0;
    *sizeof_out = count;
    *alignof_out = max_alignment;
    return 0;
  } break;
  case AST_TYPEEXPR_UNIONE: {
    return unionfields_sizealignof(nt,
                                   type->u.unione.fields,
                                   type->u.unione.fields_count,
                                   fieldstop,
                                   offsetof_out,
                                   sizeof_out,
                                   alignof_out);
  } break;
  case AST_TYPEEXPR_ARRAY: {
    CHECK(fieldstop == IDENT_VALUE_INVALID);

    uint32_t offsetof_discard;
    uint32_t elem_size;
    uint32_t elem_alignment;
    int res = help_sizealignof(nt, type->u.arraytype.param, IDENT_VALUE_INVALID,
                               &offsetof_discard, &elem_size, &elem_alignment);
    CHECK(res == 0);
    *offsetof_out = 0;
    *sizeof_out = uint32_mul(elem_size, type->u.arraytype.count);
    *alignof_out = 0;
    return 0;
  } break;
  case AST_TYPEEXPR_UNKNOWN:
  default:
    UNREACHABLE();
  }
}

int help_rhs_sizealignof(struct name_table *nt, struct ast_rhs *rhs,
                         ident_value fieldstop, uint32_t *offsetof_out,
                         uint32_t *sizeof_out, uint32_t *alignof_out) {
  switch (rhs->tag) {
  case AST_RHS_TYPE:
    return help_sizealignof(nt, &rhs->u.type, fieldstop, offsetof_out,
                            sizeof_out, alignof_out);
  case AST_RHS_ENUMSPEC: {
    CHECK(fieldstop == IDENT_VALUE_INVALID);
    uint32_t body_offset_discard;
    uint32_t body_size;
    uint32_t body_align;
    int res = unionfields_sizealignof(nt,
                                      rhs->u.enumspec.enumfields,
                                      rhs->u.enumspec.enumfields_count,
                                      fieldstop,
                                      &body_offset_discard,
                                      &body_size,
                                      &body_align);

    /* Other code expects enum nums to be dword-sized, the body offset
    likewise. */
    CHECK(body_align <= DWORD_SIZE);
    *offsetof_out = 0;
    *sizeof_out = uint32_add(body_size, DWORD_SIZE);
    *alignof_out = DWORD_SIZE;
    return res;
  } break;
  default:
    UNREACHABLE();
  }
}

void kira_field_sizeoffset(struct name_table *nt, struct ast_typeexpr *type,
                           ident_value field_name, uint32_t *sizeof_out,
                           uint32_t *offsetof_out) {
  CHECK(field_name != IDENT_VALUE_INVALID);
  uint32_t offset;
  uint32_t size;
  uint32_t invalid_alignment_param;
  int res = help_sizealignof(nt, type, field_name,
                             &offset, &size, &invalid_alignment_param);
  CHECK(res);
  *sizeof_out = size;
  *offsetof_out = offset;
}

void kira_sizealignof(struct name_table *nt, struct ast_typeexpr *type,
                      uint32_t *sizeof_out, uint32_t *alignof_out) {
  uint32_t invalid_offsetof_param;
  help_sizealignof(nt, type, IDENT_VALUE_INVALID,
                   &invalid_offsetof_param, sizeof_out, alignof_out);
}

uint32_t kira_sizeof(struct name_table *nt, struct ast_typeexpr *type) {
  uint32_t size;
  uint32_t alignment;
  kira_sizealignof(nt, type, &size, &alignment);
  return size;
}

uint32_t kira_alignof(struct name_table *nt, struct ast_typeexpr *type) {
  uint32_t size;
  uint32_t alignment;
  kira_sizealignof(nt, type, &size, &alignment);
  return alignment;
}

