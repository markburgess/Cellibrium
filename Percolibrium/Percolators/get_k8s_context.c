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
#define INSIGNIFICANT 10
#define HIGH +1
#define LOW -1
#define HASHTABLESIZE 8197
#define DEBUG 0
#define Debug if (DEBUG) printf

#define KBCTL "/usr/local/bin/kubectl"

/*****************************************************************************/
/* Add Item list Library from CFEngine */
/*****************************************************************************/

#include "item.h"
#include "item.c"

#include "../../RobIoTs/CGNgine/libpromises/graph.h"
#include "../../RobIoTs/CGNgine/libpromises/graph_defs.c"

/*****************************************************************************/
// Prototypes
/*****************************************************************************/

void InitialConcepts(void);
void GetPods(void);
void DescribePods(void);
char *SplitPods(char *podref, char *podid, char *replicaset);
void PodGraph(char *namespace,char *deploy,char *replicaset,char *podid,char *context);
void ClusterIPServiceGraph(char *deploy,char *podid,char *ipaddress, char *port, char*context);
void HostNodeGraph(char *deploy,char *podid,char *hostnode,char *context);
void MountVolumeGraph(char *container,char *host,char *vol,char *mode);
void EnvironmentGraph(char *container,char *lval,char *rval);
void ContainerImageGraph(char *deploy,char *podid,char *container,char *image);

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
 InitialConcepts();

 GetPods();       // Need this to couple namespaces
 DescribePods();
}

/*****************************************************************************/

void InitialConcepts(void)

{
 char *context = ContextGr(stdout,"Kubernetes microservices cluster deployemnt");
 

 RoleGr(stdout,"kubernetes deployment","software deployment", "kubernetes",context);
 Gr(stdout, "software deployment",a_alias,"microservice deployment",context);

 Gr(stdout, "kubernetes namespace",a_contains,"kubernetes deployment",context);
 Gr(stdout, "kubernetes deployment",a_contains,"kubernetes replica sets",context);
 Gr(stdout, "kubernetes deployment",a_contains,"kubernetes daemon sets",context);

 RoleGr(stdout,"kubernetes replica sets","replica sets", "kubernetes",context);

 Gr(stdout, "kubernetes replica sets",a_depends,"kubernetes pods",context);
 Gr(stdout, "kubernetes daemon sets",a_depends,"kubernetes pods",context);
 Gr(stdout, "kubernetes replica sets",a_generalizes,"kubernetes replica set",context);
 RoleGr(stdout,"kubernetes pods","pods", "kubernetes",context);
 Gr(stdout, "kubernetes pods",a_contains,"software containers",context);
 Gr(stdout, "kubernetes pods",a_generalizes,"kubernetes pod",context);
 Gr(stdout, "software containers",a_related_to,"docker containers",context);
 Gr(stdout, "software containers",a_hasrole,"microservice",context);
 Gr(stdout, "software containers",a_generalizes,"software container",context);

}

/*****************************************************************************/

void GetPods(void)

{ FILE *pp;
 char cmd[CGN_BUFSIZE];
 char line[CGN_BUFSIZE];
 char namespace[256],replicaset[256],deploy[256],podid[256];
 int restarts;
 char *context = "Kubernetes microservices cluster deployemnt";
 
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
    line[0] = namespace[0];
    strcpy(replicaset,"<no replicas>");
    
    fgets(line,CGN_BUFSIZE,pp);

    if (!isspace(*line))
       {
       sscanf(line,"%s %s %*s %*s %d %*s",namespace,deploy,&restarts);

       char *pod = SplitPods(deploy,podid,replicaset);

       if (strlen(namespace) > 0)
          {
          //printf("\"kubernetes pod %s\" is part of replicaset %s deployment \"%s\" in  \"kubernetes namespace %s\"\n",podid,replicaset,deploy,namespace);

          PodGraph(namespace,deploy,replicaset,podid,context);
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
 char namespace[256],replicaset[256],ipaddress[128],deploy[128],podid[128];
 char port[128],hostnode[128];
 char container[1024];
 int mounts = false, env = false, containers = false;;
 char *context = ContextGr(stdout,"Kubernetes microservices cluster deployemnt");
 
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
       hostnode[0] = port[0] = podid[0];
       strcpy(replicaset,"<no replicas>");       
       sscanf(line+5,"%s",deploy);
       SplitPods(deploy,podid,replicaset);
       container[0] = '\0';
       containers = false;
       }

    // ********
    
    if (strncmp(line,"Namespace:",10) == 0) // New container
       {
       sscanf(line+10,"%s",namespace);
       
       if (strlen(namespace) > 0)
          {
          PodGraph(namespace,deploy,replicaset,podid,context);
          }
       }

    // ********
    
    if (strncmp(line,"IP: ",4) == 0) // localization
       {
       sscanf(line+4,"%s",ipaddress);
       // printf("pod %s/%s has service at cluster IP address %s\n",deploy,podid,ipaddress);
       // defer report to Port
       }

    // ********
    
    if (strncmp(line,"Node: ",5) == 0) // localization
       {
       sscanf(line+5,"%s",hostnode);
       HostNodeGraph(deploy,podid,hostnode,context);
       }

    // ********
    
    if (strncmp(line,"    Port:",9) == 0) // localization
       {
       sscanf(line+9,"%s",port);
       ClusterIPServiceGraph(deploy,podid,ipaddress,port,context);
       }

    // ********
    //  Mounts:
    //  /etc/certs/ from istio-certs (ro)
    //  /etc/istio/proxy from istio-envoy (rw)
    //  /var/run/secrets/kubernetes.io/serviceaccount from default-token-bpngl (ro)

    if (containers && mounts)
       {
       char vol[1024],host[1024],mode[16];
       vol[0] = host[0] = mode[0] = '\0';
       sscanf(line,"%s from %s %s",vol,host,mode);
       MountVolumeGraph(container,host,vol,mode);
       }
    
    if (strncmp(line,"    Mounts:",11) == 0) // localization
       {
       mounts = true;
       }

    // ********
    //    Environment:
    //  POD_NAME:       helloworld-v1-865b58b45c-ldrrc (v1:metadata.name)
    //  POD_NAMESPACE:  default (v1:metadata.namespace)
    //  INSTANCE_IP:     (v1:status.podIP)

    if (containers && env)
       {
       char lval[1024],rval[1024];
       lval[0] = rval[0] = '\0';
       sscanf(line,"%s %s",lval,rval);
       EnvironmentGraph(container,lval,rval);
       }

    if (strncmp(line,"    Environment:",16) == 0) // localization
       {
       env = true;
       }

    // ********

    if (containers)
       {
       char *sp;
       char name[1024],lval[1024],rval[1024];
       
       if ((line[3] != ' ') && (sp = strchr(line,':'))) // Name
          {
          *sp = '\0';
          sscanf(line,"%s",container);
          }
       else
          {
          rval[0] = '\0';
          sscanf(line,"%s %s",lval,rval);
          
          if (rval[0] == '\0')
             {
             strcpy(name,lval);
             }
          
          if (strcmp(lval,"Image:") == 0)
             {
             ContainerImageGraph(deploy,podid,container,rval);
             }
          }
       }
    
    if (strncmp(line,"Containers:",11) == 0) // localization
       {
       containers = true;
       }
    }

 pclose(pp);
}

/*****************************************************************************/
/* Level 1                                                                   */
/*****************************************************************************/

char *SplitPods(char *deploy, char *podid, char *replicaset)

{ char *sp;
  char *first_guess,*splitter;
  int ref = false;

 for (sp = deploy+strlen(deploy); *sp != '-'; sp--)
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

 while ((*sp != '-') && (sp > deploy))
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

/*****************************************************************************/

void PodGraph(char *namespace,char *deploy,char *replicaset,char *podid,char *context)
{
 char fconcept[1024],tconcept[1024];
 
 snprintf(fconcept,1024,"kubernetes namespace %s",namespace);
 snprintf(tconcept,1024,"kubernetes deployment %s",deploy);
 Gr(stdout,fconcept,a_contains,tconcept,context);
 RoleGr(stdout,tconcept,"kubernetes deployment",deploy,context);             
 Gr(stdout,fconcept,a_name,namespace,context);
 Gr(stdout,tconcept,a_name,deploy,context);
  
 if (strlen(replicaset) > 0)
    {
    snprintf(fconcept,1024,"kubernetes replica set %s",replicaset);
    Gr(stdout,tconcept,a_depends,fconcept,context);
    RoleGr(stdout,fconcept,"kubernetes replica set",replicaset,context);
    snprintf(tconcept,1024,"kubernetes pod %s",podid);
    Gr(stdout,fconcept,a_depends,tconcept,context);
    }
 else
    {
    snprintf(tconcept,1024,"kubernetes pod %s",podid);
    Gr(stdout,fconcept,a_depends,tconcept,context);
    }
}

/*****************************************************************************/

void  ClusterIPServiceGraph(char *deploy,char *podid,char *ipaddress, char *port,char *context)
{
 char fconcept[1024],tconcept[1024];
 char event[CGN_BUFSIZE];
 
 snprintf(event,CGN_BUFSIZE,"deployment %s pod %s was allocated IP address %s and port %s",deploy,podid,ipaddress,port);

 Gr(stdout,event,a_hasrole,"cluster allocation",context);
 
 snprintf(tconcept,1024,"kubernetes pod %s",podid);
 Gr(stdout,event,a_hasattr,tconcept,context);

 snprintf(tconcept,1024,"kubernetes deployment %s",deploy);
 Gr(stdout,event,a_hasattr,tconcept,context);

 int portnr = 0;
 sscanf(port,"%d",&portnr);
 
 if (portnr > 0)
    {
    Gr(stdout,event,a_hasattr,IPPort(portnr),context);
    }
 
}       

/*****************************************************************************/

void HostNodeGraph(char *deploy,char *podid,char *hostnode,char *context)
{
 char fconcept[1024],tconcept[1024];
 char event[CGN_BUFSIZE];
 char name[1024],ip[1024];

 strcpy(name,"<noname>");
 strcpy(ip,"<no ip>");
 
 sscanf(hostnode,"%[^/]/%s",name,ip);
     
 snprintf(event,CGN_BUFSIZE,"deployment %s pod %s was allocated to host node %s at %s",deploy,podid,name,ip);
 Gr(stdout,event,a_hasrole,"cluster allocation",context);
 
 snprintf(tconcept,1024,"kubernetes pod %s",podid);
 Gr(stdout,event,a_hasattr,tconcept,context);

 snprintf(tconcept,1024,"kubernetes deployment %s",deploy);
 Gr(stdout,event,a_hasattr,tconcept,context);

 Gr(stdout,event,a_hasattr,Hostname(name),context);
 Gr(stdout,event,a_hasattr,IPv4(ip),context);

}

/*****************************************************************************/

void ContainerImageGraph(char *deploy,char *podid,char *container,char *image)
{
 char fconcept[1024],tconcept[1024];
 char *context = "software container configuration";
 
 snprintf(fconcept,1024,"software container %s",container);
 Gr(stdout,fconcept,a_hasrole,"software container",context);

 // Connect to image
 snprintf(tconcept,1024,"container image %s",image);
 RoleGr(stdout,tconcept,"container image",image,context);
 Gr(stdout,fconcept,a_depends,tconcept,context);

 // Connect to pod
 snprintf(fconcept,1024,"kubernetes pod %s",podid);
 snprintf(tconcept,1024,"software container %s",container);
 Gr(stdout,fconcept,a_contains,tconcept,context);
 Gr(stdout,fconcept,a_depends,tconcept,context);
}

/*****************************************************************************/

void MountVolumeGraph(char *container,char *host,char *vol,char *mode)
{
 char fconcept[1024],tconcept[1024];
 char event[CGN_BUFSIZE],attr[CGN_BUFSIZE],cont[1024];
 char *context = "software container configuration";
     
 snprintf(event,CGN_BUFSIZE,"software container %s mounts external volume %s served by %s mode %s",container,vol,host,mode);
 snprintf(attr,CGN_BUFSIZE,"software container %s,storage volume %s served by %s,storage access mode %s",container,vol,HostID(host),mode);
 RoleGr(stdout,event,"mounted volume",attr,context);

 // Container depends on its volume
 snprintf(fconcept,1024,"software container %s",container);
 snprintf(tconcept,1024,"storage volume %s served by %s",vol,HostID(host));
 Gr(stdout,event,a_depends,tconcept,context);

 snprintf(cont,1024,"storage volume %s",container);
 snprintf(attr,CGN_BUFSIZE,"served by %s",vol,HostID(host));
 RoleGr(stdout,tconcept,cont,attr,context);

 snprintf(fconcept,1024,"storage access mode %s",mode);
 RoleGr(stdout,fconcept,"storage access mode",mode,"storage access");
}

/*****************************************************************************/

void EnvironmentGraph(char *container,char *lval,char *rval)
{
 char fconcept[1024],tconcept[1024];
 char event[CGN_BUFSIZE],attr[CGN_BUFSIZE],cont[1024];
 char *context = "software container configuration";
     
 snprintf(event,CGN_BUFSIZE,"software container %s inherits variable %s with value %s",container,lval,rval);
 snprintf(attr,CGN_BUFSIZE,"environment variable lvalue %s,environment variable rvalue %s",container,lval,rval);
 RoleGr(stdout,event,"inherit environment variable",attr,context);

 // Container depends on its volume
 snprintf(fconcept,1024,"software container %s",container);

 snprintf(tconcept,1024,"environment variable lvalue %s",lval);
 Gr(stdout,event,a_depends,tconcept,context);
 RoleGr(stdout,tconcept,"environment variable lvalue",lval,context);
 Gr(stdout,fconcept,a_depends,tconcept,context);
 
 snprintf(tconcept,1024,"environment variable rvalue %s",rval);
 RoleGr(stdout,tconcept,"environment variable rvalue",rval,context);
 Gr(stdout,fconcept,a_depends,tconcept,context); 

}
