/*****************************************************************************/
/*                                                                           */
/* File: FastCGI interface for REST API CGNgine echo.c                       */
/*                                                                           */
/* Created: Thu Sep  8 11:33:28 2016                                         */
/*                                                                           */
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <fastcgi/fcgi_stdio.h>

// gcc -o echo.fcgi echo.c -lfcgi

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

int main(void)
{
 int count = 0, i;
 char *v;
 FILE *pp;

 while (FCGI_Accept() >= 0)
    {
    if (v = getenv("REQUEST_URI"))
       {
       char cmd[2000];
       
       // This program must be made setuid root in order to open the db, for Byzantine reasons
       
       snprintf(cmd,2000,"/var/CGNgine/bin/cgn-report -u %s -d /var/CGNgine", v);
       
       if ((pp = popen(cmd,"r")) != NULL)
          {
          char line[4000];
          
          while (!feof(pp))
             {
             fgets(line,4000,pp);
             printf("%s\n",line);
             }
          
          pclose(pp);
          }
       }
    else
       {
       printf("{ cgn_error : \"Not found\"; }\n");
       }
    }
 return 0;
}


