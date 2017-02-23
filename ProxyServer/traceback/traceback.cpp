#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "traceback.h"
/*
 * ��ӡջ֡
 * 
 * ͨ��backtrace,backtrace_symbols��ȡջ֡��Ϣ��Ȼ�����ܵ���ͨ��addr2line����
 * 
 */

int32_t myexec(const char *cmd) 
{
    FILE *pp = popen(cmd, "r"); //�����ܵ�
    if (!pp) 
    {
        return -1;
    }
    char tmp[1024];
       while (fgets(tmp, sizeof(tmp), pp) != NULL) 
    {
        if (tmp[strlen(tmp) - 1] == '\n') 
        {
            tmp[strlen(tmp) - 1] = '\0'; //ȥ�����з�
        }
        
        printf("%-30s",tmp);
    }
    printf("\n");
    pclose(pp); //�رչܵ�
    return 0;
}

void parseName(char * str,char *exeName,char *addr)
{
    char *strTemp = str;
    char * addrTemp;
    while (*strTemp != NULL)
    {
        if (*strTemp == '(')
            memcpy(exeName, str, strTemp - str);

        if (*strTemp == '[')
            addrTemp = strTemp;

        if (*strTemp == ']')
            memcpy(addr, str + (addrTemp - str) + 1, strTemp - addrTemp - 1);
        strTemp++;
    }
}

void print_trace(void)
{
    void *array[10];
    size_t size;
    char **strings;

    size = backtrace(array,10);
    strings = backtrace_symbols(array,size);
    
    printf("Obtained %zd stack frames.\n",size);
    char cmd[500] = {0}; 
    char exeName[100] = {0};
    char addr[100] = {0};
    for(size_t i = 0;i < size;i++)
    {
      memset(cmd,0,sizeof(cmd));
      memset(exeName,0,sizeof(exeName));
      memset(addr,0,sizeof(addr));
      
      parseName(strings[i],exeName,addr);
      printf("%-15s",addr);
      sprintf(cmd,"addr2line -f -e %s %s",exeName,addr);
      myexec(cmd);
    }     
}