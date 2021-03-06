#include "config.h"
#include "logging.h"

#include <ctype.h>
#include <stdio.h>

#define ISSPACE(c) (isspace((int)(c)))
#define ISIDENT(c) (isalnum((int)(c)) || (c) == '_' || (c) == '-')
#define ISVALUE(c) (isprint((int)(c)))

// TODO: needs unit tests
static int config_parse(char *line, const char **sectionp, const char **namep, const char **valuep)
{
  enum state { INIT, START_SECTION, SECTION, NAME, PRE_EQ, POST_EQ, VALUE, END } state = INIT;

  for (char *c = line; *c; c++) {
    switch(state) {
    case INIT:
      if (*c == '\n') {
        return 0;
      } else if (ISSPACE(*c)) {
        continue;
      } else if (*c == '[') {
        state = START_SECTION;
      } else if (ISIDENT(*c)) {
        state = NAME;
        *namep = c;
      } else {
        LOG_WARN("Invalid character at start of line: %c", *c);
        return -1;
      }
      break;

    case START_SECTION:
      if (ISIDENT(*c)) {
        state = SECTION;
        *sectionp = c;
      } else {
        LOG_WARN("Invalid character at start of section: %c", *c);
        return -1;
      }
      break;

    case SECTION:
      if (*c == ']') {
        state = END;
        *c = '\0';
      } else if (ISIDENT(*c)) {
        continue;
      } else {
        LOG_WARN("Invalid character in section: %c", *c);
        return -1;
      }
      break;

    case NAME:
      if (*c == '\n') {
        LOG_WARN("EOL in name");
        return -1;
      } else if (ISSPACE(*c)) {
        *c = '\0';
        state = PRE_EQ;
      } else if (*c == '=') {
        *c = '\0';
        state = POST_EQ;
      } else if (ISIDENT(*c)) {
        continue;
      } else {
        LOG_WARN("Invalid character in name: %c", *c);
        return -1;
      }
      break;

    case PRE_EQ:
      if (*c == '\n') {
        LOG_WARN("EOL before =");
        return -1;
      } else if (ISSPACE(*c)) {
        continue;
      } else if (*c == '=') {
        state = POST_EQ;
      } else {
        LOG_WARN("Invalid character before EQ: %c", *c);
        return -1;
      }
      break;

    case POST_EQ:
      if (*c == '\n') {
        *c = '\0';
        state = END;
        *valuep = c; // empty string
        break;
      } else if (ISSPACE(*c)) {
        continue;
      } else if (ISVALUE(*c)) {
        state = VALUE;
        *valuep = c;
      } else {
        LOG_WARN("Invalid character after EQ: %c", *c);
        return -1;
      }
      break;

    case VALUE:
      if (*c == '\n') {
        *c = '\0';
        state = END;
      } else if (ISVALUE(*c)) {
        continue;
      } else {
        LOG_WARN("Invalid character in value: %c", *c);
        return -1;
      }
      break;

    case END:
      if (ISSPACE(*c)) {
        continue;
      } else {
        LOG_WARN("Invalid character at end: %c", *c);
        return -1;
      }
      break;
    }
  }

  return 0;
}

int config_read(struct config *config, FILE *file)
{
  char buf[CONFIG_LINE];
  int lineno = 0;

  const struct configmod *mod = NULL;
  const struct configtab *tab = NULL;

  while (fgets(buf, sizeof(buf), file) != NULL) {
    const char *section = NULL;
    const char *name = NULL;
    const char *value = NULL;

    lineno++;

    LOG_DEBUG("%s", buf);

    if (config_parse(buf, &section, &name, &value)) {
      LOG_WARN("Invalid line at %s:%d", config->filename, lineno);
      continue;
    }

    if (section) {
      mod = NULL;

      if (configmod_lookup(config->modules, section, &mod)) {
        LOG_WARN("Unknown section: %s", section);
      } else {
        LOG_DEBUG("mod=%s", mod->name);
      }
    }

    if (name && value) {
      if (!mod) {
        LOG_WARN("Invalid name without section: %s", name);
      } else if (configtab_lookup(mod->table, name, &tab)) {
        LOG_WARN("Unknown name in section %s: %s", mod->name, name);
      } else if (config_set(mod, tab, value)) {
        LOG_WARN("Invalid value for section %s name %s: %s", mod->name, tab->name, value);
      } else {
        LOG_DEBUG("mod=%s tab=%s value=%s", mod->name, tab->name, value);
      }
    }
  }

  // TODO
  return 0;
}
