#include  <stdio.h>
#include  <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wait.h> 
#define MAX_INPUT_SIZE 1024 
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64 

void forkRun(char** tokens,int existPipe,int*fd,int forkNum, int pidNum);
int checkPipe(char** token, int* index); void pipeRun(char **token,int*fd); /* Splits the string by space and returns the array of tokens
 *
 */ char **tokenize(char *line) {
     char **tokens = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
     char *token = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
     int i, tokenIndex = 0, tokenNo = 0; for(i =0; i < strlen(line); i++){

         char readChar = line[i];

         if (readChar == ' ' || readChar == '\n' || readChar == '\t'){
             token[tokenIndex] = '\0';
             if (tokenIndex != 0){
                 tokens[tokenNo] = (char*)malloc(MAX_TOKEN_SIZE*sizeof(char));
                 strcpy(tokens[tokenNo++], token);
                 tokenIndex = 0; }
         } else {
             token[tokenIndex++] = readChar;
         } } 
     free(token); tokens[tokenNo] = NULL ;
     return tokens;
 }


int main(int argc, char* argv[]) 
{
    char  line[MAX_INPUT_SIZE];            
    char  **tokens;              
    int i;
    FILE* fp;
    if(argc == 2) {
        fp = fopen(argv[1],"r");
        if(fp < 0) {
            printf("File doesn't exists.");
            return -1;
        }
    }

    while(1) 
    {	
        /* BEGIN: TAKING INPUT */
        bzero(line, sizeof(line));
        if(argc == 2) { // batch mode
            if(fgets(line, sizeof(line), fp) == NULL) { // file reading finished
                break;	    
            }
            line[strlen(line) - 1] = '\0';
        } else { // interactive mode
            printf("$ ");
            scanf("%[^\n]", line);
            getchar();
        }

        line[strlen(line)] = '\n'; //terminate with new line
        tokens = tokenize(line);


        int status;
        int pipeNum=-1;  //pipe num
        int forkNum =0;
        int fd[2];
        int k=0;

        int index[MAX_NUM_TOKENS]={0};     //0로 초기화
        int existPipe[MAX_NUM_TOKENS] = {0};    //0로 초기화
        pipeNum = checkPipe(tokens,index);  //pipe 개수 

        //index에는 해당 파이프 다음 token 가르킴

        for(int i=0; i<pipeNum; i++)
        {
            if(index[i+1]!=0)
                existPipe[i]=1;
            else{break;}
        }   //exist pipe check

        forkNum = pipeNum; //생성할 fork의 개수

        int pids[forkNum+1];   //생성할 자식 프로세스 

        if(tokens !=NULL)
        {


            if(pipeNum!=0)
            {
                if(pipe(fd)==-1){
                    fprintf(stderr,"pipe error\n");
                }
                dup2(fd[0],0);  //읽기복사
                dup2(fd[1],0);  //출력복사
            }
            //pipe create....
        }

        // 토큰값 존재시에만 자식프로세스 생성 
        while(k<=forkNum)
        {
            int pos = index[k];       

            pids[k] = fork();
            //fork create...

            if(pids[k]<0){
                fprintf(stderr, "Fork Failed\n");
            }
            else if(pids[k] ==0){
                //child_process
                forkRun(&tokens[pos],existPipe[k],fd,forkNum,k); //process 실행 (배치식 모드)
            }
            else{
                //parent_process
                wait(&status);
                if(WIFEXITED(status))
                    printf("\n자식 프로세스 정상 종료\n\n");//정상종료

                else{
                    perror("\n자식 프로세스 회수 되지않음\n");
                        exit(pids[k]);
                        perror("\n회수 되지않은 자식 프로세스 종료\n");
                }
            }
            k++;
        }

        // Freeing the allocated memory	
        for(i=0;tokens[i]!=NULL;i++){
            free(tokens[i]);
        }
        free(tokens);
    }
    return 0;
}

void forkRun(char **tokens,int existPipe,int *fd, int forkNum,int pidNum)
{
    char command[MAX_TOKEN_SIZE]="\0";
    char options[MAX_NUM_TOKENS][MAX_TOKEN_SIZE];
    memset(options,'\0',MAX_NUM_TOKENS*MAX_TOKEN_SIZE);
    int optionNum =0;
    int pipeNum=-1;
    strcat(command,"/bin/");
    strcat(command,tokens[0]);  //ls cat ...

    int file;
    char fname[] = "temp.txt";
    char input[MAX_INPUT_SIZE];
    memset(input,0,MAX_INPUT_SIZE);

     if(forkNum == pidNum && forkNum!=0)
    {
        int len = read(fd[0],input,MAX_INPUT_SIZE);
        strcat(input,"\0");
        file = open("temp.txt",O_CREAT|O_RDWR|O_TRUNC,0644);
        if(write(file,input,len)==-1) 
            fprintf(stderr,"write error!\n");
        //pipe를 통한 결과 값 저장
        //파일 생성 및 fd 값 write
        close(file);
        close(fd[1]);
        close(fd[0]);
    }
     // 마지막 명령어의 경우
     else if(existPipe ==1 && forkNum != pidNum)
    {
        if(pidNum !=0)
        {
        int len = read(fd[0],input,MAX_INPUT_SIZE);
        strcat(input,"\0");
        file = open("temp.txt",O_CREAT|O_RDWR|O_TRUNC,0644);

        if(write(file,input,len)==-1) 
            fprintf(stderr,"write error!\n");
        //pipe를 통한 결과 값 저장
        //파일 생성 및 fd 값 write
        close(file);
        }
    }
    //파이프 결과 

    int i =1;
   
    for(i; i<MAX_NUM_TOKENS; i++)
    {
        if(tokens[i] !=NULL)
        {
            if(strcmp(tokens[i],"|")==0)
            {
                pipeNum = i; //save pipe position
               break;
            }

            strcat(options[i-1],tokens[i]);
            memset(options[i],'\0',MAX_TOKEN_SIZE);
            optionNum++;
        }
        else
        {
            break;
        }
    }
    
    if(strcmp(command,"/bin/grep")==0)
    {
        strcat(options[i-1],fname);
        memset(options[i],'\0',MAX_TOKEN_SIZE);
        optionNum++;   
        i++;
    }
    //strcpy token -> options...

    char **argv;
    argv = (char**)malloc((MAX_NUM_TOKENS)*sizeof(char*));

    for(int i=0; i<MAX_NUM_TOKENS;i++)
    {
        argv[i]= (char*)malloc(sizeof(char)*MAX_TOKEN_SIZE);
        memset(argv[i],'\0',MAX_TOKEN_SIZE);
    }

    if(optionNum==0)
    {
        strcpy(argv[0],command);
        if(pidNum ==0)
            argv[1] = NULL;
    }
    else
    {
        for(int i=0; i<optionNum; i++)
        {
            strcpy(argv[i+1],options[i]);

            if(i==optionNum-1)
                argv[i+2]=NULL;
        }

    } 
    
        if(pidNum!=0)
    { 
        int k;
        if(optionNum==0)
            k=1;
        else
            k=optionNum+1; 

        if(strcmp(command,"/bin/grep")!=0)
        {
            char *ptr;
            char temp[MAX_INPUT_SIZE];
            strcpy(temp,input);
            ptr = strtok(temp,"\n");

            while(ptr!=NULL)
            {
                strcpy(argv[k],ptr);
                ptr = strtok(NULL,"\n");
                k++;
            }
            if(ptr ==NULL)
                argv[k] =NULL;
                   
        }
        else if(strcmp(command,"/bin/grep")==0)
        {
            argv[k] =NULL;
        }
    }
    //pipe process adopt...
    //

    //case:1 non_option...
    if(optionNum==0)
    {
        if(pipeNum!=-1 || forkNum != pidNum) 
        { 
           dup2(fd[1],1);
        }
        if(execv(command,argv)==-1)
            fprintf(stderr,"SSUShell : Incorrect command\n");
    }
    //case:2 option_exist...
    else
    {
        //commad is echo case
        if(strcmp(command,"/bin/echo")==0)
        {
            if(pipeNum != -1 || forkNum != pidNum)
            {
                dup2(fd[1],1);
            }

            strcpy(argv[0],tokens[0]);
            for(int i=1; i<optionNum+1;i++)
            {
                if(tokens[i+1]!=NULL) 
                    strcat(argv[1],options[i+1]);
            }
            argv[optionNum+1]=NULL;
            if(execvp(tokens[0],argv)==-1)
                fprintf(stderr,"SSUShell : Incorrect command\n");
        }
        //else command case
        else
        {
            if(pipeNum != -1 || forkNum != pidNum)
            {       
                dup2(fd[1],1);
            }
            //pipe connect..

            if(execv(command,argv)==-1)
                fprintf(stderr,"SSUShell : Incorrect command\n");
        }
    }
    return;
}

int checkPipe(char **token,int* index)
{
    int pipeNum=0; //non_pipe == 0

    for(int i=0; i<MAX_NUM_TOKENS; i++)
    {
        if(token[i]==NULL)
            break;
        if(strcmp(token[i],"|")==0)
        {
            pipeNum++;  //파이프 개수 저장
           index[pipeNum] = i+1; //파이프의 index 위치 저장 
           //처음 index[0] 은 0으로 고정
        }
    }
    return pipeNum;
}
