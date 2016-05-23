#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <syslog.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <security/pam_appl.h>
#define PAM_SM_AUTH
#include <security/pam_modules.h>
#define MODULE_NAME "pam_ufpidentity"
#include "identity.h"

static int check_authentication_context(authentication_context_t * authentication_context);
static int get_display_item_count(display_item_t * display_items);
static char *get_display_item_string(display_item_t * display_items);


const char 
*get_key_value(const char *key, int argc, const char **argv) {
    char *delim, sep = '=';
    int i;

    for (i = 0; i < argc; ++i) {
        delim = strchr(argv[i], sep);

        /* No separator found */
        if (delim == NULL)
            continue;

        /* No key */
        if (delim == argv[i])
            continue;

        if (strncmp(key, argv[i], delim - argv[i]) == 0)
            return delim + 1;
    }
    return NULL;
}

#if 0
static void log_message(int priority, pam_handle_t * pamh, const char *format, ...)
{
    char *service = NULL;
    if (pamh)
        pam_get_item(pamh, PAM_SERVICE, (void *) &service);
    if (!service)
        service = "";

    char logname[80];
    snprintf(logname, sizeof(logname), "%s(" MODULE_NAME ")", service);

    va_list args;
    va_start(args, format);
    openlog(logname, LOG_CONS | LOG_PID, LOG_AUTHPRIV);
    vsyslog(priority, format, args);
    closelog();
    va_end(args);
}
#endif

PAM_EXTERN int pam_sm_setcred(pam_handle_t * pamh, int flags, int argc, const char **argv)
{
    return PAM_SUCCESS;
}

/* this function is ripped from pam_unix/support.c, it lets us do IO via PAM */
int converse(pam_handle_t * pamh, int nargs, struct pam_message **message, struct pam_response **response)
{
    int retval;
    struct pam_conv *pam_conv;

    retval = pam_get_item(pamh, PAM_CONV, (const void **) &pam_conv);
    if (retval == PAM_SUCCESS) {
        retval = pam_conv->conv(nargs, (const struct pam_message **) message, response, pam_conv->appdata_ptr);
    }
    return retval;
}

int hostname_to_ip(const char *hostname, char *ip) {
    struct hostent *he;
    struct in_addr **addr_list;
    int i;

    if ( (he = gethostbyname( hostname ) ) == NULL) {
        // get the host info
#if 0
        herror("gethostbyname");
#endif
        return 1;
    }

    addr_list = (struct in_addr **) he->h_addr_list;

    for(i = 0; addr_list[i] != NULL; i++) {
        //Return the first one;
        strcpy(ip , inet_ntoa(*addr_list[i]) );
        return 0;
    }
    return 1;
}

void try_rhost(StrMap *sm, pam_handle_t *pamh) {
    const void *from = NULL;
    pam_get_item(pamh, PAM_RHOST, &from);
    pam_syslog(pamh, LOG_DEBUG, "PAM_RHOST %s", from);
    if (from != NULL) {
        char ip[100];
        memset(ip, 0, sizeof(ip));
        int r = hostname_to_ip(from, ip);
        sm_put(sm, "client_ip", (r == 0)?ip:from);
    }
}

/* expected hook, this is where custom stuff happens */
PAM_EXTERN int pam_sm_authenticate(pam_handle_t * pamh, int flags, int argc, const char **argv)
{
    int retval;

    /* getting the username that was used in the previous authentication */
    const char *username;
    if ((retval = pam_get_user(pamh, &username, NULL)) != PAM_SUCCESS) {
        return retval;
    }
    pam_syslog(pamh, LOG_INFO, "username %s", username);

    /**
     * preAuthenticate with username, if success continue, else return error. At this point
     * we are going to be allocating memory so we can't just retun without cleaning things up.
     */
    identity_context_t *identity_context = get_identity_context((char *)get_key_value("cert", argc, argv),
                                                                (char *)get_key_value("truststore", argc, argv),
                                                                (char *)get_key_value("key", argc, argv),
                                                                (char *)get_key_value("passphrase", argc, argv));
    authentication_context_t *authentication_context = NULL;
    StrMap *sm = sm_new(10);
    try_rhost(sm, pamh);

    authentication_pretext_t *authentication_pretext = pre_authenticate(identity_context, username, sm);

    retval = PAM_AUTH_ERR;
    if (authentication_pretext != NULL) {
        pam_syslog(pamh, LOG_DEBUG, "response %s", authentication_pretext->authentication_result->message);
        if ((strcmp(authentication_pretext->authentication_result->message, "OK") == 0) && (strcmp(authentication_pretext->authentication_result->text, "SUCCESS") == 0)) {
            do {
                int count = get_display_item_count(authentication_pretext->display_items);
                char *input;
                struct pam_message msg[count], *pmsg[count];
                struct pam_response *response = NULL;

                display_item_t *display_item = authentication_pretext->display_items;
                int index = 0;
                do {
                    pmsg[index] = &msg[index];
                    msg[index].msg_style = (strncmp(display_item->name, "passphrase", 10) == 0) ? PAM_PROMPT_ECHO_OFF : PAM_PROMPT_ECHO_ON;
                    msg[index].msg = get_display_item_string(display_item);
                    index++;
                    display_item = display_item->next;
                } while (display_item != NULL);

                retval = converse(pamh, count, pmsg, &response);

                // clean up the messages
                int i;
                for (i = 0; i < count; i++)
                    free((void *)msg[i].msg);
                if (retval == PAM_SUCCESS) {
                    // if this function fails, make sure that ChallengeResponseAuthentication in sshd_config is set to yes
                    display_item = authentication_pretext->display_items;
                    index = 0;
                    StrMap *sm = sm_new(10);
                    do {
                        sm_put(sm, display_item->name, response[index].resp);
                        index++;
                        display_item = display_item->next;
			free (response[index].resp);
                    } while (display_item != NULL);
		    free (response);
                    try_rhost(sm, pamh);
                    authentication_context = authenticate(identity_context, authentication_pretext->name, sm);
                } else
                    break;
            } while (!check_authentication_context(authentication_context));
        } else
            retval = PAM_USER_UNKNOWN;
        free_authentication_pretext(authentication_pretext);
    } else
        pam_syslog(pamh, LOG_DEBUG, "authentication_pretext is NULL");

    if (authentication_context != NULL) {
        pam_syslog(pamh, LOG_DEBUG, "message %s, text %s", authentication_context->authentication_result->message, authentication_context->authentication_result->text);
        if (strcmp(authentication_context->authentication_result->text, "SUCCESS") == 0)
            retval = PAM_SUCCESS;
        else
            retval = PAM_MAXTRIES;
        free_authentication_context(authentication_context);
    }
    if (identity_context != NULL)
        free_identity_context(identity_context);
    return retval;
}

static char *get_display_item_string(display_item_t * display_item)
{
    int length = strlen(display_item->display_name) + strlen(display_item->nickname) + 5;       // a space, two parens, colon and terminating null
    char *buffer = malloc(length);
    if (buffer) {
        memset(buffer, 0, length);
        sprintf(buffer, "%s (%s):", display_item->display_name, display_item->nickname);
    }
    return buffer;
}

static int check_authentication_context(authentication_context_t * authentication_context)
{
    pam_syslog(pamh, LOG_DEBUG, "message %s, text %s", authentication_context->authentication_result->message, authentication_context->authentication_result->text);
    return (((strcmp(authentication_context->authentication_result->message, "OK") == 0) && (strcmp(authentication_context->authentication_result->text, "SUCCESS") == 0))
            || (strcmp(authentication_context->authentication_result->text, "RESET") == 0));
}

static int get_display_item_count(display_item_t * display_items)
{
    int count = 0;
    display_item_t *display_item = display_items;
    do {
        display_item = display_item->next;
        count++;
    } while (display_item != NULL);
    return count;
}
