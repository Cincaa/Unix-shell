const char* PIPE_CHAR = "|";
const char* REDIRECT_LEFT_CHAR = "<";
const char* REDIRECT_RIGHT_CHAR = ">";
const char* AND_OP_CHAR = "&&";
const char* OR_OP_CHAR = "||";

#define IS_PIPE_CHAR 1
#define IS_REDIRECT_LEFT_CHAR 2
#define IS_REDIRECT_RIGHT_CHAR 3
#define IS_AND_OP_CHAR 4
#define IS_OR_OP_CHAR 5

#define NORMAL_GROUP 1
#define PIPED_GROUP 2
#define REDIRECT_GROUP 3

#define MAX_SPECIAL_SIMBOL_LEN 2