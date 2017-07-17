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
    printf("%s:<br>\n<pre>\n", label);
    for ( ; *envp != NULL; envp++) {
        printf("%s\n", *envp);
    }
    printf("</pre><p>\n");
}

/********************************************************************************/

static void DecodeCGI(char *addr)
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
                 //   printf("%d. %s<br>\n",index,tuple[index]);
                 //   }
                 //printf("<br>");
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
 snprintf(cmd,2000,"/var/CGNgine/bin/conceptualize %s", filename);
 
 if ((pp = popen(cmd,"r")) != NULL)
    {
    while (!feof(pp))
       {
       fgets(cmd,256,pp);
       printf("%s\n",cmd);
       }
    
    pclose(pp);
    }
}

/********************************************************************************/

int main(void)
{
 int count = 0, i;
 char *v,*m;
 FILE *pp;
 extern char **environ;

 while (FCGI_Accept() >= 0)
    {
    printf("Content-Type: text/html\r\n\r\n");

    // PrintEnv("Request environment", environ); // DEBUG

    m = getenv("REQUEST_METHOD");
    
    if (strcmp(m,"POST") != 0)
       {
       printf("{ cgn_error : \"No data posted\"; }\n");
       return 0;
       }
    else
       {
       if (v = getenv("REQUEST_URI"))
          {
          if (strcmp(v,"/cgn/associations") != 0)
             {
             printf("Unknown operation");
             return 1;
             }
          
          DecodeCGI(getenv("REMOTE_ADDR"));
          }
       
       return 0;
       }

    if (strcmp(m,"GET") != 0)
       {
       // story in text or JSON  /cgn/stories/text
       if (v = getenv("REQUEST_URI"))
          {
          if (strcmp(v,"/cgn/stories/text") == 0)
             {
             return 0;
             }
          }
       
       if (v = getenv("REQUEST_URI"))
          {
          if (strcmp(v,"/cgn/stories/text") == 0)
             {
             return 0;
             }
          }

       printf("Unknown operation");
       return 1;
       }
    
    printf("{ cgn_error : \"Not found ... why?\"; }\n");
    }
 return 0;
}

