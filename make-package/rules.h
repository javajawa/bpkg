#ifndef _BPKG_RULES_H
#define _BPKG_RULES_H

#include <stddef.h>

#include "common/tar.h"

struct owner_rule
{
	size_t path_len;
	char   user [TAR_USERLEN];
	char   group[TAR_USERLEN];
	char   mask[8];
	char*  path;
};

struct rule_list
{
	struct owner_rule * rules;
	size_t count;
};
extern struct rule_list rules;

static char const * const default_user  = "root";
static char const * const default_group = "root";
static char const * const default_mask  = "0755";

#define rule(i) rules.rules[i]

void load_rules();
void match_rule( char const * const file, size_t const file_len, char const ** user, char const ** group, char const ** mask );
void free_rules();

#endif
