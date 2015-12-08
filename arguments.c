#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arguments.h"

/*
 * parses an argument like key=value and its expected to return an
 * array of argument structs with value_index pointing to actual NULL
 * terminated value, and key_length the non-NULL terminated length of
 * the key, at 0
 */
arguments_t *get_arguments(int count, const char **arguments) {
    arguments_t *args = NULL;
    if (count > 0) {
        args = (arguments_t *)malloc(sizeof(arguments_t)*count);
        int i = 0, index = 0;
        for (i = 0; i < count; i++)
            args[i].argument_index = -1;

        for (i = 0; i < count; i++) {
            char *pch = strchr(arguments[i], '=');
            if (pch != NULL) {
                arguments_t *argptr = &args[index];
                argptr->argument_index = i;
                argptr->value_index = (pch-arguments[i])+1;
                argptr->key_length = pch-arguments[i];
            }
            index++;
        }
    }
    return args;
}

const char *get_key_value(char *key, arguments_t *arguments, int argc, const char **args) {
    const char *value = NULL;
    int index = 0;
    arguments_t *argptr = &arguments[index];
    while (index < argc) {
        if (argptr->argument_index > -1) {
            if (strncmp(key, args[argptr->argument_index], (argptr->key_length > strlen(key)) ? strlen(key) : argptr->key_length ) == 0) {
                value = args[argptr->argument_index]+argptr->value_index;
                break;
            }
        }
        index++;
        argptr = &arguments[index];
    }
    return value;
}

#ifndef __PIC__
int main(int argc, const char **argv)
{
    arguments_t *arguments = get_arguments(argc, argv);
    int i = 0;
    for (i = 0; i < argc; i++) {
        if (arguments[i].argument_index > -1) {
            printf("found argument %d, with key %.*s, and value %s\n", i, arguments[i].key_length, argv[arguments[i].argument_index],
               argv[arguments[i].argument_index]+arguments[i].value_index);
        }
    }

    printf("looking for key value: %s\n", get_key_value("key", arguments, argc, argv));
    printf("looking for cert value: %s\n", get_key_value("cert", arguments, argc, argv));
    printf("looking for passphrase value: %s\n", get_key_value("passphrase", arguments, argc, argv));
    printf("looking for truststore value: %s\n", get_key_value("truststore", arguments, argc, argv));
    printf("looking for foo value: %s\n", get_key_value("foo", arguments, argc, argv));
    free(arguments);
}
#endif
