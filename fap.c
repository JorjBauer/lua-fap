#include <lua.h>
#include <lauxlib.h>

#include <fap.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "luaabstract.h"

#define MODULENAME      "fap"

#ifndef VERSION
#define VERSION "undefined"
#endif

/* These type strings aren't exported from libfap. Shame. */

static const char *type_strings[] = { "location", "object", "item",
				      "MIC-E", "NMEA", "WX",
				      "message", "capabilities",
				      "status", "telemetry",
				      "telemetry message",
				      "DX SPOT", "experimental" };



/* fap.init()
 *
 * Initializes the FAP shared library.
 */
static int lfap_init(lua_State *l)
{
  int numargs = lua_gettop(l);
  if (numargs != 0) {
    lua_pushstring(l, "usage: fap.init()");
    lua_error(l);
    return 0;
  }

  fap_init();

  return 1;
}

/* fap.cleanup()
 *
 * Frees all FAP library shared memory. Must re-init() after calling.
 */
static int lfap_cleanup(lua_State *l)
{
  int numargs = lua_gettop(l);
  if (numargs != 0) {
    lua_pushstring(l, "usage: fap.cleanup()");
    lua_error(l);
    return 0;
  }

  fap_cleanup();

  return 1;
}

/* Utility macros for parseaprs data-stuffing */

#define PUSHCNUM(x) if (p->x) {lua_pushnumber(l, *p->x); lua_setfield(l, -2, #x);}
#define PUSHNUM(x) {lua_pushnumber(l, p->x); lua_setfield(l, -2, #x);}
#define PUSHSTR(x) {lua_pushstring(l, p->x); lua_setfield(l, -2, #x);}

/* int result = fap.parseaprs(string data, int is_ax25=0)
 *
 * Parses the given data string as either APRS-IS data (is_ax25==0 or is 
 * omitted) or AX.25 network data (is_ax25==1).
 */

static int lfap_parseaprs(lua_State *l)
{
  const char *data = NULL;
  int is_ax25 = 0;
  fap_packet_t *p = NULL;

  int numargs = lua_gettop(l);
  if (numargs < 1 || numargs > 2 ||
      lua_type(l, 1) != LUA_TSTRING ||
      (numargs == 2 && lua_type(l,2) != LUA_TNUMBER &&
       lua_type(l,2) != LUA_TBOOLEAN)) {
    lua_pushstring(l, "usage: fap.parseaprs(data, is_ax25=0)");
    lua_error(l);
    return 0;
  }

  data = tostring(l, 1);
  
  if (numargs == 2) {
    if (lua_type(l,2) == LUA_TBOOLEAN) {
      is_ax25 = lua_toboolean(l, 2);
    } else {
      is_ax25 = tointeger(l, 2);
    }
  }

  p = fap_parseaprs(data, strlen(data), is_ax25);

  if (!p) {
    lua_pushstring(l, "internal error: fap_parseaprs returned NULL");
    lua_error(l);
    return 0;
  }

  /* Copy the contents of 'p' in to a new table, to be returned */
  lua_newtable(l);

  /* First, the return code from the API call */
  lua_pushnumber(l, p->error_code ? *p->error_code : 0);
  lua_setfield(l, -2, "error_code");
  if (p->error_code) {
    char msg[256];
    fap_explain_error(*p->error_code, msg);
    lua_pushstring(l, msg);
    lua_setfield(l, -2, "error_message");
  }

  /* Content that always exists */
  PUSHSTR(header);
  PUSHSTR(body);
  PUSHSTR(src_callsign);
  PUSHSTR(dst_callsign);
  PUSHNUM(symbol_table);
  PUSHNUM(symbol_code);
  PUSHNUM(dao_datum_byte);
  PUSHSTR(destination);
  PUSHSTR(message);
  PUSHSTR(message_ack);
  PUSHSTR(message_nack);
  PUSHSTR(message_id);
  PUSHSTR(object_or_item_name);

  lua_pushlstring(l, p->comment, p->comment_len);
  lua_setfield(l, -2, "comment");

  /* Content that sometimes exists */
  if (p->type) {
    lua_pushstring(l, type_strings[*p->type]);
    lua_setfield(l, -2, "type");
  }

  PUSHCNUM(latitude);
  PUSHCNUM(longitude);
  PUSHCNUM(pos_resolution);
  PUSHCNUM(pos_ambiguity);
  PUSHCNUM(altitude);
  PUSHCNUM(course);
  PUSHCNUM(speed);
  PUSHCNUM(messaging);
  PUSHCNUM(gps_fix_status);
  PUSHCNUM(radio_range);
  PUSHCNUM(phg);
  PUSHCNUM(timestamp);
  PUSHCNUM(nmea_checksum_ok);
  PUSHCNUM(alive);

  if (p->status_len) {
    lua_pushlstring(l, p->status, p->status_len);
    lua_setfield(l, -2, "status");
  }

  if (p->messagebits) {
    char buffer[20];
    lua_pushlstring(l, p->messagebits, 3);
    lua_setfield(l, -2, "messagebits");
    fap_mice_mbits_to_message(p->messagebits, buffer);
    lua_pushstring(l, buffer);
    lua_setfield(l, -2, "message");
  }

  /* Arrays that sometimes exist */
  if (p->capabilities_len) {
    lua_createtable(l, p->capabilities_len, 0);
    char pos[3];
    int i;
    for (i=0; i<10 && i<p->capabilities_len; i++) {
      lua_pushstring(l, p->capabilities[i]);
      sprintf(pos, "%d", i+1); // arrays are 1-based in Lua
      lua_setfield(l, -2, pos);
    }
    lua_setfield(l, -2, "capabilities");
  }
  
  if (p->path_len) {
    lua_createtable(l, p->path_len, 0);
    char pos[3];
    int i;
    for (i=0; i<10 && i<p->path_len; i++) {
      lua_pushstring(l, p->path[i]);
      sprintf(pos, "%d", i+1); // arrays are 1-based in Lua
      lua_setfield(l, -2, pos);
    }
    lua_setfield(l, -2, "path");
  }

  /* Free the original packet response */
  fap_free(p);

  /* Return 1 item on the stack (the table with the packet information). */

  return 1; /* # of arguments returned on stack */
}


/* metatable, hook for calling gc_context on context structs */
static const luaL_reg meta[] = {
  //  { "__gc", gc_context },
  { NULL,   NULL        }
};

/* function table for this module */
static const struct luaL_reg methods[] = {
  { "init",         lfap_init      },
  { "cleanup",      lfap_cleanup   },
  { "parseaprs",    lfap_parseaprs },
  { NULL,           NULL           }
};

/* Module initializer, called from Lua when the module is loaded. */
int luaopen_fap(lua_State *l)
{
  /* Construct a new namespace table for Lua, and register it as the global 
   * named "fap".
   */
  luaL_openlib(l, MODULENAME, methods, 0);

  /* Create metatable, which is used to tie C data structures to our garbage 
   * collection function. */
  luaL_newmetatable(l, MODULENAME);
  luaL_openlib(l, 0, meta, 0);
  lua_pushliteral(l, "__index");
  lua_pushvalue(l, -3);               /* dup methods table*/
  lua_rawset(l, -3);                  /* metatable.__index = methods */
  lua_pushliteral(l, "__metatable");
  lua_pushvalue(l, -3);               /* dup methods table*/
  lua_rawset(l, -3);                  /* hide metatable:
                                         metatable.__metatable = methods */
  lua_pop(l, 1);                      /* drop metatable */
  return 1;                           /* return methods on the stack */

}

