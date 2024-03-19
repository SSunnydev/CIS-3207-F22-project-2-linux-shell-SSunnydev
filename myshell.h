int countTokens(char *str, char **save);

int countCommands(char *str);

void help_func(char *command);

int cd_func(char *cd_dir);

void dir_func(char *curr_path);

void builtins(char** progArgv, int* is_builtinPtr);
