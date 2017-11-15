/*****************************************************************************/
/*                                                                           */
/* File: get_k8s_context                                                     */
/*                                                                           */
/*****************************************************************************/

// gcc -g get_k8s_context.c -o get_k8s_context

#define _XOPEN_SOURCE 
#define _BSD_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <regex.h>
#include <stdlib.h>
#include <sys/stat.h>

#define true 1
#define false 0
#define CGN_BUFSIZE 1024
#define INSIGNIFICANT 10
#define HIGH +1
#define LOW -1
#define HASHTABLESIZE 8197
#define DEBUG 0
#define Debug if (DEBUG) printf

#define KBCTL "/usr/local/bin/kubectl"

/*****************************************************************************/
/* Add Item list Library from CFEngine */

#include "item.h"
#include "item.c"

void GetPods(void);
void DescribePods(void);
char *SplitPods(char *podref, char *podid, char *replicaset);

/*****************************************************************************/

typedef struct
{
   char *namespace;
   char *pod;
   char *container;
   char *appsysname;
}
    ClusterLocation;

/*****************************************************************************/
/* BEGIN                                                                     */
/*****************************************************************************/

void main(int argc, char** argv)
{
 printf("Getting kubernetes context\n");
 GetPods();
 DescribePods();
}

/*****************************************************************************/

void GetPods(void)

{ FILE *pp;
 char cmd[CGN_BUFSIZE];
 char line[CGN_BUFSIZE];
 char namespace[256],replicaset[256],podref[256],podid[256];
 int restarts;
 
 //snprintf(cmd,CGN_BUFSIZE,"%s get pods --all-namespaces",KBCTL);
 snprintf(cmd,CGN_BUFSIZE, "/bin/cat /tmp/test_get_k8s #");

 /* FORMAT
NAMESPACE      NAME                                          READY     STATUS    RESTARTS   AGE
book-info      details-v1-6fc98d6c9-jchj5                    2/2       Running   0          22d
book-info      productpage-v1-6fc697bfcf-4n5rp               2/2       Running   0          22d
default        hello-world-85f98cf8c9-99wgv                  1/1       Running   0          42d
default        helloworld-v1-865b58b45c-ldrrc                2/2       Running   0          39d
elk-logging    crawler-jslrc                                 1/1       Running   0          14d
 */
 
 if ((pp = popen(cmd,"r")) == NULL)
    {
    printf("Couldn't execute %s\n",cmd);
    return;
    }

 fgets(line,CGN_BUFSIZE,pp); // Skip header
     
 while(!feof(pp))
    {
    line[0] = namespace[0] = replicaset[0] = '\0';
    fgets(line,CGN_BUFSIZE,pp);

    if (!isspace(*line))
       {
       sscanf(line,"%s %s %*s %*s %d %*s",namespace,podref,&restarts);

       char *pod = SplitPods(podref,podid,replicaset);

       if (strlen(namespace) > 0)
          {
          printf("\"kubernetes pod %s\" is part of replicaset %s deployment \"%s\" in  \"kubernetes namespace %s\"\n",podid,replicaset,podref,namespace);
          }
       }
    }

 pclose(pp);
}

/*****************************************************************************/

void DescribePods(void)

{ FILE *pp;
 char cmd[CGN_BUFSIZE];
 char line[CGN_BUFSIZE];
 char namespace[256],replicaset[256],ipaddress[128],podref[128],podid[128];
 char port[128],hostnode[128];
 char *container;
 int mounts = false, env = false;

 printf("Look for dependencies...\n");
 
 //snprintf(cmd,CGN_BUFSIZE,"%s describe pods --all-namespaces",KBCTL);
 snprintf(cmd,CGN_BUFSIZE, "/bin/cat /tmp/test_get_k8s2 #");

 // Look for resource dependencies now. Don't try to parse everything,
 // as this is not helpful - mine opportunistically.
 
 if ((pp = popen(cmd,"r")) == NULL)
    {
    printf("Unable to execute %s\n",cmd);
    return;
    }

 while(!feof(pp))
    {
    line[0] = '\0';
    fgets(line,CGN_BUFSIZE,pp);

    if (strchr(line,'(') == NULL) // reset flags if not data line
       {
       mounts = false;
       env = false;
       }

    if (strncmp(line,"Name:",5) == 0) // New container
       {
       hostnode[0] = port[0] = podid[0] = replicaset[0] = '\0';
       
       sscanf(line+5,"%s",podref);
       container = SplitPods(podref,podid,replicaset);       
       }

    // ********
    
    if (strncmp(line,"Namespace:",10) == 0) // New container
       {
       sscanf(line+10,"%s",namespace);
       
       if (strlen(namespace) > 0)
          {
          printf("\"kubernetes pod %s\" is part of replicaset %s deployment \"%s\" in  \"kubernetes namespace %s\"\n",podid,replicaset,podref,namespace);
          }
       }

    // ********
    
    if (strncmp(line,"IP: ",4) == 0) // localization
       {
       sscanf(line+4,"%s",ipaddress);
       printf("pod %s/%s has service at cluster IP address %s\n",podref,podid,ipaddress);
       }

    // ********
    
    if (strncmp(line,"Node: ",5) == 0) // localization
       {
       sscanf(line+5,"%s",hostnode);
       printf("pod %s/%s is hosted at NODE %s\n",podref,podid,hostnode);
       }

    // ********
    
    if (strncmp(line,"    Port:",9) == 0) // localization
       {
       sscanf(line+9,"%s",port);
       printf("pod %s/%s is at cluster service IP address %s and port %s\n",podref,podid,ipaddress,port);
       }

    //  Mounts:
    //  /etc/certs/ from istio-certs (ro)
    //  /etc/istio/proxy from istio-envoy (rw)
    //  /var/run/secrets/kubernetes.io/serviceaccount from default-token-bpngl (ro)

    if (mounts)
       {
       char vol[1024],host[1024],mode[16];
       vol[0] = host[0] = mode[0] = '\0';
       sscanf(line,"%s from %s %s",vol,host,mode);
       printf("pod %s/%s containers depend on external volume %s:%s mode %s \n",podref,podid,host,vol,mode);
       }
    
    if (strncmp(line,"    Mounts:",11) == 0) // localization
       {
       mounts = true;
       }

    //    Environment:
    //  POD_NAME:       helloworld-v1-865b58b45c-ldrrc (v1:metadata.name)
    //  POD_NAMESPACE:  default (v1:metadata.namespace)
    //  INSTANCE_IP:     (v1:status.podIP)

    if (env)
       {
       char lval[1024],rval[1024];
       lval[0] = rval[0] = '\0';
       sscanf(line,"%s %s",lval,rval);
       printf("pod %s/%s containers depend on environment %s=%s \n",podref,podid,lval,rval);
       }

    if (strncmp(line,"    Environment:",16) == 0) // localization
       {
       env = true;
       }
    }

 pclose(pp);
}

/*****************************************************************************/
/* Level 1                                                                   */
/*****************************************************************************/

char *SplitPods(char *podref, char *podid, char *replicaset)

{ char *sp;
  char *first_guess,*splitter;
  int ref = false;

 for (sp = podref+strlen(podref); *sp != '-'; sp--)
    {
    }
 
 first_guess = sp;
 strcpy(podid,first_guess+1);
 *first_guess++ = '\0';

 if (strcmp(first_guess,"master") == 0)
    {
    *sp = '\0';
    return sp+1;
    }
 
 sp--;

 while ((*sp != '-') && (sp > podref))
    {
    if (isdigit(*sp))
       {
       ref = true;
       }
    sp--;
    }

 if (ref)
    {
    *sp = '\0';
    splitter = sp+1;
    strcpy(replicaset,splitter);
    }
 else
    {
    splitter = first_guess;
    }

 return splitter; // deployment
}
