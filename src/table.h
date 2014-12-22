#ifndef KIRA_TABLE_H_
#define KIRA_TABLE_H_

#include "ast.h"

struct def_entry {
  ident_value name;
  struct ast_generics generics;
  struct ast_typeexpr type;

  int is_primitive;
  struct ast_def *def;
};

#define ARITY_NO_PARAMLIST SIZE_MAX

struct generics_arity {
  /* ARITY_NO_PARAMLIST means no param list, 0 means an empty param list. */
  size_t value;
};

struct generics_arity params_arity(struct ast_generics *a);

struct generics_arity no_param_list_arity(void);
struct generics_arity param_list_arity(size_t arity);

struct deftype_entry {
  ident_value name;
  struct generics_arity arity;

  /* Non-null, if !arity_no_paramlist(arity) */
  int *flatly_held;
  /* Equal to arity.value, if it has a value. */
  size_t flatly_held_count;

  int has_been_checked;
  int is_being_checked;

  int is_primitive;
  struct ast_deftype *deftype;
};

int deftype_entry_param_is_flatly_held(struct deftype_entry *entry,
                                       size_t which_generic);

struct name_table {
  struct def_entry **defs;
  size_t defs_count;
  size_t defs_limit;

  struct deftype_entry **deftypes;
  size_t deftypes_count;
  size_t deftypes_limit;
};

void name_table_init(struct name_table *t);
void name_table_destroy(struct name_table *t);

int name_table_add_def(struct name_table *t,
                       ident_value name,
                       struct ast_generics *generics,
                       struct ast_typeexpr *type,
                       struct ast_def *def);
int name_table_add_primitive_def(struct name_table *t,
                                 ident_value name,
                                 struct ast_generics *generics,
                                 struct ast_typeexpr *type);
int name_table_add_deftype(struct name_table *t,
                           ident_value name,
                           struct generics_arity arity,
                           struct ast_deftype *deftype);
int name_table_add_primitive_type(struct name_table *t,
                                  ident_value name,
                                  int *flatly_held,
                                  size_t flatly_held_count);

/* TODO: Remove this, it's obviously wrong. */
int name_table_lookup_def(struct name_table *t,
                          ident_value name,
                          struct generics_arity arity,
                          struct def_entry **out);

int name_table_match_def(struct name_table *t,
                         ident_value name,
                         struct ast_typeexpr *generics_or_null,
                         size_t generics_count,
                         struct ast_typeexpr *partial_type,
                         struct ast_typeexpr *unified_type_out,
                         struct def_entry **entry_out);

int name_table_lookup_deftype(struct name_table *t,
                              ident_value name,
                              struct generics_arity arity,
                              struct deftype_entry **out);

int name_table_shadowed(struct name_table *t, ident_value name);

struct deftype_entry *lookup_deftype(struct name_table *t,
                                     struct ast_deftype *a);

void deftype_entry_mark_is_being_checked(struct deftype_entry *ent);
void deftype_entry_mark_has_been_checked(struct deftype_entry *ent);
void deftype_entry_mark_generic_flatly_held(struct deftype_entry *ent,
                                            size_t which_generic);


#endif /* KIRA_TABLE_H_ */
