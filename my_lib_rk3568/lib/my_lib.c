#include<stdio.h>
#include<fcntl.h>
#include<unistd.h>
#include <my_lib.h>
int my_gpio(char *gpio,int a)
{
    a=a+'0';
    int fd=open(gpio,O_RDWR);
    if(fd<0){printf("打开%s失败，请检查对应的gpio\n",gpio);return -1;}
    write(fd,&a,1);
    close(fd);
    return 0;
}