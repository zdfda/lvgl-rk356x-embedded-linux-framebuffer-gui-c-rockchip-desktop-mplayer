#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include<stdio.h>
#include<fcntl.h>
#include<stdlib.h>
#include<stdint.h>
#include<string.h>
#include<dirent.h>
#include<sys/stat.h>
#include <signal.h>
#include <arpa/inet.h>
#include <poll.h>
#include <sys/mman.h>
#pragma pack(push,1)
#define N 8 //app个数
//全局变量
lv_obj_t *curtest_printf= NULL; 
static lv_obj_t *ta_pos,*ta_size,*ta_path_in,*ta_path_out;
static int pass_flag;
static char save_path[128]={0};
static int yx_cnt=0;
static lv_obj_t *yx_pos;
static int rw_x,rw_y,rw_x2,rw_y2;
#define led1 "/sys/class/gpio/gpio120/value" 
#define led2 "/sys/class/gpio/gpio121/value" 
#define led3 "/sys/class/gpio/gpio123/value" 
#define led4 "/sys/class/gpio/gpio124/value"
#define buzzer "/sys/class/gpio/gpio111/value"
//功能函数
lv_obj_t* creat_ck(int a,int b,int c,int d,lv_obj_t* p);
lv_obj_t* creat_button(lv_obj_t* p,int x,int y);
lv_obj_t* create_label(lv_obj_t* p,int x,int y);
lv_obj_t* show_bmp(char *p,lv_obj_t* p2);
lv_obj_t* creat_slide(lv_obj_t* p,int x,int y);
lv_obj_t* creat_canvas(lv_obj_t* p,int w,int l,char * raw,int date);
void my_scanf_xs();
void creat_close_button(lv_obj_t* p);
void jg(char* p);
lv_obj_t* tc(char* p);
void keyboard_ck();
lv_obj_t* creat_text(lv_obj_t*,int x,int y,char* label_txt);
//回调函数
void my_free(lv_event_t *e);
void control_app(lv_event_t *e);
void control_gpio(lv_event_t *e);
void my_close(lv_event_t *e);
void creat_file(lv_event_t* e);
void creat_dir(lv_event_t* e);
void my_remove(lv_event_t* e);
void del_timer(lv_event_t *e);
//主函数
void app();
//app5函数
void gpio_int();
int my_gpio(char *gpio,int a);
void app_mode(lv_event_t * e);
void app5(lv_obj_t* canvas);
static int my_strcmp(const char *s,const char *ext)
{
    size_t ls=strlen(s),le = strlen(ext);
    return ls>=le&&strcmp(s+ls-le,ext)==0;
}
struct app6_t
{
    int cnt;//计时，隐藏按钮
    lv_obj_t* but[3];//功能按钮
    char* path;//正在播放视频的地址，用于切换
    char flag;//暂停标志位
    int sum;//视频文件数量
    int x,y;//视频位置
    int sizex,sizey;//视频窗口大小
    lv_obj_t* ta2;
    lv_obj_t* ta3;
    pid_t pid;//子进程pid
    lv_obj_t* obj;
}app6_t;
struct use
{
    char pass[128];
    char name[128];
    char photo[128];
}use_date={0};
int main(void)
{
    lv_init();
    gpio_int();
    /*Linux frame buffer device init*/
    lv_display_t * disp = lv_linux_fbdev_create();
    lv_linux_fbdev_set_file(disp, "/dev/fb0");
    lv_indev_t * indev = lv_evdev_create(LV_INDEV_TYPE_POINTER, "/dev/input/event6");
    lv_obj_set_style_text_font(lv_screen_active(), &lv_font_simsun_16_cjk, 0);
    /*Create a Demo*/
    //lv_demo_widgets();
    //lv_demo_widgets_start_slideshow();
    app();
    keyboard_ck();
    while(1) 
    {
        lv_timer_handler();
        usleep(5000);
    }
    return 0;
}
void jg(char* p)
{
    lv_obj_t* obj=creat_ck(0,0,512,300,lv_layer_top());
    lv_obj_center(obj);
    lv_obj_t* label=create_label(obj,0,0);
    lv_obj_center(label);
    lv_label_set_text(label,p);
    lv_obj_t* button=creat_button(obj,0,100);
    lv_obj_center(button);
    lv_obj_set_size(button,200,100);
    lv_obj_add_event_cb(button,my_close,LV_EVENT_CLICKED,obj);
}
void my_printf(lv_event_t *e)
{
    lv_obj_t* button=lv_event_get_target(e);
    lv_obj_t* label=lv_obj_get_child(button,0);
    char* p=lv_label_get_text(label);
    if(curtest_printf==NULL||!lv_obj_is_valid(curtest_printf)){jg("no text can't put");return;}
    lv_obj_t* ta=curtest_printf;
    if (strcmp(p,"ok")==0) {lv_obj_send_event(ta,LV_EVENT_READY, NULL);}
    else if (strcmp(p,"back")==0) {lv_textarea_delete_char(ta);}
    else if (strcmp(p,"qk")==0){lv_textarea_set_text(ta,"");}
    else {lv_textarea_add_text(ta,p);}
}
void my_scanf_xs_l(lv_event_t *e)
{
    lv_obj_t* obj=lv_event_get_user_data(e);
    static char flag=0;
    flag++;
    flag=flag%2;
    if(flag==1){lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);}
    else{lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);}
}
void my_scanf_xs_r(lv_event_t *e)
{
    lv_obj_t* obj=lv_event_get_user_data(e);
    static char flag=0;
    flag++;
    flag=flag%2;
    if(flag==1){lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);}
    else{lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);}
}
void keyboard_ck()
{   
    lv_obj_t* slide_l=creat_slide(lv_layer_top(),0,300);
    lv_obj_t* slide_r=creat_slide(lv_layer_top(),824,300);
    lv_obj_t* obj_l=creat_ck(0,0,100,600,lv_layer_top());
    lv_obj_t* obj_r=creat_ck(924,0,100,600,lv_layer_top());
    lv_obj_t* button[51],*label[51];
    int t;
    for(int i=0;i<26;i++)
    {
        button[i]=creat_button(obj_l,0,50*i);
        lv_obj_set_size(button[i],100,50);
        label[i]=create_label(button[i],0,0);
        lv_obj_center(label[i]);
        lv_label_set_text_fmt(label[i],"%c",i+'a');
        lv_obj_add_event_cb(button[i],my_printf,LV_EVENT_CLICKED,obj_r);
    }
    for(int i=26;i<51;i++)
    {
        t=i-26;
        button[i]=creat_button(obj_r,0,50*t);
        lv_obj_set_size(button[i],100,50);
        label[i]=create_label(button[i],0,0);
        lv_obj_center(label[i]);
        lv_label_set_text_fmt(label[i],"%c",t+'.');
        lv_obj_add_event_cb(button[i],my_printf,LV_EVENT_CLICKED,obj_r);
    }
    lv_label_set_text(label[38],"_");
    lv_label_set_text(label[41]," ");
    lv_label_set_text(label[39],"back");
    lv_label_set_text(label[40],"ok");
    lv_label_set_text(label[42],"qk");
    lv_label_set_text(label[43],"@");
    lv_label_set_text(label[44],"#");
    lv_label_set_text(label[45],"~");
    lv_label_set_text(label[46],"$");
    lv_label_set_text(label[47],"/my_photo");
    lv_label_set_text(label[48],"/my_studio");
    lv_label_set_text(label[49],"0 0");
    lv_label_set_text(label[50],"512 300");
    lv_obj_add_flag(obj_l, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(obj_r, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_size(slide_l,200,50);
    lv_obj_set_size(slide_r,200,50);
    lv_slider_set_range(slide_l,0,1);
    lv_slider_set_range(slide_r,0,1);
    lv_slider_set_value(slide_r, 1, LV_ANIM_OFF);
    lv_obj_set_style_opa(slide_r, LV_OPA_TRANSP, 0); 
    lv_obj_set_style_opa(slide_l, LV_OPA_TRANSP, 0); 
    lv_obj_add_event_cb(slide_l,my_scanf_xs_l,LV_EVENT_VALUE_CHANGED,obj_l);
    lv_obj_add_event_cb(slide_r,my_scanf_xs_r,LV_EVENT_VALUE_CHANGED,obj_r);
}
lv_obj_t* show_bmp(char *p,lv_obj_t* p2)
{
    
    lv_obj_t* img=lv_image_create(p2);
    char a[128]={0};
    strcat(a,"A:");
    strcat(a,p);
    lv_image_set_src(img,a);
    lv_obj_center(img);
    lv_obj_set_style_border_width(img, 0, 0);
    return img;
}
lv_obj_t* creat_ck(int a,int b,int c,int d,lv_obj_t* p)
{
    lv_obj_t * obj=lv_obj_create(p);
    lv_obj_set_size(obj,c,d);
    lv_obj_set_pos(obj,a,b); 
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    return obj;
}
lv_obj_t* creat_button(lv_obj_t* p,int x ,int y)
{
    lv_obj_t* button= lv_btn_create(p);
    lv_obj_set_pos(button,x,y);
    lv_obj_set_style_bg_opa(button, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(button, 0, 0);
    return button;
}
lv_obj_t* create_label(lv_obj_t* p, int x,int y)
{
    lv_obj_t * label = lv_label_create(p);
    lv_obj_set_pos(label,x,y);
    lv_obj_set_style_bg_opa(label, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(label, 0, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0), 0); 
    return label;
}
lv_obj_t* creat_slide(lv_obj_t* p,int x,int y)
{
    lv_obj_t* slide=lv_slider_create(p);
    lv_obj_set_size(slide,200,50);
    lv_obj_set_pos(slide,x,y);
    lv_obj_set_style_border_width(slide, 0, 0);
    lv_obj_set_style_bg_main_opa(slide,LV_OPA_TRANSP,0);
    //lv_obj_set_style_opa(slide, LV_OPA_TRANSP, 0); 
    return slide;
}
void my_free(lv_event_t * e)
{
    void * user_data = lv_event_get_user_data(e); 
    free(user_data);
}
lv_obj_t * creat_canvas(lv_obj_t* p,int w,int l,char * raw,int date)
{
    int data_size = w*l*4;
    char *p2 = malloc(data_size);
    FILE *fp = fopen(raw,"r");
    if(fp==NULL)
    {
        jg("no photo,please Settings");
        for(int i = 0; i < data_size; i += 4)
        {
            //char t=rand()%256;
            char t=0;
            p2[i]=t;   
            p2[i+1]=t;   
            p2[i+2]=t;   
            p2[i+3]=date; 
        }
    }
    else
    {
        fread(p2,1,data_size,fp);
        fclose(fp);
        for(int i=3;i<data_size;i+=4)
        p2[i]=date;
    }
    lv_obj_t *canvas=lv_canvas_create(p);
    lv_canvas_set_buffer(canvas,p2,w,l,LV_COLOR_FORMAT_ARGB8888);
    lv_obj_add_event_cb(canvas,my_free,LV_EVENT_DELETE,p2);
    lv_obj_set_style_bg_main_opa(canvas,LV_OPA_TRANSP,0);
    return canvas;
}
void my_close(lv_event_t* e)
{
    lv_obj_t *a=lv_event_get_user_data(e);
    lv_obj_del(a);
}
void creat_close_button(lv_obj_t* p)
{
    lv_obj_t *close_btn=lv_btn_create(p);
    lv_obj_set_size(close_btn,50,50);
    lv_obj_align(close_btn,LV_ALIGN_TOP_RIGHT, 0, 0);  
    lv_obj_set_style_bg_opa(close_btn,LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(close_btn,0,0);
    lv_obj_t *label=lv_label_create(close_btn);
    lv_label_set_text(label,"X"); 
    lv_obj_set_style_text_color(label, lv_color_hex(0xFF0000), 0); 
    lv_obj_center(label);
    lv_obj_add_event_cb(close_btn,my_close, LV_EVENT_CLICKED, p);
}
void text_clicked(lv_event_t* e)
{
    lv_obj_t *ta=lv_event_get_target(e);
    lv_obj_t *label=lv_event_get_user_data(e);
    if(label==NULL){return;}
    lv_obj_remove_event_cb(ta, text_clicked);
    lv_obj_del(label);
}
void p_mode(lv_event_t * e)
{
    lv_obj_t *ta=lv_event_get_target(e);
    curtest_printf=ta;
}
lv_obj_t* creat_text(lv_obj_t* p,int x,int y,char* label_txt)
{
    lv_obj_t* ta=lv_textarea_create(p);
    lv_obj_set_pos(ta,x,y);
    lv_obj_set_size(ta,200,75);
    lv_textarea_set_one_line(ta, true);
    lv_obj_t* label=create_label(ta,0,0);
    lv_obj_center(label);
    lv_label_set_text(label,label_txt);
    curtest_printf=ta;
    lv_obj_add_event_cb(ta,text_clicked,LV_EVENT_CLICKED,label);
    lv_obj_add_event_cb(ta,p_mode, LV_EVENT_CLICKED, NULL);
    return ta;
}
void enter_dir(lv_event_t* e)
{
    lv_obj_t* ta=lv_event_get_user_data(e);//输目录的文本框
    lv_obj_t* button_last=lv_event_get_target(e);
    lv_obj_t* obj2=lv_obj_get_parent(button_last);
    lv_obj_t* obj=lv_obj_get_parent(obj2);
    lv_obj_t* label_last = lv_obj_get_child(button_last,0);
    char* path=lv_label_get_text(label_last);
    lv_textarea_set_text(ta,path);
    DIR* dirp=opendir(path);
    if(dirp==NULL){jg("this isn't open");return;}
    char t2[64]={0};
    strcpy(t2,path);
    lv_obj_del(obj2);
    obj2=creat_ck(0,0,200,200,obj);
    FILE * p=fopen("/my_exe.log","w+");
    int i=0;
    struct dirent* a;
    char t[64]={0};
    char date[64]={0};
    while(a=readdir(dirp))
    {
        if(strcmp(a->d_name,".")==0||strcmp(a->d_name,"..")==0){continue;}
        i++; 
        strcpy(date,t2);
        if(strcmp(date,"/")!=0){strcat(date,"/");}
        printf("date2%s\n",date);
        strcat(date,a->d_name);
        printf("date3%s\n",date);
        fprintf(p,"%s\n",date);   
        strcpy(date,t);
        printf("date4%s\n",date);
    }
    lv_obj_t *button[i],*label[i];
    fseek(p,0,SEEK_SET);
    for(int j=0;j<i;j++)
    {
        button[j]=creat_button(obj2,0,50*j);
        lv_obj_set_size(button[j],200,50);
        label[j]=create_label(button[j],0,0);
        lv_obj_center(label[j]);
        fscanf(p,"%s",date);
        lv_label_set_text(label[j],date);
        lv_obj_set_width(label[j], 200);
        lv_label_set_long_mode(label[j], LV_LABEL_LONG_SCROLL_CIRCULAR);
        strcpy(date,t);
        lv_obj_add_event_cb(button[j],enter_dir,LV_EVENT_CLICKED,ta);
    }
    fclose(p);
    closedir(dirp); 
}
void show_dir(lv_event_t* e)
{
    lv_obj_t* ta_last=lv_event_get_target(e);
    lv_obj_t* obj=lv_event_get_user_data(e);
    lv_obj_t* obj2=creat_ck(0,0,200,200,obj);
    const char* path=lv_textarea_get_text(ta_last);
    int i=0;
    struct dirent* a;
    char date[64]={0};
    char t[64]={0};
    DIR* dirp=opendir(path);
    if(dirp==NULL){jg("path error");return;}
    FILE * p=fopen("/my_exe.log","w+");
    while(a=readdir(dirp))
    {
        if(strcmp(a->d_name,".")==0||strcmp(a->d_name,"..")==0){continue;}
        i++; 
        strcpy(date,path);
        if(strcmp(path,"/")!=0){strcat(date,"/");}
        strcat(date,a->d_name);
        fprintf(p,"%s\n",date);   
    }
    lv_obj_t *button[i],*label[i],*ta[3];
    fseek(p,0,SEEK_SET);
    for(int j=0;j<i;j++)
    {
        button[j]=creat_button(obj2,0,50*j);
        lv_obj_set_size(button[j],200,50);
        label[j]=create_label(button[j],0,0);
        lv_obj_center(label[j]);
        fscanf(p,"%s",date);
        lv_label_set_text(label[j],date);
        strcpy(date,t);
        lv_obj_add_event_cb(button[j],enter_dir,LV_EVENT_CLICKED,ta_last);
    }
    ta[0]=creat_text(obj,250,75,"input creat file name");
    lv_obj_add_event_cb(ta[0],creat_file,LV_EVENT_READY,ta_last);
    ta[1]=creat_text(obj,250,150,"input creat dir name");
    lv_obj_add_event_cb(ta[1],creat_dir,LV_EVENT_READY,ta_last);
    ta[2]=creat_text(obj,250,225,"input remove name");
    lv_obj_add_event_cb(ta[2],my_remove,LV_EVENT_READY,ta_last);
    fclose(p);
    return;
}   
void creat_file(lv_event_t* e)
{
    char a[128]={0};
    lv_obj_t* ta=lv_event_get_target(e);
    const char* p=lv_textarea_get_text(ta);
    lv_obj_t* ta_path=lv_event_get_user_data(e);
    lv_obj_t* obj=lv_obj_get_parent(ta_path);
    const char* path=lv_textarea_get_text(ta_path);
    if(path[0]!='/'){jg("form error");return;}
    strcpy(a,path);
    strcat(a,"/");
    strcat(a,p);
    FILE* p_file=fopen(a,"w+");
    if(p_file!=NULL){jg("creat file success");lv_obj_send_event(ta_path,LV_EVENT_READY,obj);}
    else{jg("creat file fail");return;}
    fclose(p_file);
}
void creat_dir(lv_event_t* e)
{
    char a[128]={0};
    lv_obj_t* ta=lv_event_get_target(e);
    const char* p=lv_textarea_get_text(ta);
    lv_obj_t* ta_path=lv_event_get_user_data(e);
    lv_obj_t* obj=lv_obj_get_parent(ta_path);
    const char* path=lv_textarea_get_text(ta_path);
    if(path[0]!='/'){jg("form error");return;}
    strcpy(a,path);
    strcat(a,"/");
    strcat(a,p);   
    if(mkdir(a,0755)==0){jg("creat dir success");lv_obj_send_event(ta_path,LV_EVENT_READY,obj);}
    else{jg("creat dir fail");return;}
}
void my_remove(lv_event_t* e)
{
    char a[128]={0};
    lv_obj_t* ta=lv_event_get_target(e);
    const char* p=lv_textarea_get_text(ta);
    lv_obj_t* ta_path=lv_event_get_user_data(e);
    lv_obj_t* obj=lv_obj_get_parent(ta_path);
    const char* path=lv_textarea_get_text(ta_path);
    if(path[0]!='/'){jg("form error");return;}
    if(p[0]==0){remove(path);jg("remove success");return;}
    strcpy(a,path);
    strcat(a,"/");
    strcat(a,p);   
    int fd=open(a,O_RDONLY);
    if(fd<0){jg("file isn't exit");return;}
    remove(a);
    jg("remove success");
    lv_obj_send_event(ta_path,LV_EVENT_READY,obj);
    close(fd);
}

void app1(lv_obj_t* canvas)
{
    lv_obj_t* obj=creat_ck(0,0,512,300,canvas);
    creat_close_button(obj);
    lv_obj_t* ta=creat_text(obj,250,0,"work path,input path");
    lv_obj_add_event_cb(ta,show_dir, LV_EVENT_READY,obj);
}
void gpio_int()//用c语言包装的函数要使用频繁fflush与fclose实现缓存与内核的通讯,所以用系统io
{
    int fd=open("/sys/class/gpio/export",O_WRONLY),fd2,fd3;
    fd2=open("/sys/class/gpio/gpio111/direction",O_WRONLY);
    if(write(fd2,"out",3)>0){printf("蜂鸣器成功使能\n");;close(fd2);}
    else{write(fd,"111",3);sleep(1);fd2=open("/sys/class/gpio/gpio111/direction",O_WRONLY);}//延时给内核创建目录的时间
    if(write(fd2,"out",3)>0){printf("蜂鸣器成功使能\n");close(fd2);}
    fd2=open("/sys/class/gpio/gpio120/direction",O_WRONLY);
    if(write(fd2,"out",3)>0){printf("LED1成功使能\n");close(fd2);}
    else{write(fd,"120",3);sleep(1);fd2=open("/sys/class/gpio/gpio120/direction",O_WRONLY);}
    if(write(fd2,"out",3)>0){printf("LED1成功使能\n");close(fd2);}
    fd2=open("/sys/class/gpio/gpio121/direction",O_WRONLY);
    if(write(fd2,"out",3)>0){printf("LED2成功使能\n");close(fd2);}
    else{write(fd,"121",3);sleep(1);fd2=open("/sys/class/gpio/gpio121/direction",O_WRONLY);}
    if(write(fd2,"out",3)>0){printf("LED2成功使能\n");close(fd2);}
    fd2=open("/sys/class/gpio/gpio123/direction",O_WRONLY);
    if(write(fd2,"out",3)>0){printf("LED3成功使能\n");close(fd2);}
    else{write(fd,"123",3);sleep(1); fd2=open("/sys/class/gpio/gpio123/direction",O_WRONLY);}
    if(write(fd2,"out",3)>0){printf("LED3成功使能\n");close(fd2);}
    fd2=open("/sys/class/gpio/gpio124/direction",O_WRONLY);
    if(write(fd2,"out",3)>0){printf("LED4成功使能\n");close(fd2);}
    else{write(fd,"124",3);sleep(1);fd2=open("/sys/class/gpio/gpio124/direction",O_WRONLY);}
    if(write(fd2,"out",3)>0){printf("LED4成功使能\n");close(fd2);}
    close(fd);
}
int my_gpio(char *gpio,int a)
{
    a=a+'0';
    int fd=open(gpio,O_RDWR);
    if(fd<0){printf("打开%s失败，请先使能gpio\n",gpio);return -1;}
    write(fd,&a,1);
    close(fd);
    return 0;
}
struct gpio
{
    lv_obj_t* label[5];
    lv_obj_t* slider[5];
    int cnt[5];   
}gpio;
void control_gpio(lv_event_t *e)
{
    lv_obj_t* slider=lv_event_get_target(e);
    int val=lv_slider_get_value(slider);
    int fd=open("/sys/class/backlight/backlight/brightness",O_WRONLY);
    char c[4];
    c[0]=val/100+'0';c[1]=val/10%10+'0';c[2]=val%10+'0';c[3]='\n';
    write(fd,c,4);
    close(fd);
}
void pwm_control(lv_timer_t *timer)
{
    
    int n[5];
    for(int i=0;i<5;i++)
    {
    gpio.cnt[i]++;    
    n[i]=lv_slider_get_value(gpio.slider[i]);
    gpio.cnt[i]=gpio.cnt[i]%20;
    }
    if(n[0]<=gpio.cnt[0])my_gpio(led1,0);
    else{my_gpio(led1,1);}
    if(n[1]<=gpio.cnt[1])my_gpio(led2,0);
    else{my_gpio(led2,1);}
    if(n[2]<=gpio.cnt[2])my_gpio(led3,0);
    else{my_gpio(led3,1);}
    if(n[3]<=gpio.cnt[3])my_gpio(led4,0);
    else{my_gpio(led4,1);}
    if(n[4]<=gpio.cnt[4])my_gpio(buzzer,0);
    else{my_gpio(buzzer,1);}
}
void app5(lv_obj_t* canvas)
{
    lv_obj_t* obj=creat_ck(256,100,512,300,canvas),*label[6],*slider[6];
    creat_close_button(obj);
    
    for(int i=0;i<6;i++)
    {
        label[i]=create_label(obj,0,50*i);
        lv_obj_set_size(label[i],100,50);
        lv_label_set_text_fmt(label[i],"contorl led%d",i+1);
        slider[i]=creat_slide(obj,200,50*i);
        lv_obj_set_size(slider[i],250,50);
        lv_slider_set_range(slider[i],0,20);
    }
    for(int i=0;i<5;i++) 
    {
    gpio.slider[i]=slider[i];
    gpio.cnt[i]=-1;
    }
    lv_obj_add_event_cb(slider[5],control_gpio, LV_EVENT_VALUE_CHANGED,label[5]);
    lv_label_set_text_fmt(label[4],"contorl buzzer");    
    lv_label_set_text_fmt(label[5],"back light");
    lv_slider_set_range(slider[5],0,255);
    lv_timer_t *timer=lv_timer_create(pwm_control,5,NULL); 
    lv_obj_add_event_cb(obj,del_timer,LV_EVENT_DELETE,timer);
}
void show_photo(lv_event_t *e)
{
    lv_obj_t* button_last=lv_event_get_target(e);
    lv_obj_t* label_last=lv_obj_get_child(button_last,0);
    lv_obj_t* ta=lv_event_get_user_data(e);
    char* path=lv_label_get_text(label_last);
    char a[128]={0};
    strcpy(a,path);
    lv_textarea_set_text(ta,path);
    if(my_strcmp(a, ".raw"))
    {
    lv_obj_t* canvas=creat_canvas(lv_screen_active(),1024,600,path,255);
    creat_close_button(canvas);
    return;
    }
    lv_obj_t* img=show_bmp(path,lv_screen_active());
    creat_close_button(img);
}
void my_led(char *p)//传入一个bmp图片的数据文件.raw，像素要为1024*600
{
    int fd=open(p,O_RDONLY); 
    int fd2=open("/dev/fb0",O_WRONLY);
    if(fd<0||fd2<0){printf("图片不存在，请在图片管理器中设置\n");return;}
    char *date=malloc(1024*600*4);
    memset(date,0,1024*600*4);
    read(fd,date,1024*600*4);
    write(fd2,date,1024*600*4);
    free(date);
    close(fd);
    close(fd2);
}
int my_sleep(int a,int fd)
{
    int b=a*10;
    uint8_t c[24];
    while(b--)
    {
        my_gpio(led1,0);my_gpio(led2,0);my_gpio(led3,0);my_gpio(led4,0);
        usleep(20000);
        my_gpio(led1,1);
        usleep(20000);
        my_gpio(led1,0);
        my_gpio(led2,1);
        usleep(20000);
        my_gpio(led2,0);
        my_gpio(led3,1);
        usleep(20000);
        my_gpio(led3,0);
        my_gpio(led4,1);
        usleep(19990);
        usleep(10);
        if(read(fd,c,24)>0)
        { 
            return 1;
        }
    }
    return 0;
}
void photo_tv(lv_event_t* e)
{
    lv_obj_t* obj2=lv_event_get_user_data(e);
    lv_obj_t* obj=lv_obj_get_parent(obj2);
    lv_obj_add_flag(obj,LV_OBJ_FLAG_HIDDEN);
    FILE *p_file=fopen("/exe.log/photo.txt","a+");
    fseek(p_file,0,SEEK_SET);
    char* a=malloc(200);
    int fd = open("/dev/input/event6", O_RDONLY|O_NONBLOCK);//windows系统头文件fcntl未定义O_NONBLOCK
    if(fd<0){return;}
    int flag2=0;
    while(1)
    {
        fseek(p_file,0,SEEK_SET);
        while(fscanf(p_file,"%s",a)==1)
        {
            my_led(a);
            int flag=my_sleep(2,fd);
            if (flag==1){my_gpio(buzzer,0);lv_obj_clear_flag(obj,LV_OBJ_FLAG_HIDDEN);return;}
            flag2=1;
        }
        if(flag2==0){jg("no photo");lv_obj_clear_flag(obj,LV_OBJ_FLAG_HIDDEN);break;}
    }
    free(a);
    close(fd);
    fclose(p_file);
}
void bz(lv_event_t* e)
{
    lv_obj_t* bz_setting=lv_event_get_target(e);
    const char* date=lv_textarea_get_text(bz_setting);
    FILE* p=fopen("/exe.log/pass.log","w+");
    fprintf(p,"%s\n",use_date.name);
    fprintf(p,"%s\n",use_date.pass);
    fprintf(p,"%s\n",date);
    fclose(p);
}
void show_raw_bmp(lv_event_t *e)
{
    lv_obj_t *dir_list_box= NULL;  
    lv_obj_t* ta=lv_event_get_target(e);
    const char* path=lv_textarea_get_text(ta);
    lv_obj_t* obj=lv_event_get_user_data(e);
    if (dir_list_box == NULL) {dir_list_box = creat_ck(0, 0, 200, 200,obj);
    } else {lv_obj_clean(dir_list_box);}
    lv_obj_t* obj2=creat_ck(0,0,200,200,obj);
    int i=0;
    struct dirent* a;
    char date[64]={0};
    char t[64]={0};
    DIR* dirp=opendir(path);
    if(dirp==NULL){jg("path error");return;}
    FILE * p=fopen("/exe.log/photo.txt","w+");
    while(a=readdir(dirp))
    {
        if(strcmp(a->d_name,".")==0||strcmp(a->d_name,"..")==0){continue;}
        if(!my_strcmp(a->d_name,".raw") &&!my_strcmp(a->d_name,".bmp"))
        {
            continue;
        }
        i++; 
        strcpy(date,path);
        strcat(date,"/");
        strcat(date,a->d_name);
        fprintf(p,"%s\n",date);   
    }
    lv_obj_t *button[i],*label[i],*button_tv,*label_tv,*bz_setting;
    fseek(p,0,SEEK_SET);
     bz_setting=creat_text(obj,250,150,"input bz name");
    lv_obj_add_event_cb(bz_setting,bz,LV_EVENT_READY,ta);
    for(int j=0;j<i;j++)
    {
        button[j]=creat_button(obj2,0,50*j);
        lv_obj_set_size(button[j],200,50);
        label[j]=create_label(button[j],0,0);
        lv_obj_center(label[j]);
        fscanf(p,"%s",date);
        lv_label_set_text(label[j],date);
        strcpy(date,t);
        lv_obj_add_event_cb(button[j],show_photo,LV_EVENT_CLICKED,bz_setting);
    }
    button_tv=creat_button(obj,250,75);
    lv_obj_add_event_cb(button_tv,photo_tv,LV_EVENT_CLICKED,obj);
    lv_obj_set_size(button_tv,200,75);
    label_tv=create_label(button_tv,0,0);
    lv_obj_center(label_tv);
    lv_label_set_text(label_tv,"photo tv");
    fclose(p);
    return;
}
void app2(lv_obj_t* canvas)
{
    lv_obj_t* obj=creat_ck(512,0,512,300,canvas);
    creat_close_button(obj);
    lv_obj_t* ta=creat_text(obj,250,0,"input photo path");
    lv_obj_add_event_cb(ta,show_raw_bmp,LV_EVENT_READY,obj);
}
void bmp_raw(int x,int y,int fd,int fd2,int date)
{
    struct bmp
    {
    uint16_t type;          
    uint32_t size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset;
    uint32_t size2;          
    int32_t  width;
    int32_t  height;        
    uint16_t planes;
    }c;
    lseek(fd,0,SEEK_SET);
    read(fd,&c,sizeof(c));
    if(c.type!= 0x4D42){jg("file in't bmp");close(fd);close(fd2);return;} 
    int row=(c.width*3+3)/4*4;
    int flag=0;
    if(c.height<0){c.height=-c.height;flag=1;}
    char (*p)[row]=malloc(row*c.height);//bmp格式没有透明度，所以*3
    if(p==NULL){jg("malloc error");return;}
    char (*p2)[x*4]=malloc(x*4*y);
    if(p2==NULL){jg("malloc error");return;}
    lseek(fd,c.offset,SEEK_SET);
    read(fd,p,row*c.height);
    for(int i=0;i<y;i++)
    {
    int m;
    if(flag==1) {m=i;} 
    else{m=y-1-i;}
        for(int j=0;j<x;j++)
        {
            p2[m][4*j]=p[c.height*i/y][3*c.width*j/x];
            p2[m][4*j+1]=p[c.height*i/y][3*c.width*j/x+1];
            p2[m][4*j+2]=p[c.height*i/y][3*c.width*j/x+2];
            p2[m][4*j+3]=date;
        }
    }
    write(fd2,p2,x*y*4);
    free(p);
    free(p2);
}
void ok_btn(lv_event_t *e)
{
    lv_obj_t* box=lv_event_get_user_data(e);
    lv_obj_del(box);jg("save success");
}
void cancel_btn(lv_event_t *e)
{
    lv_obj_t* box=lv_event_get_user_data(e);
    lv_obj_del(box);remove(save_path);jg("cancel success");
}
void creat_photo(lv_event_t *e)
{
    lv_obj_t* canvas=lv_event_get_user_data(e);
    const char* pos=lv_textarea_get_text(ta_pos);
    const char* size=lv_textarea_get_text(ta_size);
    const char* path_in=lv_textarea_get_text(ta_path_in);
    const char* path_out=lv_textarea_get_text(ta_path_out);
    strcpy(save_path,path_out);
    int x=0,y=0,w=0,l=0; 
    sscanf(pos,"%d %d",&x,&y);
    sscanf(size,"%d %d",&w,&l);
    if(x<0||y<0||w<=0||l<=0){jg("error date");return;} 
    if(x+w>1024||y+l>600){jg("size is't ture");return;}
    printf("%s\n%s\n",path_in,path_out);
    int fd=open(path_in,O_RDONLY);
    if(fd==-1){jg("input file isn't exit");return;}
    int fd2=open(path_out,O_RDONLY);
    if(fd2>0){jg("output file is exit");close(fd);close(fd2);return;}
    fd2=open(path_out,O_RDWR|O_CREAT,0755);
    if(fd==-1){jg("output path isn't exit");close(fd);return;}
    bmp_raw(w,l,fd,fd2,255);
    lv_obj_t* canvas2=creat_canvas(canvas,w,l,(char*)path_out,255);
    lv_obj_set_pos(canvas2,x,y);
    creat_close_button(canvas2);
    lv_obj_t *box = creat_ck(0, 0, 400, 200, lv_layer_top());
    lv_obj_center(box);
    lv_obj_move_foreground(box);
    lv_obj_set_style_bg_opa(box, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(box, lv_color_hex(0xFFFFFF), 0);
    lv_obj_t *label = create_label(box,100,0);
    lv_obj_set_size(label,200,50);
    lv_label_set_text(label,"are you want save it");
    lv_obj_t *btn_ok = creat_button(box,0,100);
    lv_obj_set_size(btn_ok,100,100);
    lv_obj_t *label_ok = create_label(btn_ok, 0, 0);
    lv_obj_center(label_ok);
    lv_label_set_text(label_ok, "OK");
    lv_obj_add_event_cb(btn_ok, ok_btn, LV_EVENT_CLICKED,box);
    lv_obj_t *btn_cancel = creat_button(box,300,100);
    lv_obj_set_size(btn_cancel,100,100);
    lv_obj_t *label_cancel = create_label(btn_cancel,0,0);
    lv_obj_center(label_cancel);
    lv_label_set_text(label_cancel, "Cancel");
    lv_obj_add_event_cb(btn_cancel, cancel_btn, LV_EVENT_CLICKED,box);
    close(fd);
    close(fd2);
}
void app3(lv_obj_t* canvas)
{
    lv_obj_t* obj=creat_ck(0,300,512,300,lv_screen_active());
    creat_close_button(obj);
    ta_pos=creat_text(obj,250,0,"input x y");
    ta_size=creat_text(obj,250,75,"input w l");
    ta_path_in=creat_text(obj,250,150,"bmp input path");
    ta_path_out=creat_text(obj,250,225,"raw output path");
    lv_obj_add_event_cb(ta_path_in,creat_photo, LV_EVENT_READY,canvas);
    lv_obj_add_event_cb(ta_pos,creat_photo, LV_EVENT_READY,canvas);
    lv_obj_add_event_cb(ta_size,creat_photo, LV_EVENT_READY,canvas);
    lv_obj_add_event_cb(ta_path_out,creat_photo, LV_EVENT_READY,canvas);
}
struct photo
    {
        lv_obj_t* s2;
        lv_obj_t* s3[4];
        lv_obj_t* s4;
        int move;
        char* photo;
        int x[4];//子弹坐标
        int y[4];
        char forword[4];//子弹朝向0待发射1左2右
        char rwforword;//1左2右
        char hp;
    }canva;
//玩家2
struct photo2 
{
    lv_obj_t* s3[4];      
    lv_obj_t* s4;         
    int move;            
    char* photo;          
    int x[4];   
    int y[4];          
    char forword[4];      
    char rwforword;
    char hp;
}canva2;
static char game_over;
void photo_mv(lv_timer_t *timer)
{
    if(rw_y<=410){rw_y=rw_y+20;}
    else{rw_y=410;}
    if(rw_x<10){rw_x=10;}
    if(rw_x>914){rw_x=914;}
    if(rw_x2<10){rw_x2=10;}
    if(rw_x2>914){rw_x2=914;}
    if(canva.move==1){lv_canvas_set_buffer(canva.s4,canva.photo,100,100, LV_COLOR_FORMAT_ARGB8888);rw_x-=10;}
    else if(canva.move==2){lv_canvas_set_buffer(canva.s4,canva.photo,100,100, LV_COLOR_FORMAT_ARGB8888);rw_x+=10;}
    else{lv_canvas_set_buffer(canva.s4,canva.photo,100,100, LV_COLOR_FORMAT_ARGB8888);}
    for(int i=0;i<4;i++)
    {
        if(canva.forword[i]==0)continue;
        if(canva.x[i]<=0||canva.x[i] >=974)
        {
            canva.forword[i]=0;
            canva.x[i]=50*i;
            lv_obj_set_pos(canva.s3[i],50*i,0);
        }
        else
        {
        if(canva.forword[i]==1)canva.x[i]-=20;
        else{canva.x[i]+=20;}  
        lv_obj_set_pos(canva.s3[i],canva.x[i],canva.y[i]);
            if(canva.x[i]<rw_x2+100&&canva.x[i]+50>rw_x2&&canva.y[i]<rw_y2+100&&canva.y[i]+80>rw_y2)
            {
                canva2.hp--;canva.forword[i]=0;canva.x[i]=50*i;
                lv_obj_set_pos(canva.s3[i],50*i,0);
                if(canva2.hp<=0){game_over=1;jg("P2 DEAD! P1 WIN!");}
            }
        }

    }
    lv_obj_set_pos(canva.s4,rw_x,rw_y);
    if(rw_y2<=410) {rw_y2+=20;}
    else {rw_y2=410;}
    if(canva2.move == 1){lv_canvas_set_buffer(canva2.s4,canva2.photo,100,100,LV_COLOR_FORMAT_ARGB8888);rw_x2-=10;}
    else if(canva2.move==2){lv_canvas_set_buffer(canva2.s4,canva2.photo,100,100, LV_COLOR_FORMAT_ARGB8888);rw_x2+=10;}
    else {lv_canvas_set_buffer(canva2.s4,canva2.photo,100,100,LV_COLOR_FORMAT_ARGB8888);}
    for(int i = 0; i < 4; i++) 
    {
        if(canva2.forword[i]==0) continue;
        if(canva2.x[i]<=0||canva2.x[i]>=974) 
        {
            canva2.forword[i]=0;
            canva2.x[i] =1024-50*(i+1);  
            lv_obj_set_pos(canva2.s3[i],canva2.x[i],0);
        }
        else 
        {
        if(canva2.forword[i]==1)canva2.x[i]-=20;
        else canva2.x[i]+=20;
        lv_obj_set_pos(canva2.s3[i], canva2.x[i],canva2.y[i]);
            if(canva2.x[i]<rw_x+100&&canva2.x[i]+50>rw_x&&canva2.y[i]<rw_y+100&&canva2.y[i]+80>rw_y)
            {
                canva.hp--;canva.forword[i]=0;canva2.x[i]=50*i;
                lv_obj_set_pos(canva2.s3[i],50*i,0);
                if(canva.hp<=0){game_over=1;jg("P1 DEAD! P2 WIN!");}
            }
        }
    }
    lv_obj_set_pos(canva2.s4,rw_x2,rw_y2);
}

void del_timer(lv_event_t *e)
{
    lv_timer_t *timer=lv_event_get_user_data(e);
    lv_timer_delete(timer);
    if(canva.photo!=NULL)
    {free(canva.photo);canva.photo=NULL;}
    if(canva2.photo!=NULL)
    {free(canva2.photo);canva2.photo=NULL;}
    if(app6_t.pid!=0){kill(app6_t.pid,9);app6_t.pid=0;}
}
void forword_control(lv_event_t *e)
{
    lv_obj_t *button = lv_event_get_target(e); 
    lv_obj_t *label=lv_obj_get_child(button, 0);
    const char *key = lv_label_get_text(label);
    int fd;
    if(strcmp(key, "A") == 0){canva.move=1;fd=open("4.raw",O_RDONLY);canva.rwforword=1;}
    else if(strcmp(key, "D") == 0){canva.move=2;fd=open("3.raw",O_RDONLY);canva.rwforword=2;}
    if(fd>0){read(fd,canva.photo,100*100*4);close(fd);}
}
void forword_control2(lv_event_t *e)
{
    lv_obj_t *button=lv_event_get_target(e);
    lv_obj_t *label=lv_obj_get_child(button,0);
    const char *key=lv_label_get_text(label);
    int fd=-1;
    if(strcmp(key,"A")==0){canva2.move=1;fd=open("6.raw",O_RDONLY);canva2.rwforword=1;}
    else if(strcmp(key,"D")==0){canva2.move=2;fd=open("5.raw",O_RDONLY);canva2.rwforword=2;}
    if(fd>0){read(fd,canva2.photo,100*100*4);close(fd);}
}
void click_control(lv_event_t *e)
{
    lv_obj_t *button = lv_event_get_target(e); 
    lv_obj_t *label=lv_obj_get_child(button, 0);
    const char *key = lv_label_get_text(label);
    if(rw_y<350){return;}
    if(strcmp(key, "W") == 0)rw_y=rw_y-200;
    if(strcmp(key, "S") == 0)
    {
        if(canva.rwforword==1)
        {
        if(canva.forword[0]==0){lv_obj_set_pos(canva.s3[0],rw_x-100,rw_y);canva.x[0]=rw_x-100;canva.y[0]=rw_y;canva.forword[0]=1;}
        else if(canva.forword[1]==0){lv_obj_set_pos(canva.s3[1],rw_x-100,rw_y);canva.x[1]=rw_x-100;canva.y[1]=rw_y;canva.forword[1]=1;}
        else if(canva.forword[2]==0){lv_obj_set_pos(canva.s3[2],rw_x-100,rw_y);canva.x[2]=rw_x-100;canva.y[2]=rw_y;canva.forword[2]=1;}
        else if(canva.forword[3]==0){lv_obj_set_pos(canva.s3[3],rw_x-100,rw_y);canva.x[3]=rw_x-100;canva.y[3]=rw_y;canva.forword[3]=1;}
        else {printf("玩家一子弹耗尽\n");return;}
        }
        else
        {
        if(canva.forword[0]==0){lv_obj_set_pos(canva.s3[0],rw_x+100,rw_y);canva.x[0]=rw_x+100;canva.y[0]=rw_y;canva.forword[0]=2;}
        else if(canva.forword[1]==0){lv_obj_set_pos(canva.s3[1],rw_x+100,rw_y);canva.x[1]=rw_x+100;canva.y[1]=rw_y;canva.forword[1]=2;}
        else if(canva.forword[2]==0){lv_obj_set_pos(canva.s3[2],rw_x+100,rw_y);canva.x[2]=rw_x+100;canva.y[2]=rw_y;canva.forword[2]=2;}
        else if(canva.forword[3]==0){lv_obj_set_pos(canva.s3[3],rw_x+100,rw_y);canva.x[3]=rw_x+100;canva.y[3]=rw_y;canva.forword[3]=2;}
        else {printf("玩家一子弹耗尽\n");return;}
        }
    }
}
void click_control2(lv_event_t *e)
{
    lv_obj_t *button=lv_event_get_target(e);
    lv_obj_t *label=lv_obj_get_child(button,0);
    const char *key=lv_label_get_text(label);
    if(rw_y2<350){return;}
    if(strcmp(key,"W")==0)rw_y2=rw_y2-200;
    if(strcmp(key,"S")==0)
    {
        if(canva2.rwforword==1)
        {
            if(canva2.forword[0]==0){lv_obj_set_pos(canva2.s3[0],rw_x2-100,rw_y2);canva2.x[0]=rw_x2-100;canva2.y[0]=rw_y2;canva2.forword[0]=1;}
            else if(canva2.forword[1]==0){lv_obj_set_pos(canva2.s3[1],rw_x2-100,rw_y2);canva2.x[1]=rw_x2-100;canva2.y[1]=rw_y2;canva2.forword[1]=1;}
            else if(canva2.forword[2]==0){lv_obj_set_pos(canva2.s3[2],rw_x2-100,rw_y2);canva2.x[2]=rw_x2-100;canva2.y[2]=rw_y2;canva2.forword[2]=1;}
            else if(canva2.forword[3]==0){lv_obj_set_pos(canva2.s3[3],rw_x2-100,rw_y2);canva2.x[3]=rw_x2-100;canva2.y[3]=rw_y2;canva2.forword[3]=1;}
            else{return;}
        }
        else
        {
            if(canva2.forword[0]==0){lv_obj_set_pos(canva2.s3[0],rw_x2+100,rw_y2);canva2.x[0]=rw_x2+100;canva2.y[0]=rw_y2;canva2.forword[0]=2;}
            else if(canva2.forword[1]==0){lv_obj_set_pos(canva2.s3[1],rw_x2+100,rw_y2);canva2.x[1]=rw_x2+100;canva2.y[0]=rw_y2;canva2.forword[1]=2;}
            else if(canva2.forword[2]==0){lv_obj_set_pos(canva2.s3[2],rw_x2+100,rw_y2);canva2.x[2]=rw_x2+100;canva2.y[0]=rw_y2;canva2.forword[2]=2;}
            else if(canva2.forword[3]==0){lv_obj_set_pos(canva2.s3[3],rw_x2+100,rw_y2);canva2.x[3]=rw_x2+100;canva2.y[0]=rw_y2;canva2.forword[3]=2;}
            else{return;}
        }
    }
}
void but_relase(lv_event_t *e)
{
    canva.move=0;
}
void but_relase2(lv_event_t *e)
{
    canva2.move=0;
}
void yx_init()
{
    lv_obj_t* obj=creat_ck(0,0,1024,600,lv_screen_active());
    creat_close_button(obj);
    canva.s2=creat_canvas(obj,1024,600,"2.raw",255);//背景初始化
    canva.s4=creat_canvas(canva.s2,100,100,"3.raw",255);//人物初始化3.raw向右，4.raw向左
    canva.rwforword=2;
    canva.hp=3;
    canva.photo=malloc(100*100*4);
    lv_obj_set_pos(canva.s4,10,430);
    rw_x=0,rw_y=430;
    rw_x2=924;          
    rw_y2=430; 
    canva.move=0;
    lv_timer_t *timer = lv_timer_create(photo_mv,80,NULL);
    lv_obj_add_event_cb(obj,del_timer,LV_EVENT_DELETE,timer);
    lv_obj_t* button[4],*label[4];
    button[0]=creat_button(canva.s2,100,100);
    button[1]=creat_button(canva.s2,0,200);
    button[2]=creat_button(canva.s2,100,200);
    button[3]=creat_button(canva.s2,200,200);
    for(int i=0;i<4;i++)
    {
        canva.s3[i]=creat_canvas(canva.s2,50,80,"1.raw",255);//子弹初始化,个数显示在左上角
        lv_obj_set_pos(canva.s3[i],50*i,0);
        canva.forword[i]=0;
        lv_obj_set_size(button[i],100,100);
        label[i]=create_label(button[i],0,0);
        lv_obj_center(label[i]);       
    }
    //玩家2初始化
    canva2.s4=creat_canvas(canva.s2,100,100,"5.raw",255);
    canva2.rwforword=1;
    canva2.hp=3;
    canva2.photo=malloc(100*100*4);
    int fd=open("6.raw",O_RDONLY);
    if(fd>0){read(fd,canva2.photo,100*100*4);close(fd);}
    lv_canvas_set_buffer(canva2.s4,canva2.photo,100,100,LV_COLOR_FORMAT_ARGB8888);
    lv_obj_set_pos(canva2.s4,914,430);
    rw_x2=924,rw_y2=430;
    canva2.move=0;
    for(int i=0;i<4;i++)
    {
        canva2.s3[i]=creat_canvas(canva.s2,50,80,"1.raw",255);
        lv_obj_set_pos(canva2.s3[i],1024-50*(i+1),0);
        canva2.forword[i]=0;
    }
    lv_label_set_text(label[0],"W");
    lv_label_set_text(label[1],"A");
    lv_label_set_text(label[2],"S");
    lv_label_set_text(label[3],"D");
    lv_obj_add_event_cb(button[0],click_control,LV_EVENT_PRESSED,NULL);
    lv_obj_add_event_cb(button[1],forword_control,LV_EVENT_PRESSED,NULL);
    lv_obj_add_event_cb(button[2],click_control,LV_EVENT_CLICKED,NULL);
    lv_obj_add_event_cb(button[3],forword_control,LV_EVENT_PRESSED,NULL); 
    lv_obj_add_event_cb(button[1],but_relase,LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(button[3],but_relase,LV_EVENT_RELEASED,NULL);
    //玩家2的按钮反馈
    lv_obj_t* button2[4],*label2[4];
    button2[0]=creat_button(canva.s2,824,100);
    button2[1]=creat_button(canva.s2,724,200);
    button2[2]=creat_button(canva.s2,824,200);
    button2[3]=creat_button(canva.s2,924,200);
    for(int i=0;i<4;i++)
    {
        lv_obj_set_size(button2[i],100,100);
        label2[i]=create_label(button2[i],0,0);
        lv_obj_center(label2[i]);
    }
    lv_label_set_text(label2[0],"W");
    lv_label_set_text(label2[1],"A");
    lv_label_set_text(label2[2],"S");
    lv_label_set_text(label2[3],"D");
    lv_obj_add_event_cb(button2[0],click_control2,LV_EVENT_PRESSED,NULL);
    lv_obj_add_event_cb(button2[1],forword_control2,LV_EVENT_PRESSED,NULL);
    lv_obj_add_event_cb(button2[2],click_control2,LV_EVENT_CLICKED,NULL);
    lv_obj_add_event_cb(button2[3],forword_control2,LV_EVENT_PRESSED,NULL);
    lv_obj_add_event_cb(button2[1],but_relase2,LV_EVENT_RELEASED,NULL);
    lv_obj_add_event_cb(button2[3],but_relase2,LV_EVENT_RELEASED,NULL);
}

void app4(lv_obj_t* canvas)
{
    yx_init();
}
void hide_mk(lv_timer_t *timer)
{
    app6_t.cnt=app6_t.cnt%5;
    if(app6_t.cnt==0)
    {
        for(int i=0;i<3;i++)
        {
            lv_obj_add_flag(app6_t.but[i],LV_OBJ_FLAG_HIDDEN);
        }
    }
    else
    {
        for(int i=0;i<3;i++)
        {
            lv_obj_clear_flag(app6_t.but[i],LV_OBJ_FLAG_HIDDEN);
        }
    }
    if(app6_t.cnt!=0)app6_t.cnt++;
}
void my_cnt(lv_event_t *e)
{
    app6_t.cnt=1;
}
void zt_studio(lv_event_t *e)
{
    if(app6_t.flag==0){kill(app6_t.pid,19);app6_t.flag=1;}
    else if(app6_t.flag==1){kill(app6_t.pid,18);app6_t.flag=0;}
}
void last_studio(lv_event_t *e)
{
    int fd = open("1.fifo", O_WRONLY | O_NONBLOCK);
    write(fd,"seek -15\n",9);
    close(fd);
}
void next_studio(lv_event_t *e)
{
    int fd = open("1.fifo", O_WRONLY | O_NONBLOCK);
    write(fd,"seek 15\n",8);
    close(fd);
}
void creat_app6mk()
{
    app6_t.obj=creat_ck(app6_t.x,app6_t.y,app6_t.sizex,app6_t.sizey,lv_screen_active());
    creat_close_button(app6_t.obj);
    lv_obj_t *lab[3];
    for(int i=0;i<3;i++)
    {
        app6_t.but[i]=creat_button(app6_t.obj,app6_t.x+app6_t.sizex-50,50*(i+1));
        lv_obj_set_size(app6_t.but[i],50,50);
        lab[i]=create_label(app6_t.but[i],0,0);
        lv_obj_center(lab[i]);
    }
    lv_label_set_text(lab[0],"<<");
    lv_label_set_text(lab[1],"||");
    lv_label_set_text(lab[2],">>");
    lv_timer_t *timer = lv_timer_create(hide_mk,1000,NULL);
    lv_obj_add_event_cb(app6_t.obj,my_cnt,LV_EVENT_CLICKED,NULL);
    lv_obj_add_event_cb(app6_t.obj,del_timer,LV_EVENT_DELETE,timer);
    lv_obj_add_event_cb(app6_t.but[0],last_studio,LV_EVENT_CLICKED,NULL);
    lv_obj_add_event_cb(app6_t.but[1],zt_studio,LV_EVENT_CLICKED,NULL);
    lv_obj_add_event_cb(app6_t.but[2],next_studio,LV_EVENT_CLICKED,NULL);
}
void show_studio(lv_event_t *e)
{
    if(app6_t.sum==0){return;}
    if(app6_t.pid!=0){kill(app6_t.pid,9);app6_t.pid=0;lv_obj_delete(app6_t.obj);}
    creat_app6mk();
    lv_obj_t* button_last=lv_event_get_target(e);
    lv_obj_t* label_last=lv_obj_get_child(button_last,0);
    lv_obj_t* ta=lv_event_get_user_data(e);
    int fd = open("/dev/input/event6", O_RDONLY|O_NONBLOCK);
    app6_t.path=lv_label_get_text(label_last);
    char a[20]={0},b[20]={0},c[20]={0};
    int x=app6_t.sizex-50;
    sprintf(a,"+%d+%d",app6_t.x,app6_t.y);
    sprintf(b,"%d",x);
    sprintf(c,"%d",app6_t.sizey);
    mkfifo("1.fifo",0666);
    app6_t.pid=fork();
    if(app6_t.pid==0)
    {
        execlp("mplayer64","mplayer64","-slave", "-quiet", "-input","file=1.fifo",
        "-vo", "fbdev2", "-ao", "alsa", "-geometry",a, "-zoom", "-x",b, "-y",c,app6_t.path, NULL);
        exit(0);
    }
}
lv_obj_t* obj_app6;
void show_file(lv_event_t *e)
{
    const char* c=lv_textarea_get_text(app6_t.ta2);
    sscanf(c,"%d %d",&app6_t.x,&app6_t.y);
    const char* b=lv_textarea_get_text(app6_t.ta3);
    sscanf(b,"%d %d",&app6_t.sizex,&app6_t.sizey);
    if(app6_t.x<0||app6_t.y<0||app6_t.x+app6_t.sizex+50>1024||app6_t.y+app6_t.sizey>600||app6_t.x+app6_t.sizex+50<=0||app6_t.y+app6_t.sizey<=0)
    {jg("date error");return;}
    static lv_obj_t *dir_list_box= NULL;  
    lv_obj_t* ta=lv_event_get_user_data(e);
    const char* path=lv_textarea_get_text(ta);
    if (dir_list_box == NULL) {dir_list_box = creat_ck(0,0,200,200,obj_app6);
    } else {lv_obj_clean(dir_list_box);}
    lv_obj_t* obj2=creat_ck(0,0,200,200,obj_app6);
    int i=0;
    struct dirent* a;
    char date[64]={0};
    char t[64]={0};
    DIR* dirp=opendir(path);
    if(dirp==NULL){jg("path error");return;}
    FILE * p=fopen("/exe.log/studio.txt","w+");
    while(a=readdir(dirp))
    {
        if(strcmp(a->d_name,".")==0||strcmp(a->d_name,"..")==0){continue;}
        if(!my_strcmp(a->d_name,".mp3") &&!my_strcmp(a->d_name,".mp4"))
        {
            continue;
        }
        i++; 
        app6_t.sum++;
        strcpy(date,path);
        strcat(date,"/");
        strcat(date,a->d_name);
        fprintf(p,"%s\n",date);   
    }
    lv_obj_t *button[i],*label[i];
    fseek(p,0,SEEK_SET);
    for(int j=0;j<i;j++)
    {
        button[j]=creat_button(obj2,0,50*j);
        lv_obj_set_size(button[j],200,50);
        label[j]=create_label(button[j],0,0);
        lv_obj_center(label[j]);
        fscanf(p,"%s",date);
        lv_label_set_text(label[j],date);
        lv_obj_add_event_cb(button[j],show_studio,LV_EVENT_CLICKED,NULL);
        strcpy(date,t);
    }
    fclose(p);
    return;
}
void app6(lv_obj_t* canvas)
{
    app6_t.cnt=0;
    app6_t.pid=0;
    app6_t.flag=0;
    app6_t.sum=0;
    app6_t.x=0;app6_t.y=0;app6_t.sizex=512;app6_t.sizey=300;//默认参数
    obj_app6=creat_ck(512,0,512,300,canvas);
    creat_close_button(obj_app6);
    lv_obj_t* ta=creat_text(obj_app6,250,100,"input studio path");
    lv_obj_add_event_cb(ta,show_file,LV_EVENT_READY,ta);
    app6_t.ta2=creat_text(obj_app6,250,150,"location:x y");
    app6_t.ta3=creat_text(obj_app6,250,200,"size:x y");
    lv_obj_add_event_cb(app6_t.ta2,show_file,LV_EVENT_READY,ta);
    lv_obj_add_event_cb(app6_t.ta3,show_file,LV_EVENT_READY,ta);
}
struct App7
{
    int sock_fd;
    lv_obj_t* obj;//总窗口
    lv_obj_t* obj2;//聊天窗口
    lv_obj_t* label;//显示问题和状态
    lv_obj_t* ta;//用户输入文本框
    int obj_ok;//有人发送
    int use_ok;//用户准备
    uint64_t file_size;//文件大小
    uint32_t id;//文件存放
    int cnt;//聊天布局
    int flag;//出错强制终止
    int fd;
    char* fd_mmp;
}App7;
int look_id()
{
    char a[64];
    while(1)
    {
    memset(a,0,64);
    sprintf(a,"save_file/%d",App7.id);
    FILE* p=fopen(a,"r");
    if(p==NULL){break;}
    App7.id++;
    fclose(p);
    }
    int fd=open(a,O_RDWR|O_CREAT|O_TRUNC,0644);//映射到虚拟内存，节省io开销
    ftruncate(fd,App7.file_size);
    App7.fd_mmp=mmap(NULL,App7.file_size,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
    if(App7.fd_mmp==MAP_FAILED){return -1;}
    return fd;
}
void check_path()
{
    DIR* p=opendir("save_file");
    if(p==NULL){mkdir("save_file",0755);}
    App7.fd=look_id();
    if(App7.fd==-1){App7.flag=0;return;}
}
void* file_revier(void* a)
{
    char b[2048];
    uint64_t fget=0;
    int flag=0;
    while(1)
    {
        struct pollfd fds[1];
        fds[0].fd=App7.sock_fd;
        fds[0].events=POLLIN;
        int res=poll(fds,1,10);
        if(res==0){jg("timeout,return file_stran");break;}
        if(res<0){jg("poll error");break;} 
        int n=read(App7.sock_fd,b,2048);
        if(n<=0){jg("sever error");break;}
        if((n+fget-8)==App7.file_size)
        {
            if(strcmp(b+strlen(b)-8,"^^^^^^^^")==0)
            strncpy(b,b,strlen(b)-8);
            flag=1;
            memcpy(App7.fd_mmp,b,n);fget+=n;
            break;
        }
        if(((n+fget-8)>App7.file_size))
        {
            break;
        }
        memcpy(App7.fd_mmp,b,n);fget+=n;
    }
    munmap(App7.fd_mmp,App7.file_size);App7.fd_mmp=NULL;
    close(App7.fd);App7.fd=-1;
    App7.use_ok=0;App7.obj_ok=0;
    if(flag==1){jg("receive success");}
    else{jg("receive fail");}
    return NULL;

}
void check_img(lv_timer_t *timer)
{
    if(App7.use_ok==1){return;}//用户传输直接返回，避免文件泄露到用户终端
    struct pollfd fds[1];
    fds[0].fd=App7.sock_fd;
    fds[0].events=POLLIN;
    pthread_t tid;
    int ret=poll(fds,1,0);      
    if(ret<=0){return;}           
    if(!(fds[0].revents&POLLIN))return;
    char a[256];
    int n=read(App7.sock_fd,a,sizeof(a));
    if (n<=0) 
    {
        lv_label_set_text(App7.label,"object error");
        return;
    }
    lv_label_set_text(App7.label,"con success");
    if(a[0]=='$'){App7.obj_ok=1;sscanf(a,"$ %*d %lu",&App7.file_size);}
    if(strstr(a,"~传输开始"))
    {
        if(App7.obj_ok!=1)return;
        App7.use_ok=1;
        pthread_create(&tid,NULL,file_revier,NULL);
        pthread_detach(tid);
        return;
    }
    App7.cnt++;
    lv_obj_t* label=create_label(App7.obj2,0,App7.cnt*50);
    lv_obj_set_size(label,512,50);
    lv_label_set_text(label,a);
}
void send_date(lv_event_t * e)
{
    const char* a=lv_textarea_get_text(App7.ta);
    write(App7.sock_fd, a, strlen(a));
    if(a[0]=='~'&&App7.obj_ok==1){check_path();}
    if(App7.flag==0){App7.flag=1;jg("check_path error");return;}
    App7.cnt++;
    lv_obj_t* label=create_label(App7.obj2,512,App7.cnt*50);
    lv_obj_set_size(label,512,50);
    lv_label_set_text(label,a);
    lv_textarea_set_text(App7.ta,"");
}
void con_server(lv_event_t * e)
{
    lv_obj_t* ta=lv_event_get_target(e);
    const char* b=lv_textarea_get_text(ta);
    char ip[16];char dk[5];
    if (sscanf(b,"%15[^_]_%4s",ip,dk)!=2){jg("input error");return;}
    printf("%s\n%s\n",ip,dk);
    App7.sock_fd=socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in a={0};
    a.sin_family=AF_INET;
    a.sin_port=htons(atoi(dk));
    inet_pton(AF_INET,ip,&a.sin_addr);
    if(connect(App7.sock_fd,(struct sockaddr*)&a,16)<0){jg("con fail");perror("connect");return;}
    App7.obj=creat_ck(0,50,1024,550,lv_screen_active());
    App7.obj2=creat_ck(0,0,1024,400,App7.obj);
    App7.label=create_label(App7.obj,0,400);
    lv_obj_set_size(App7.label,200,50);
    App7.ta=creat_text(App7.obj,200,400,"input");
    lv_obj_set_size(App7.ta,200,50);
    lv_obj_add_event_cb(App7.ta,send_date,LV_EVENT_READY,NULL);
    creat_close_button(App7.obj);
    lv_timer_t *timer=lv_timer_create(check_img,100,NULL);
    lv_obj_add_event_cb(App7.obj,del_timer,LV_EVENT_DELETE,timer);
}
void app7(lv_obj_t* canvas)
{
    App7.sock_fd=-1;
    App7.cnt=0;
    App7.obj_ok=0;
    App7.flag=1;
    lv_obj_t* obj=creat_ck(0,0,512,300,canvas);
    creat_close_button(obj);
    lv_obj_t* ta=creat_text(obj,250,100,"input ip_dk");
    lv_obj_add_event_cb(ta,con_server,LV_EVENT_READY,obj);

}
void app_mode(lv_event_t * e)
{
    lv_obj_t* canvas = lv_event_get_user_data(e);
    lv_obj_t* button = lv_event_get_target(e);
    lv_obj_t* label = lv_obj_get_child(button,0);
    char* text = lv_label_get_text(label);
    int mode;
    sscanf(text,"%d",&mode);
    switch (mode)
    {
    case 1:
        app1(canvas);
        break;
    case 2:
        app2(canvas);
        break;
    case 3:
        app3(canvas);
        break;
    case 4:
        app4(canvas);
        break;
    case 5:
        app5(canvas);
        break;
    case 6:
        app6(canvas);
        break;
    case 7:
        app7(canvas);
        break;
    default:
        jg("this isn't open");
        break;
    } 
}
void control_app(lv_event_t * e)
{
    lv_obj_t* ta_pass=lv_event_get_target(e);
    lv_obj_t* obj=lv_obj_get_parent(ta_pass);
    lv_obj_t* ta_name=lv_event_get_user_data(e);
    const char* pass= lv_textarea_get_text(ta_pass);    
    const char* name= lv_textarea_get_text(ta_name);
    lv_obj_t *button[N],*label[N];
    if(pass_flag==-1)//用户信息加载
    {
        printf("1\n");
        mkdir("/exe.log",0755);
        FILE* p=fopen("/exe.log/pass.log","w+");
        strcpy(use_date.name,name);strcpy(use_date.pass,pass);
        fprintf(p,"%s\n",name);
        fprintf(p,"%s\n",pass);
        fclose(p);
    }
    if(pass_flag==0)
    {
        FILE* p=fopen("/exe.log/pass.log","r");
        fscanf(p,"%s\n",use_date.name);
        fscanf(p,"%s\n",use_date.pass);
        fscanf(p,"%s\n",use_date.photo);
        if(strcmp(use_date.name,(char*)name)!=0||strcmp(use_date.pass,(char*)pass)!=0){jg("pass or name error");fclose(p);return;}
    }
    strcpy(use_date.name,name);strcpy(use_date.pass,pass);
    lv_obj_del(ta_name);
    lv_obj_del(ta_pass);
    lv_obj_t* canvas=creat_canvas(obj,1024,600,use_date.photo,255);
    int fd=open("/my_photo/1.raw",O_RDONLY);
    if (fd<0) 
    {
        jg("photo load fail");
    }
    lv_obj_center(canvas);
    for(int i=0;i<N;i++)
    {
    int a=i/4,b=i%4;//构建框架每行4个app;
    button[i]=creat_button(canvas,100+b*(824/4),300*a);
    lv_obj_set_size(button[i],206,200);
    label[i]=lv_label_create(button[i]);
    lv_label_set_text_fmt(label[i],"%d",i+1);
    lv_obj_center(label[i]);
    lv_obj_add_event_cb(button[i],app_mode,LV_EVENT_CLICKED,canvas);
    } 
    lv_label_set_text(label[0],"1 File Manager");
    lv_label_set_text(label[1],"2 Photo Viewer");
    lv_label_set_text(label[2],"3 BMP to RAW Converter");
    lv_label_set_text(label[3],"4 Game");
    lv_label_set_text(label[4],"5 GPIO Control");
    lv_label_set_text(label[5],"6 Studio");
    lv_label_set_text(label[6],"7 chat");
}
void app()
{
    lv_obj_t *scr=lv_screen_active(),*obj,*obj2,*img,*ta_pass,*label,*ta_name;
    obj=creat_ck(0,0,1024,600,scr);
    int fd=open("/exe.log/pass.log",O_RDONLY);
    char name[128]={0},pass[128]={0},pass2[128]={0};
    pass_flag=0;
    lv_obj_move_background(obj);
    if(fd==-1)
    {
        pass_flag=-1;
        ta_name=creat_text(obj,312,100,"first meet,input name");
        lv_obj_set_size(ta_name,400,100);
        ta_pass=creat_text(obj,312,400,"first meet,input pass");
        lv_obj_set_size(ta_pass,400,100);
        lv_obj_add_event_cb(ta_pass,control_app, LV_EVENT_READY,ta_name);
        close(fd);
        return;
    }
    pass_flag=0;
    ta_name=creat_text(obj,312,100,"input name");
    lv_obj_set_size(ta_name,400,100);
    ta_pass=creat_text(obj,312,400,"input pass");
    lv_obj_set_size(ta_pass,400,100);
    lv_obj_add_event_cb(ta_pass,control_app, LV_EVENT_READY,ta_name);
    close(fd);
}