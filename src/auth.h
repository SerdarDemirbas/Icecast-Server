/* Icecast
 *
 * This program is distributed under the GNU General Public License, version 2.
 * A copy of this license is included with this source.
 *
 * Copyright 2000-2004, Jack Moffitt <jack@xiph.org, 
 *                      Michael Smith <msmith@xiph.org>,
 *                      oddsock <oddsock@xiph.org>,
 *                      Karl Heyes <karl@xiph.org>
 *                      and others (see AUTHORS for details).
 * Copyright 2014-2018, Philipp "ph3-der-loewe" Schafft <lion@lion.leolix.org>,
 */

#ifndef __AUTH_H__
#define __AUTH_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "common/thread/thread.h"
#include "common/httpp/httpp.h"

#include "icecasttypes.h"

/* implemented */
#define AUTH_TYPE_ANONYMOUS       "anonymous"
#define AUTH_TYPE_STATIC          "static"
#define AUTH_TYPE_LEGACY_PASSWORD "legacy-password"
#define AUTH_TYPE_URL             "url"
#define AUTH_TYPE_HTPASSWD        "htpasswd"

typedef enum
{
    /* XXX: ??? */
    AUTH_UNDEFINED,
    /* user authed successfull */
    AUTH_OK,
    /* user authed failed */
    AUTH_FAILED,
    /* session got terminated */
    AUTH_RELEASED,
    /* XXX: ??? */
    AUTH_FORBIDDEN,
    /* No match for given username or other identifier found */
    AUTH_NOMATCH,
    /* status codes for database changes */
    AUTH_USERADDED,
    AUTH_USEREXISTS,
    AUTH_USERDELETED
} auth_result;

typedef struct auth_client_tag
{
    client_t     *client;
    auth_result (*process)(auth_t *auth, struct auth_client_tag *auth_user);
    void        (*on_no_match)(client_t *client, void (*on_result)(client_t *client, void *userdata, auth_result result), void *userdata);
    void        (*on_result)(client_t *client, void *userdata, auth_result result);
    void         *userdata;
    struct auth_client_tag *next;
} auth_client;


struct auth_tag
{
    /* unique ID */
    unsigned long id;

    /* URL for any kind of UI used to configure this or NULL. */
    char *management_url;

    char *mount;

    /* filters */
    int method[httpp_req_unknown+1];

    /* whether authenticate_client() and release_client() will return immediate.
     * Setting this will result in no thread being started for this.
     */
    int immediate;

    /* Authenticate using the given username and password */
    auth_result (*authenticate_client)(auth_client *aclient);
    auth_result (*release_client)(auth_client *auth_user);

    /* auth state-specific free call */
    void (*free)(auth_t *self);

    auth_result (*adduser)(auth_t *auth, const char *username, const char *password);
    auth_result (*deleteuser)(auth_t *auth, const char *username);
    auth_result (*listuser)(auth_t *auth, xmlNodePtr srcnode);

    mutex_t lock;
    int running;
    size_t refcount;

    thread_type *thread;

    /* per-auth queue for clients */
    auth_client *head, **tailp;
    int pending_count;

    void *state;
    char *type;
    char *unique_tag;

    /* acl to set on succsessful auth */
    acl_t *acl;
    /* role name for later matching, may be NULL if no role name was given in config */
    char  *role;
};

typedef struct auth_stack_tag auth_stack_t;

/* prototypes for auths that do not need own header file */
int auth_get_anonymous_auth(auth_t *auth, config_options_t *options);
int auth_get_static_auth(auth_t *auth, config_options_t *options);
int auth_get_url_auth(auth_t *authenticator, config_options_t *options);
int auth_get_htpasswd_auth(auth_t *auth, config_options_t *options);

/* prototypes for auth.c */
void auth_initialise(void);
void auth_shutdown(void);

auth_t  *auth_get_authenticator(xmlNodePtr node);
void    auth_release(auth_t *authenticator);
void    auth_addref(auth_t *authenticator);

int auth_release_client(client_t *client);

void auth_stack_add_client(auth_stack_t  *stack,
                           client_t      *client,
                           void         (*on_result)(client_t      *client,
                                                     void          *userdata,
                                                     auth_result   result),
                           void          *userdata);

void          auth_stack_release(auth_stack_t *stack);
void          auth_stack_addref(auth_stack_t *stack);
int           auth_stack_next(auth_stack_t **stack); /* returns -1 on error, 0 on success, +1 if no next element is present */
int           auth_stack_push(auth_stack_t **stack, auth_t *auth);
int           auth_stack_append(auth_stack_t *stack, auth_stack_t *tail);
auth_t       *auth_stack_get(auth_stack_t *stack);
auth_t       *auth_stack_getbyid(auth_stack_t *stack, unsigned long id);
acl_t        *auth_stack_get_anonymous_acl(auth_stack_t *stack, httpp_request_type_e method);

#endif
