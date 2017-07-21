/*****************************************************************************/
/*                                                                           */
/* File: FastCGI interface for Percolibrium REST API echo.c                  */
/*                                                                           */
/* Created: Thu Sep  8 11:33:28 2016                                         */
/*                                                                           */
/*****************************************************************************/

// gcc -o echo.fcgi RESTAPI-echo.c -lfcgi; chmod 755 echo.fcgi

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fastcgi/fcgi_stdio.h>
#include <fastcgi/fcgimisc.h>
#include <fastcgi/fastcgi.h>
#include <fastcgi/fcgios.h>
#include <fastcgi/fcgiapp.h>

#define true 1
#define false 0
#define CGN_BUFSIZE 1024

/*

The configuration for fastcgi in lighttpd: (don't put sockets in /tmp if there are login users)

/etc/lighttpd/conf.d/fastcgi.conf

fastcgi.debug = 1

fastcgi.server = (
    "/cgn" => (
               "echo.fcgi.handler" => (
                                      "socket" => "/tmp/echo.fcgi.socket",
                                      "check-local" => "disable",
                                      "bin-path" => "/path/echo.fcgi",
                                      "max-procs" => 1
                                      )
               )
    )


 */

/********************************************************************************/

static void PrintEnv(char *label, char **envp)
{
    printf("%s:\n", label);

    for ( ; *envp != NULL; envp++)
       {
       printf("%s\n", *envp);
       }

    printf("\n");
}

/********************************************************************************/

void Convert(char *s)
{
 char *sp;
 
 for (sp = s; *sp != '\0'; sp++)
    {
    if (*sp == '+')
       {
       *sp = ' ';
       }
    }
}
/********************************************************************************/

static void DecodeCGITextQuery(char *query, char *context)
{
 FILE *pp;
 char cmd[4000];
 snprintf(cmd,2000,"/var/CGNgine/bin/stories -s \"%s\" -c \"%s\"", query, context);
 
 if ((pp = popen(cmd,"r")) != NULL)
    {
    printf("EXEC stories  %s\n", cmd);
    cmd[0] = '\0';
    
    while (!feof(pp))
       {
       fgets(cmd,256,pp);
       printf("%s\n",cmd);
       }
    
    pclose(pp);
    }
}

/********************************************************************************/

static void DecodeCGIJsonQuery(char *query, char *context)
{

}

/********************************************************************************/

static void DecodeCGIPost(char *addr)
{
 char ch;
 char hex[3],buffer[CGN_BUFSIZE];
 char from[CGN_BUFSIZE],to[CGN_BUFSIZE],fwd[CGN_BUFSIZE],bwd[CGN_BUFSIZE],context[CGN_BUFSIZE];
 int i = 0,j,type ,inside_tuple = false;
 double weight;
 int index = 0;
 FILE *fout, *pp;
 inside_tuple = false;
 char filename[256];

 char tuple[6][CGN_BUFSIZE];
 
          /* CGI encoding
             message= prefix
             %2C comma ,
             %28 (
             %29 )
             +  space
             %OD%OA newline \n\r
          */

memset(buffer,0,CGN_BUFSIZE);
snprintf(filename,256,"/tmp/post_%s",addr);

printf("OPENED...\n");


if ((fout=fopen(filename,"w")) == NULL)
   {
   printf("Cannot write POST data");
   return;
   }

 while ((ch = fgetc(stdin)) != EOF)
    {
    switch(ch)
       {
       case '%':
           hex[0] = fgetc(stdin);
           hex[1] = fgetc(stdin);
           hex[2] = '\0';

           if (strcmp(hex,"28") == 0)
              {
              // (
              inside_tuple = true;
              i = -1;
              buffer[0] = '\0';
              }
           else if (strcmp(hex,"2C") == 0 || strcmp(hex,"29") == 0)
              {
              // comma or )  end of item
              buffer[i] = '\0';
              strncpy(tuple[index],buffer,CGN_BUFSIZE);

              if (++index == 6)
                 {
                 //for (index = 0; index < 6; index++)
                 //   {
                 //   printf("%d. %s\n",index,tuple[index]);
                 //   }
                 //printf("\n");
                 fprintf(fout,"(%s,%s,%s,%s,%s,%s)\n",tuple[0],tuple[1],tuple[2],tuple[3],tuple[4],tuple[5],tuple[6]);
                 index = 0;
                 }
              
              if (strcmp(hex,"29") == 0)
                 {
                 // )
                 inside_tuple = false;
                 }

              buffer[0] = '\0';
              i = -1;
              } 
           break;
           
       case '+':
           buffer[i] = ' ';
           break;
           
       default:
           if (inside_tuple)
              {
              buffer[i] = ch;
              }
           break;
       }

    if (i >= CGN_BUFSIZE)
       {
       break;
       }
    
    if (inside_tuple)
       {
       i++;
       }
    }

 fclose(fout);

 // Now write the temporary file to the CGN data

 char cmd[4000];
 snprintf(cmd,2000,"/var/CGNgine/bin/conceptualize %s %s", filename, filename); // bug in args??
 
 if ((pp = popen(cmd,"r")) != NULL)
    {
    printf("EXEC %s\n", cmd);
    cmd[0] = '\0';
    
    while (!feof(pp))
       {
       fgets(cmd,256,pp);
       printf("out %s\n",cmd);
       }
    
    pclose(pp);
    }
}

/********************************************************************************/

int main(void)
{
 int count = 0, i;
 char *v;
 FILE *pp;
 extern char **environ;

 while (FCGI_Accept() >= 0)
    {
    printf("Content-type: text/plain\r\n\r\n");
    PrintEnv("Request environment", environ); // DEBUG

    if (strcmp(getenv("REQUEST_METHOD"),"POST") == 0)
       {
       if (v = getenv("REQUEST_URI"))
          {
          if (strcmp(v,"/cgn/associations") != 0)
             {
             printf("Unknown POST operation");
             return 1;
             }

          printf("Content-Type: text/html\r\n\r\n");
          DecodeCGIPost(getenv("REMOTE_ADDR"));
          }
       
       return 0;
       }

    if (strcmp(getenv("REQUEST_METHOD"),"GET") == 0)
       {
       char *sp,*qs,query[256] = {0},context[256] = {0};
       
       qs = getenv("QUERY_STRING");

       for (sp = qs; *sp != '\0'; sp++)
          {
          if (strncmp(sp,"query=",5) == 0)
             {
             sscanf(sp+6,"%255[^&]",query);
             sp += 6;
             Convert(query);
             }
          
          if (strncmp(sp,"context=",7) == 0)
             {
             sscanf(sp+8,"%255[^&]",context);
             sp += 8;
             Convert(context);
             }
          }

       v = getenv("REQUEST_URI");
       
       // story in text or JSON  /cgn/stories/text
       if (strncmp(v,"/cgn/stories/json",strlen("/cgn/stories/json")) == 0)
          {
          printf("Content-type: application/json\r\n\r\n");
          DecodeCGIJsonQuery(query,context);
          return 0;
          }

       if (strncmp(v,"/cgn/stories/text",strlen("/cgn/stories/text")) == 0)
          {
          printf("Content-type: text/plain\r\n\r\n");
          DecodeCGITextQuery(query,context);
          return 0;
          }

       printf("Unknown GET operation");
       return 1;
       }
    
    printf("{ cgn_error : \"Not found ... why?\"; }\n");
    }
 return 0;
}

