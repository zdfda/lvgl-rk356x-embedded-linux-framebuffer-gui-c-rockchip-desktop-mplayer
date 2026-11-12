#include<my_lib.h>
#include<unistd.h>
int main()
{
	my_gpio(led1,1);
	sleep(1);
	my_gpio(led2,1);
        sleep(1);
	my_gpio(led3,1);
        sleep(1);
	my_gpio(led4,1);
        sleep(1);
	return 0;
}
