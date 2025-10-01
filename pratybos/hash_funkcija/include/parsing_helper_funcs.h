#include <string>

// CLI parsing helpers (minimal-churn renames to snake_case)
char *get_cmd_option(char **begin, char **end, const std::string &option);
bool cmd_option_exists(char **begin, char **end, const std::string &option);
