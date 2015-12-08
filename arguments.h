#ifndef _ARGUMENTS_H
#define _ARGUMENTS_H
struct arguments {
    int argument_index;
    int value_index;
    int key_length;
};

typedef struct arguments arguments_t;

arguments_t *get_arguments(int count, const char **arguments);
const char *get_key_value(char *, arguments_t *arguments, int argc, const char **args);
#endf /* _ARGUMENTS_H */
