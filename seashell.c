#define _GNU_SOURCE
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAX 64
#define MAXLINE 512

extern char **environ;

/* tiny libc replacements */

int strlen_n(const char *s){int i=0;while(s[i])i++;return i;}

int strcmp_n(const char *a,const char *b){
    while(*a && *b && *a==*b){a++;b++;}
    return *a-*b;
}

int strncmp_n(const char *a,const char *b,int n){
    for(int i=0;i<n;i++){
        if(a[i]!=b[i]||a[i]==0||b[i]==0)
            return a[i]-b[i];
    }
    return 0;
}

char *strchr_n(const char *s,int c){
    while(*s){
        if(*s==c) return (char*)s;
        s++;
    }
    return 0;
}

/* command structure */

struct cmd{
    char *argv[MAX];
    char *in;
    char *out;
    int app;
    int bg;
};

/* tokenizer */

char *stok(char *s,char **save){
    char *p=s?s:*save;
    if(!p) return 0;

    while(*p==' '||*p=='\t'||*p=='\n') p++;
    if(!*p) return 0;

    char *start=p;

    while(*p && *p!=' ' && *p!='\t' && *p!='\n') p++;

    if(*p){
        *p=0;
        *save=p+1;
    }else{
        *save=0;
    }

    return start;
}

/* parse command */

void parse(char *l,struct cmd *c){

    char *s=0;
    int i=0;

    c->in=0;
    c->out=0;
    c->app=0;
    c->bg=0;

    char *t=stok(l,&s);

    while(t){

        if(strcmp_n(t,"<")==0){
            t=stok(0,&s);
            if(t) c->in=t;
        }

        else if(strcmp_n(t,">")==0){
            t=stok(0,&s);
            if(t){ c->out=t; c->app=0; }
        }

        else if(strcmp_n(t,">>")==0){
            t=stok(0,&s);
            if(t){ c->out=t; c->app=1; }
        }

        else if(strcmp_n(t,"&")==0){
            c->bg=1;
        }

        else{
            c->argv[i++]=t;
        }

        t=stok(0,&s);
    }

    c->argv[i]=0;
}

/* split pipeline */

int split(char *l,char *a[]){

    int n=0;
    a[n]=l;

    while(*l){
        if(*l=='|'){
            *l=0;
            l++;
            a[++n]=l;
        }else{
            l++;
        }
    }

    return n+1;
}

/* exec search PATH */

void exec_path(char *c,char **argv){

    if(strchr_n(c,'/'))
        execve(c,argv,environ);

    char *p=0;

    for(char **e=environ;*e;e++)
        if(!strncmp_n(*e,"PATH=",5)){
            p=*e+5;
            break;
        }

    if(!p) _exit(1);

    char b[512];
    int i=0,j;

    while(1){

        j=0;

        while(p[i] && p[i]!=':')
            b[j++]=p[i++];

        if(p[i]==':') i++;

        b[j++]='/';

        int k=0;
        while(c[k])
            b[j++]=c[k++];

        b[j]=0;

        execve(b,argv,environ);

        if(!p[i]) break;
    }

    _exit(1);
}

/* pipeline executor */

void pipe_exec(struct cmd c[],int n){

    int pipes[MAX][2];

    for(int i=0;i<n-1;i++)
        if(pipe(pipes[i])<0)
            _exit(1);

    for(int i=0;i<n;i++){

        pid_t pid=fork();

        if(pid<0)
            _exit(1);

        if(pid==0){

            signal(SIGINT,SIG_DFL);

            if(i>0)
                dup2(pipes[i-1][0],0);

            if(i<n-1)
                dup2(pipes[i][1],1);

            for(int j=0;j<n-1;j++){
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            if(c[i].in){
                int fd=open(c[i].in,O_RDONLY);
                if(fd<0) _exit(1);
                dup2(fd,0);
                close(fd);
            }

            if(c[i].out){
                int fd=open(
                    c[i].out,
                    O_WRONLY|O_CREAT|
                    (c[i].app?O_APPEND:O_TRUNC),
                    0644
                );

                if(fd<0) _exit(1);

                dup2(fd,1);
                close(fd);
            }

            exec_path(c[i].argv[0],c[i].argv);

            _exit(1);
        }
    }

    for(int i=0;i<n-1;i++){
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    for(int i=0;i<n;i++)
        if(!c[n-1].bg)
            wait(0);
}

/* prompt */

char *prompt(){

    static char p[MAXLINE];

    char cwd[128];
    char h[64];

    char *u="user";
    char *home=0;

    int i,j;

    for(char **e=environ;*e;e++){
        if(!strncmp_n(*e,"USER=",5)) u=*e+5;
        if(!strncmp_n(*e,"HOME=",5)) home=*e+5;
    }

    if(getcwd(cwd,sizeof(cwd))<0){
        cwd[0]='?';
        cwd[1]=0;
    }

    if(gethostname(h,sizeof(h))<0){
        h[0]='h';
        h[1]=0;
    }

    i=j=0;

    while(u[i]) p[j++]=u[i++];

    p[j++]='@';

    i=0;
    while(h[i]) p[j++]=h[i++];

    p[j++]=':';

    i=0;

    if(home){
        int k=0;

        while(home[k] && cwd[k]==home[k])
            k++;

        if(!home[k]){
            p[j++]='~';
            i=k;
        }
    }

    while(cwd[i])
        p[j++]=cwd[i++];

    p[j++]=' ';

    p[j++]=(getuid()==0?'#':'$');

    p[j++]=' ';

    p[j]=0;

    return p;
}

/* main loop */

int main(){

    char l[MAXLINE];
    char *c[MAX];

    struct cmd cm[MAX];

    signal(SIGINT,SIG_IGN);

    while(1){

        char *p = prompt();
        write(1,p,strlen_n(p));

        int len=0;
        char ch;

        while(len<MAXLINE-1){

            if(read(0,&ch,1)<=0)
                _exit(0);

            if(ch=='\n'){
                l[len]=0;
                break;
            }

            l[len++]=ch;
        }

        l[len]=0;

        if(len==0) continue;

        if(strcmp_n(l,"exit")==0)
            break;

        if(!strncmp_n(l,"cd ",3)){
            chdir(l+3);
            continue;
        }

        int n=split(l,c);

        for(int i=0;i<n;i++)
            parse(c[i],&cm[i]);

        pipe_exec(cm,n);
    }

    return 0;
}