#ifndef MY_LIB_H
#define MY_LIB_H

#define led1 "/sys/class/gpio/gpio120/value" 
#define led2 "/sys/class/gpio/gpio121/value" 
#define led3 "/sys/class/gpio/gpio123/value" 
#define led4 "/sys/class/gpio/gpio124/value"
#define bezzer "/sys/class/gpio/gpio111/value"
int my_gpio(char *gpio,int a);
#endif
