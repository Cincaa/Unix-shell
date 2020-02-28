#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <string.h>
#include <sys/stat.h>
#include <signal.h>
#include <limits.h>
#include <fcntl.h>
#include "../include/inc.h"
#define HISTORY_COUNT 20
#define READ_HEAD 0
#define WRITE_HEAD 1
char *hist[HISTORY_COUNT];  //aici salvez istoricul
int current = 0;    //numarul de comenzi din istoric
 
typedef struct command_info
{
    char *command;
    char **params;
    int paramsno;
} command_info;

int remove_extraspaces(char *str)
{
    int i, x;
    for(i=x=0; str[i]; ++i)
        if(!isspace(str[i]) || (i > 0 && !isspace(str[i-1])))
            str[x++] = str[i];
    str[x] = '\0';
    if(isspace(str[strlen(str)-1])) str[strlen(str)-1] = '\0';
}

int history(char *hist[], int current)
{
    //functia asta afiseaza o lista cu ultimele 20 de comenzi in ordine cronologica
        int i = current;
        int hist_num = 1;
 
        do {
                if (hist[i]) {
                        printf("%4d  %s\n", hist_num, hist[i]);
                        hist_num++;
                }
 
                i = (i + 1) % HISTORY_COUNT;
 
        } while (i != current-1);
 
        return 0;
}

int clear_history(char *hist[])
{
    //Pretty straightforworard
    //Elibereaza istoricul
        int i;
 
        for (i = 0; i < HISTORY_COUNT; i++) {
                free(hist[i]);
                hist[i] = NULL;
        }
        current = 0;
        return 0;
}

int suspend(command_info cmd)
{
    if(cmd.paramsno < 1 || cmd.paramsno > 1)
    {
        printf("Invalid parameters!\n");
        return 1;
    }
    kill(atoi(cmd.params[0]), SIGSTOP);
    return 1;
}

int resume(command_info cmd)
{
    if(cmd.paramsno < 1 || cmd.paramsno > 1)
    {
        printf("Invalid parameters!\n");
        return 1;
    }
    //printf("ID vechi: %d\n", getpid());
    kill(atoi(cmd.params[0]), SIGCONT);
    //printf("ID nou: %d\n", getpid());
    return 1;
}

int GetUserInput(char * input)
{
    char buff[200];
    fgets(buff, sizeof(buff), stdin);
    buff[strlen(buff)-1] = '\0'; //stergere \n de la final
    if(strlen(buff) > 0)
    {
        //Adauga comanda in history
        
        remove_extraspaces(buff); //stergere spatii extra

        hist[current] = strdup(buff);
        current = (current + 1) % HISTORY_COUNT;
 
        strcpy(input, buff);
    }
    else
    {
        printf("Empty input. Please type a command.\n");
        return 1;
    }
    return 0;
}

int ShowUserTerminalInfo()
{
    //Luam directorul curent al utilizatorului (cwd)
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd() error");
        return 1;
    }
 
    //Eliminam path-ul catre home al utilizatorului din cwd
    strcpy(cwd, cwd + strlen(getenv("HOME")));
 
    printf("CTerminal:~%s:$ ", cwd);
    return 0;
}
 
int GetInputTokensNumber(char *input)
{
    int count = 0;
    const char *delimiter = " ";
    char *tmp = (char*) malloc(sizeof(char) * sizeof(input));
    strcpy(tmp, input);
    while( tmp = strstr( tmp + 1, delimiter)){
        count++;
    }
    free(tmp);
    return count + 1;
}
 
char** TokenizeInput(char *input)
{
    //Returneaza un array de cuvinte (comenzi/parametrii/semne speciale/pipes/etc..)
    int nr_tokens = GetInputTokensNumber(input);
    char **tokens = (char**) malloc(sizeof(char*) * (nr_tokens));
   
    int i = 0;
    char *tmp = (char*) malloc(sizeof(char) * strlen(input));
    strncpy(tmp, input, strlen(input));
   
    tmp = strtok(tmp, " "); //token-izare dupa spatii
    while(tmp != NULL)
    {
        tokens[i] = (char*) malloc(sizeof(char) * strlen(tmp));
        strncpy(tokens[i], tmp, strlen(tmp));
        tmp = strtok(NULL, " ");
        i++;
    }
    free(tmp);
    return tokens;
}

int ExecuteCommand(command_info cmd)
{
    //Returneaza statusul procesului copil
    //-1 in caz de fail

    //Construire array de parametrii pentru execvp
    char **params = (char**) malloc(sizeof(char*) * (cmd.paramsno + 2));

    //Primul parametrul este numele programului.
    params[0] = cmd.command;

    for(int i = 1; i < cmd.paramsno + 1; i++)
    {
        params[i] = cmd.params[i-1]; 
    }

    //Adaugare null la finalul parametriilor
    params[cmd.paramsno + 1] = NULL;


    pid_t child_pid = fork();
    int child_exit_code = -1;
    if(child_pid < 0)
    {
        printf("Couldn't create the child process for command %s\n", cmd.command);
        return -1;
    }
    else
    {
        if(child_pid == 0) //Cod executat de procesul copil
        {
            int ret = execvp(params[0], params);
            if(ret < 0) //executa comanda incercand toate path-urile din variablia PATH
            {
                printf("Invalid command\n");
                //exit(0);
            }
            exit(ret); //Terminare executie copil. Return termina doar executia functiei, sarind mai departe in executia main-ului. 
        }
        else //Cod executat de parinte (terminal)
        {
            int status;
            wait(&status); //Asteapta executia copilului
            if( WIFEXITED(status) )
            {
                child_exit_code = WEXITSTATUS(status);
            }
            else child_exit_code = -1; 
        }
    }
    free(params);
    return child_exit_code;
}

int cd(char* path){
    if(chdir(path)==-1){ //se incearca mutarea in alt director 
        return -1;//fail
    }
    return 1;
}

int ExecuteBuiltInCommand(command_info cmd)
{
    //Returneaza 0 daca s-a executat o comanda custom. 1 altfel
 
    if (strcmp(cmd.command, "history") == 0)
    {
        history(hist, current);
        return 0;
    }
    else if (strcmp(cmd.command, "hc") == 0)
    {
        clear_history(hist);
        return 0;
    }
    else if(strcmp(cmd.command, "resume") == 0)
    {
        resume(cmd);
        return 0;
    }
    else if(strcmp(cmd.command, "suspend") == 0)
    {
        suspend(cmd);
        return 0;
    }
    else if(strcmp(cmd.command, "cd") == 0)
    {
        printf("1: %s\n2:%s\n", cmd.params[0], cmd.params[1]);
        char path[200];
        memset(path, 0, 200);
        getcwd(path, 200);
        strcat(path, "/");
        strcat(path, cmd.params[0]);
        printf("PATH: %s\n", path);
        if(cd(path)==-1){
            printf("Nu exista fisierul!\n");
            return 1;
        }
        return 0;
    }
    return 1;
}

int isBuiltInCommand(command_info cmd)
{
    //Returneaza 1 daca s-a este o comanda custom. 0 altfel
 
    if (strcmp(cmd.command, "history") == 0)
    {
        return 1;
    }
    else if (strcmp(cmd.command, "hc") == 0)
    {
        return 1;
    }
    else if(strcmp(cmd.command, "suspend") == 0)
    {
        return 1;
    }
    else if(strcmp(cmd.command, "resume") == 0)
    {
        return 1;
    }
    else if(strcmp(cmd.command, "cd") == 0)
    {
        return 1;
    }
    return 0;
}

int isDirectory(const char *path) {
   struct stat statbuf;
   if (stat(path, &statbuf) != 0)
       return 0;
   return S_ISDIR(statbuf.st_mode);
}

int isCommand(command_info cmd){

    if(strcmp(cmd.command, "..") == 0) // .. este comanda si nu vrem asta!
        return 0;
    char *env = getenv("PATH");
    char path[200];
    strncpy(path, env, sizeof(path));
    char *p = strtok(path, ":");
    while(p!=NULL)
    {
        char command[200];
        strcpy(command, p);
        strcat(command, "/");
        strcat(command,cmd.command);
        if( access(command, F_OK)==0)
        {
            return 1;
        }
        p = strtok(NULL, ":");
    }

    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd() error");
        return 1;
    }

    return isBuiltInCommand(cmd);
}

/*
int executePiped(command_info cmd1, command_info cmd2){
    printf("Executing piped: %s and %s\n", cmd1.command, cmd2.command);
    for(int i = 0; i < cmd1.paramsno; i++) printf("Params1: %s\n", cmd1.params[i]);
    for(int i = 0; i < cmd2.paramsno; i++) printf("Params2: %s\n", cmd2.params[i]);
	//Construire array de parametrii pentru execvp
    char **params1 = (char**) malloc(sizeof(char*) * cmd1.paramsno + 2);
    char **params2 = (char**) malloc(sizeof(char*) * cmd2.paramsno + 2);
    //Primul parametrul este numele programului.
    params1[0] = cmd1.command;
    params2[0] = cmd2.command;
    for(int i = 1; i < cmd1.paramsno + 1; i++)
    {
        params1[i] = cmd1.params[i-1]; 
    }
    for(int i = 1; i < cmd2.paramsno + 1; i++)
    {
        params2[i] = cmd2.params[i-1]; 
    }

    //Adaugare null la finalul parametriilor
    params1[cmd1.paramsno + 1] = NULL;
    params2[cmd2.paramsno + 1] = NULL;
    pid_t p2;
    int fd[2];

    if(pipe(fd)<0){
        perror("pipe creating was not successfull\n");
        printf("EROR PIPE\n");
        return 1;
    }

    pid_t p1 = fork();
    if(p1 < 0)
    {
        perror("couldn't fork");
        return 1;
    }
    else if(p1 == 0)
    {
        //The first command only needs to write so we can close the read end.
        //close(fd[READ_HEAD]); //close read fd
        
        close(fd[READ_HEAD]);
        //dup2(STDOUT_FILENO, fd[WRITE_HEAD]);
        dup2(fd[WRITE_HEAD], STDOUT_FILENO);//setare fd[WRITE_HEAD] la capatul de scriere al pipeului
        close(fd[WRITE_HEAD]);
        //close(fd[WRITE_HEAD]); //inchidem descriptorul de scriere intrucat orice modificare a lui STDOUT este oglindita si in fd[WRITE_HEAD].
        //acum orice scriere in STDOUT va afecta si descriptorul fd[WRITE_HEAD].
        
        //executia primei comenzi	
        if(execvp(params1[0], params1) < 0) //executa comanda incercand toate path-urile din variablia PATH
        {
            printf("Invalid command %s\n", params1[0]);
            exit(0);
        }
        exit(0); //opreste copilul in a continua executia (sa nu intre in while-ul principal)
    }
    else 
    {
        p2 = fork();
        if(p2 < 0)
        {
            perror("couldn't fork");
            return 1;
        }
        else if(p2 == 0)
        {
            //The second command only needs to read so we can close the write end.
            close(fd[WRITE_HEAD]);
            //dup2(STDIN_FILENO, fd[READ_HEAD]);
            dup2(fd[READ_HEAD], STDIN_FILENO); //duplicare STDIN. fd[READ_HEAD] se va comporta exact ca descriptorul lui STDIN.
            close(fd[READ_HEAD]);
            //close(fd[READ_HEAD]);

            if(execvp(params2[0], params2) < 0) //executa comanda incercand toate path-urile din variablia PATH
            {
                printf("Invalid command %s\n", params2[0]);
                exit(0);
            }
            exit(0); //opreste copilul in a continua executia (sa nu intre in while-ul principal)
            //printf("TREASDASD");
        }
        else 
        {
            //dup2(STDIN_FILENO, fd[READ_HEAD]);
            //dup2(STDOUT_FILENO, fd[WRITE_HEAD]);
            //printf("TESTTTTT");
        }
        
    }

    close(fd[READ_HEAD]);
    close(fd[WRITE_HEAD]);

    for(int i = 0; i < 2; i++)
    {
        wait(NULL);
    }
    //printf("DONEEE]\n");
		

	//mi-am cam prins urechile pe aici
	//nu am putut sa o testez pentru ca nu stiu cum sa obtin cele 2 comenzi
	// separate de | :)))
}*/

int GetSpecialCharacterType(char *c)
{
    if(strcmp(c, PIPE_CHAR) == 0)
        return IS_PIPE_CHAR;
    if(strcmp(c, REDIRECT_LEFT_CHAR) == 0)
        return IS_REDIRECT_LEFT_CHAR;
    if(strcmp(c, REDIRECT_RIGHT_CHAR) == 0)
        return IS_REDIRECT_LEFT_CHAR;
    if(strcmp(c, AND_OP_CHAR) == 0)
        return IS_AND_OP_CHAR;
    if(strcmp(c, OR_OP_CHAR) == 0)
        return IS_OR_OP_CHAR;
    return 0;
}

int IsParameter(char *c)
{
    if(c[0] == '-') return 1; //primul caracter este "-". Este sigur parametru
    //creare comanda intermediara pt verificare;
    command_info cmd;
    cmd.command = c;
    if(!isCommand(cmd)) return 1;
    return 0;
}

int Execute(command_info cmd)
{
    int exit_code = 1;
    if(isCommand(cmd) == 1)
    {
        if(!isBuiltInCommand(cmd))
        {
            exit_code = ExecuteCommand(cmd);
        }
        else 
        {
            if(ExecuteBuiltInCommand(cmd) == 1)
                exit_code = 0; //succes
            else 
                exit_code = 1; //fail
        }
    }
    else exit_code = 1;
    return exit_code;
}

int ExecuteGroupedPiped(command_info * group, int group_len)
{
    int fd[2];
    int fdd = 0;
    pipe(fd);

    int ret_val = 0;
    int comenzi_exec = 0;
    for(int i = 0; i < group_len; i++)
    {
        char **params1 = (char**) malloc(sizeof(char*) * group[i].paramsno + 2);
        params1[0] = group[i].command;
        for(int j = 1; j < group[i].paramsno + 1; j++)
        {
            params1[j] = group[i].params[j-1]; 
        }
        params1[group[i].paramsno + 1] = NULL;

        pid_t p1 = fork();
        if(p1 < 0)
        {
            perror("couldn't fork");
            return 1;
        }
        else if(p1 == 0)
        {
            dup2(fdd, STDIN_FILENO); //redirecteaza STDIN catre ffd
            if(i != group_len-1)
                dup2(fd[WRITE_HEAD], STDOUT_FILENO); //redirecteaza STDOUT catre fd[INPUT]
            close(fd[READ_HEAD]); //inchide capul de citire

            ret_val = Execute(group[i]);
            exit(ret_val); //opreste copilul in a continua executia (sa nu intre in while-ul principal)
        }
        else 
        {
            if(ret_val != 0)
            {
                break; //nu mai continua cu executia pipeului
            }
            close(fd[WRITE_HEAD]); // inchide fd[WRITE] pt a lasa celelalte procese sa scrie in STDOUT
            fdd = fd[READ_HEAD];
        }
        comenzi_exec ++;
        free(params1);
    }

    for(int i = 0; i < comenzi_exec; i++)
    {
        wait(NULL); //asteapta executia tuturor proceselor
    }
    return ret_val;
}

int ExecuteGrouped(command_info * group, int group_len, int group_type)
{
    int last_status = 0;
    if(group_len == 1) //doar o comanda
    {
        int exit_code = Execute(group[0]);
        return exit_code;
    }
    else
    {
        if(group_type == PIPED_GROUP)
        {
            return ExecuteGroupedPiped(group, group_len);
        }
    }
    return 1;
}

int ProcessInput(char *input, int tokensno, char **tokens)
{
    //Parsare

    command_info *commands_info = malloc(sizeof(command_info)*tokensno); // vector de comenzi
    int *params = malloc(sizeof(int)*tokensno); //alocare maxim tokensno parametrii pentru o singura comanda
    int curr_command_index = 0;
    int curr_parameter_index = 0;

    //operatori
    char **operators = (char**) malloc(sizeof(char*) * tokensno); //vector de operatori
    int *operators_type = (int*) malloc(sizeof(int) * tokensno);
    int operators_index = 0;

    for(int i = 0; i < tokensno; i++)
    {
        if(GetSpecialCharacterType(tokens[i]) == 0) //Nu e caracter special
        {
            if(!IsParameter(tokens[i])) //Nu e parametru
            {
                //Este comanda
                if(i != 0) //prima comanda nu are ce comanda anterioara sa finalizeze
                {
                    commands_info[curr_command_index].params = malloc(sizeof(char*) * (curr_parameter_index+1));
                    for(int h = 0; h < curr_parameter_index+1; h++)
                    {
                        commands_info[curr_command_index].params[h] = tokens[params[h]];
                    }
                    commands_info[curr_command_index].paramsno = curr_parameter_index;
                    curr_command_index ++;
                    curr_parameter_index = 0;
                }
                commands_info[curr_command_index].command = tokens[i];
            }
            else
            {
                params[curr_parameter_index] = i;
                curr_parameter_index ++;
            }
        }
        else //E operator/caracter special
        {
            operators[operators_index] = tokens[i];
            operators_type[operators_index] = GetSpecialCharacterType(tokens[i]);
            operators_index ++;
        }
    }

    //finalizare ultima comanda
    commands_info[curr_command_index].params = malloc(sizeof(char*) * (curr_parameter_index+1));
    for(int h = 0; h < curr_parameter_index+1; h++)
    {
        commands_info[curr_command_index].params[h] = tokens[params[h]];
    }
    commands_info[curr_command_index].paramsno = curr_parameter_index;
    curr_command_index++;


    command_info cmd;
    cmd.command = tokens[0];
    if(!isCommand(cmd)) //prima comanda trebuie sa fie valida
    {
        printf("Invalid file or directory %s\n", tokens[0]);
        return 1;
    }

    for(int i = 0; i < curr_command_index; i++)
    {
        printf("Comanda %d: %s\n", i, commands_info[i].command);
        for(int j = 0; j < commands_info[i].paramsno; j++)
        {
            printf("Parametrul %d: %s\n", j, commands_info[i].params[j]);
        }
    }

    /*for(int i = 0; i < operators_index; i ++)
    {
        printf("Operator: %s\n", operators[i]);
    }
    printf("\n");*/

    //alocare liste de grupuri de comenzi care trebuie executate cu pipe
    int *len_grup = (int*) malloc(sizeof(int) * (operators_index+1)); //vector de dimensiuni ale grupurilor 
    command_info **group_cmds = (command_info**) malloc(sizeof(command_info*)*(operators_index+1)); //vector de grupuri de comenzi
    for(int i = 0; i < (operators_index+1); i++)
    {
        group_cmds[i] = (command_info*) malloc(sizeof(command_info)*(operators_index+1));
    }
    int *group_type = (int*) malloc(sizeof(int)*(operators_index+1));


    //Executare pipe-uri prima data
    int pipe_found;
    int group_cmd_index = 0;
    int nr_groups = 0;

    //operatori noi
    char **operators2 = (char**) malloc(sizeof(char*) * tokensno); //maxim tokenso operatori
    int *operators_type2 = (int*) malloc(sizeof(int) * tokensno);
    int index_operator2 = 0;

    if(operators_index == 0)
    {
        return Execute(commands_info[0]);
    }

    for(int i = 0; i < operators_index; i++)
    {
        if(operators_type[i] == IS_PIPE_CHAR)
        {
            pipe_found = 1;
        }
        else pipe_found = 0;

        while(pipe_found == 1)
        {
            if(operators_type[i] == IS_PIPE_CHAR && i < operators_index)
            {
                group_cmds[nr_groups][group_cmd_index] = commands_info[i];
                group_type[nr_groups] = PIPED_GROUP;
                group_cmd_index ++;
                i++; //urmatorul operator
            }
            else 
            {
                group_cmds[nr_groups][group_cmd_index] = commands_info[i]; //adaugare comanda de dupa ultimul pipe
                len_grup[nr_groups] = group_cmd_index + 1;
                i++;
                pipe_found = 0;
                nr_groups ++; //trecem la urmatorul bloc de comenzi
                group_cmd_index = 0;

            }
        }
        //Grup de cate o comanda
        group_cmds[nr_groups][group_cmd_index] = commands_info[i];
        group_type[nr_groups] = NORMAL_GROUP;
        len_grup[nr_groups] = 1;
        nr_groups++;
    }
    // ls && C1 && C2 | C3 | C4 | C5 || C6 && C7 | C8 | C9 || C10

    /* stergere pipe din operatori*/
    index_operator2 = 0;
    for(int i = 0; i < operators_index; i++)
    {
        if(operators_type[i] != IS_PIPE_CHAR)
        {
            operators2[index_operator2] = operators[i];
            operators_type2[index_operator2] = operators_type[i];
            index_operator2 ++;
        }
    }
    
    int last_status;
    last_status = ExecuteGrouped(group_cmds[0], len_grup[0], group_type[0]); //executie primul grup

    for(int i = 1; i < nr_groups; i++)
    {
        if(operators_type2[i-1] == IS_AND_OP_CHAR) //ultimul operator
        {
            if(last_status == 0) // last==succes
                last_status = ExecuteGrouped(group_cmds[i], len_grup[i], group_type[i]);
            else 
                return 1; //abort
        }
        else if(operators_type2[i-1] == IS_OR_OP_CHAR)
        {
            if(last_status != 0) //last == fail
                last_status = ExecuteGrouped(group_cmds[i], len_grup[i], group_type[i]);
            else 
                return 1; //abort
        }
    }

    for(int i = 0; i < (operators_index+1); i++)
    {
        free(group_cmds[i]);
    }
    /*free(len_grup);
    free(commands_info);
    free(params);
    free(operators_type);
    free(operators_type2);
    free(operators2);
    free(group_type);
    free(group_cmds);
    for(int i = 0; i < tokensno; i++)
    {
        free(tokens[i]);
    }
    free(tokens);*/
    return 0;
}

int main(int argc, char **argv)
{
    //citeste inputul trimis de user
    //printf("\n");
    char input[200];
    char directory[200];
 
    for (int i = 0; i < HISTORY_COUNT; i++)
            hist[i] = NULL;
 
    while(1)
    {
        //Afisare numele consolei + directorul in care se afla user-ul (asemenea terminalului default)
        if(ShowUserTerminalInfo())
        {
            printf("Cannot get the current directory\n"); //eroare la getcwd()
            exit(0);
        }
       
        if(GetUserInput(input)) //returneaza 1 daca nu introduce nimic -> skip la urmatoarea comanda
            continue;
 
        //Procesare continut comanda
        ProcessInput(input, GetInputTokensNumber(input), TokenizeInput(input));
       
    }
    clear_history(hist);
    return 0;
}
