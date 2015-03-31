/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
// vim: ft=cpp:expandtab:ts=8:sw=4:softtabstop=4:  
#ident "Copyright (c) 2014 Tokutek Inc.  All rights reserved."

#define MYSQL_SERVER
#include <mysql_version.h>
#include <mysql/plugin.h>
#include <my_global.h>
#include <my_dbug.h>
#include <log.h>
#include <sql_show.h>

// can not access globals from other plugins
// extern struct st_mysql_sys_var *tokudb_system_variables[];
// use gdb to patch in the variables address in the tokudb_variables_fill_table function

static char *tokudb_variables_version;

static MYSQL_SYSVAR_STR(version, tokudb_variables_version, PLUGIN_VAR_NOCMDARG | PLUGIN_VAR_READONLY, "version example",
                        NULL, NULL, "0.1");

static MYSQL_THDVAR_ULONG(ulong_example, 0, "ulong example", NULL, NULL, 42, 0, 1<<24, 1);

static MYSQL_THDVAR_DOUBLE(double_example, 0, "double example", NULL /*check*/, NULL /*update*/, 1.0 /*def*/, 0 /*min*/, 1.0 /*max*/, 1);

static const char *enum_example_names[] = {
    "uncompressed",
    "zlib",
    "quicklz",
    "lzma",
    NullS
};

static TYPELIB enum_example_typelib = {
    array_elements(enum_example_names) - 1,
    "enum_example_typelib",
    enum_example_names,
    NULL
};


static MYSQL_THDVAR_ENUM(enum_example, 0, "enum example", NULL, NULL, 1, &enum_example_typelib);

static struct st_mysql_sys_var *tokudb_variables_variables[] = {
    MYSQL_SYSVAR(version),
    MYSQL_SYSVAR(ulong_example),
    MYSQL_SYSVAR(double_example),
    MYSQL_SYSVAR(enum_example),
    NULL,
};

#undef MYSQL_SYSVAR_NAME
#define MYSQL_SYSVAR_NAME(name) name
#define const

typedef DECLARE_MYSQL_SYSVAR_BASIC(sysvar_bool_t, my_bool);
typedef DECLARE_MYSQL_THDVAR_BASIC(thdvar_bool_t, my_bool);
typedef DECLARE_MYSQL_SYSVAR_BASIC(sysvar_str_t, char *);
typedef DECLARE_MYSQL_THDVAR_BASIC(thdvar_str_t, char *);

typedef DECLARE_MYSQL_SYSVAR_TYPELIB(sysvar_enum_t, unsigned long);
typedef DECLARE_MYSQL_THDVAR_TYPELIB(thdvar_enum_t, unsigned long);
typedef DECLARE_MYSQL_SYSVAR_TYPELIB(sysvar_set_t, ulonglong);
typedef DECLARE_MYSQL_THDVAR_TYPELIB(thdvar_set_t, ulonglong);

typedef DECLARE_MYSQL_SYSVAR_SIMPLE(sysvar_int_t, int);
typedef DECLARE_MYSQL_SYSVAR_SIMPLE(sysvar_long_t, long);
typedef DECLARE_MYSQL_SYSVAR_SIMPLE(sysvar_longlong_t, longlong);
typedef DECLARE_MYSQL_SYSVAR_SIMPLE(sysvar_uint_t, uint);
typedef DECLARE_MYSQL_SYSVAR_SIMPLE(sysvar_ulong_t, ulong);
typedef DECLARE_MYSQL_SYSVAR_SIMPLE(sysvar_ulonglong_t, ulonglong);
typedef DECLARE_MYSQL_SYSVAR_SIMPLE(sysvar_double_t, double);

typedef DECLARE_MYSQL_THDVAR_SIMPLE(thdvar_int_t, int);
typedef DECLARE_MYSQL_THDVAR_SIMPLE(thdvar_long_t, long);
typedef DECLARE_MYSQL_THDVAR_SIMPLE(thdvar_longlong_t, longlong);
typedef DECLARE_MYSQL_THDVAR_SIMPLE(thdvar_uint_t, uint);
typedef DECLARE_MYSQL_THDVAR_SIMPLE(thdvar_ulong_t, ulong);
typedef DECLARE_MYSQL_THDVAR_SIMPLE(thdvar_ulonglong_t, ulonglong);
typedef DECLARE_MYSQL_THDVAR_SIMPLE(thdvar_double_t, double);

#undef const

static void append_attr(String &s, const char *k, bool k_quote, const char *v, bool v_quote) {
    if (k_quote)
        s.append("\""); 
    s.append(k); 
    if (k_quote)
        s.append("\"");
    s.append(":");
    if (v_quote)
        s.append("\""); 
    s.append(v); 
    if (v_quote)
        s.append("\"");
}

static void append_attr(String &s, const char *k, const char *v) {
    append_attr(s, k, true, v, true);
}

static void append_int_attr(String &s, const char *k, ulonglong v, bool is_unsigned) {
    String t;
    t.set_int(v, is_unsigned, system_charset_info);
    append_attr(s, k, true, t.c_ptr(), false);
}

static void append_double_attr(String &s, const char *k, double v) {
    String t;
    t.set_real(v, 1, system_charset_info);
    append_attr(s, k, true, t.c_ptr(), false);
}

struct fake_st_mysql_sys_var {
    union {
        struct {
            MYSQL_PLUGIN_VAR_HEADER;
        } h;
        sysvar_bool_t sys_bool;
        thdvar_bool_t thd_bool;
        sysvar_str_t sys_str;
        thdvar_str_t thd_str;
        sysvar_int_t sys_int;
        thdvar_int_t thd_int;
        sysvar_uint_t sys_uint;
        thdvar_uint_t thd_uint;
        sysvar_long_t sys_long;
        thdvar_long_t thd_long;
        sysvar_ulong_t sys_ulong;
        thdvar_ulong_t thd_ulong;
        sysvar_longlong_t sys_longlong;
        thdvar_longlong_t thd_longlong;
        sysvar_ulonglong_t sys_ulonglong;
        thdvar_ulonglong_t thd_ulonglong;
        sysvar_double_t sys_double;
        thdvar_double_t thd_double;
        sysvar_enum_t sys_enum;
        thdvar_enum_t thd_enum;
    } u;

    const char *name();
    bool readonly();
    bool thdlocal();
    const char *scope();
    const char *comment();
    String type_attributes();
};

const char *fake_st_mysql_sys_var::name() {
    return u.h.name;
}

bool fake_st_mysql_sys_var::readonly() {
    return (u.h.flags & PLUGIN_VAR_READONLY) != 0;
}

bool fake_st_mysql_sys_var::thdlocal() {
    return (u.h.flags & PLUGIN_VAR_THDLOCAL) != 0;
}

const char *fake_st_mysql_sys_var::comment() {
    return u.h.comment;
}

String fake_st_mysql_sys_var::type_attributes() {
    String s;
    bool is_unsigned = (u.h.flags & PLUGIN_VAR_UNSIGNED) != 0;
    switch (u.h.flags & 0xf) {
    case PLUGIN_VAR_BOOL: {
        append_attr(s, "type", "bool");
        my_bool *def_val = thdlocal() ? &u.thd_bool.def_val : &u.sys_bool.def_val;
        s.append(",");
        append_attr(s, "def_val", true, *def_val ? "true" : "false", false);
        break;
    }
    case PLUGIN_VAR_STR: {
        append_attr(s, "type", "str");
        char *def_val = thdlocal() ? u.thd_str.def_val : u.sys_str.def_val;
        if (def_val) {
            s.append(",");
            append_attr(s, "def_val", def_val);
        }
        break;
    }
    case PLUGIN_VAR_INT: {
        append_attr(s, "type", is_unsigned ? "uint" : "int");
        s.append(",");
        append_int_attr(s, "def_val", thdlocal() ? u.thd_uint.def_val : u.sys_uint.def_val, is_unsigned);
        s.append(",");
        append_int_attr(s, "min_val", thdlocal() ? u.thd_uint.min_val : u.sys_uint.min_val, is_unsigned);
        s.append(",");
        append_int_attr(s, "max_val", thdlocal() ? u.thd_uint.max_val : u.sys_uint.max_val, is_unsigned);
        break;
    }
    case PLUGIN_VAR_LONG: {
        append_attr(s, "type", is_unsigned ? "ulong" : "long");
        s.append(",");
        append_int_attr(s, "def_val", thdlocal() ? u.thd_ulong.def_val : u.sys_ulong.def_val, is_unsigned);
        s.append(",");
        append_int_attr(s, "min_val", thdlocal() ? u.thd_ulong.min_val : u.sys_ulong.min_val, is_unsigned);
        s.append(",");
        append_int_attr(s, "max_val", thdlocal() ? u.thd_ulong.max_val : u.sys_ulong.max_val, is_unsigned);
        break;
    }
    case PLUGIN_VAR_LONGLONG: {
        append_attr(s, "type", is_unsigned ? "ulonglong" : "longlong");
        s.append(",");
        append_int_attr(s, "def_val", thdlocal() ? u.thd_ulonglong.def_val : u.sys_ulonglong.def_val, is_unsigned);
        s.append(",");
        append_int_attr(s, "min_val", thdlocal() ? u.thd_ulonglong.min_val : u.sys_ulonglong.min_val, is_unsigned);
        s.append(",");
        append_int_attr(s, "max_val", thdlocal() ? u.thd_ulonglong.max_val : u.sys_ulonglong.max_val, is_unsigned);
        break;
    }
    case PLUGIN_VAR_ENUM: {
        append_attr(s, "type", "enum");
        TYPELIB *typelib = thdlocal() ? u.thd_enum.typelib : u.sys_enum.typelib;
        s.append(",");
        append_attr(s, "def_val", typelib->type_names[thdlocal() ? u.thd_enum.def_val : u.sys_enum.def_val]);
        s.append(",");
        String t;
        t.append("[");
        for (unsigned int i = 0; i < typelib->count; i++) {
            t.append("\""); t.append(typelib->type_names[i]); t.append("\"");
            if (i < typelib->count-1)
                t.append(",");
        }
        t.append("]");
        append_attr(s, "val", true, t.c_ptr(), false);
        break;
    }
    case PLUGIN_VAR_SET: {
        append_attr(s, "type", "set");
        break;
    }
    case PLUGIN_VAR_DOUBLE: {
        append_attr(s, "type", "double");
        s.append(",");
        append_double_attr(s, "def_val", thdlocal() ? u.thd_double.def_val : u.sys_double.def_val);
        s.append(",");
        append_double_attr(s, "min_val", thdlocal() ? u.thd_double.min_val : u.sys_double.min_val);
        s.append(",");
        append_double_attr(s, "max_val", thdlocal() ? u.thd_double.max_val : u.sys_double.max_val);
        break;
    }
    default:
        break;
    }
    return s;
}

static ST_FIELD_INFO tokudb_variables_field_info[] = {
    { "variable", 512, MYSQL_TYPE_STRING, 0, 0, NULL, SKIP_OPEN_TABLE },
    { NULL, 0, MYSQL_TYPE_NULL, 0, 0, NULL, SKIP_OPEN_TABLE}
};

static String var_json(struct st_mysql_sys_var *var) {
    struct fake_st_mysql_sys_var *fvar = (struct fake_st_mysql_sys_var *) var;
    String s;
    s.append("{");
    append_attr(s, "name", fvar->name());
    s.append(",");
    append_attr(s, "readonly", true, fvar->readonly() ? "true" : "false", false);
    s.append(",");
    append_attr(s, "scope", fvar->thdlocal() ? "session" : "system");
    s.append(",");
    s.append(fvar->type_attributes());
    s.append(",");
    append_attr(s, "comment", fvar->comment());
    s.append("}");
    return s;
}

#if MYSQL_VERSION_ID >= 50600
static int tokudb_variables_fill_table(THD *thd, TABLE_LIST *tables, Item *cond) {
#else
static int tokudb_variables_fill_table(THD *thd, TABLE_LIST *tables, COND *cond) {
#endif
    DBUG_ENTER(__FUNCTION__);
    int error = 0;
    TABLE *table = tables->table;
    struct st_mysql_sys_var **variables = tokudb_variables_variables;
    struct st_mysql_sys_var *var;
    // NOTE: can use gdb to set variables to something else here
    for (uint i = 0; error == 0 && (var = variables[i]); i++) {
        String doc = var_json(var);
        table->field[0]->store(doc.c_ptr(), doc.length(), system_charset_info);
        error = schema_table_store_record(thd, table);
    }
    DBUG_RETURN(error);
}

static int tokudb_variables_plugin_init(void *p) {
    DBUG_ENTER(__FUNCTION__);
    ST_SCHEMA_TABLE *schema = (ST_SCHEMA_TABLE *) p;
    schema->fields_info = tokudb_variables_field_info;
    schema->fill_table = tokudb_variables_fill_table;
    DBUG_RETURN(0);
}

static int tokudb_variables_plugin_deinit(void *p) {
    DBUG_ENTER(__FUNCTION__);
    DBUG_RETURN(0);
}

static struct st_mysql_daemon tokudb_variables_plugin = {
    MYSQL_INFORMATION_SCHEMA_INTERFACE_VERSION
};

#ifndef TOKUDB_VARIABLES_PLUGIN_VERSION_MAJOR
#define TOKUDB_VARIABLES_PLUGIN_VERSION_MAJOR 0
#endif
#ifndef TOKUDB_VARIABLES_PLUGIN_VERSION_MINOR
#define TOKUDB_VARIABLES_PLUGIN_VERSION_MINOR 0
#endif

mysql_declare_plugin(tokudb_backup) {
    MYSQL_INFORMATION_SCHEMA_PLUGIN,
    &tokudb_variables_plugin,
    "tokudb_variables",
    "Tokutek",
    "TokuDB variables",
    PLUGIN_LICENSE_PROPRIETARY,
    tokudb_variables_plugin_init,      // Plugin Init
    tokudb_variables_plugin_deinit,    // Plugin Deinit
    (TOKUDB_VARIABLES_PLUGIN_VERSION_MAJOR << 8) + TOKUDB_VARIABLES_PLUGIN_VERSION_MINOR,
    NULL,                        // status variables
    tokudb_variables_variables,  // system variables
    NULL,                        // config options
    0,                           // flags
}
mysql_declare_plugin_end;
