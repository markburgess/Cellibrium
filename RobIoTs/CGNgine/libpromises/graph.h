/*****************************************************************************/
/*                                                                           */
/* File: graph.h                                                             */
/*                                                                           */
/* Created: Mon Aug 29 10:32:25 2016                                         */
/*                                                                           */
/*****************************************************************************/

enum associations
{
    a_contains,
    a_generalizes,
    a_origin,
    a_providedby,
    a_maintainedby,
    a_depends,
    a_caused_by,
    a_uses,
    a_name,
    a_hasattr,  // avoid this, it says nothing unambiguously
    a_promises, 
    a_hasinstance,
    a_hasvalue,
    a_hasarg,
    a_hasrole,     // what is its function/object type? e.g. file,process..
    a_hasoutcome,
    a_hasfunction,
    a_hasconstraint,
    a_interpreted,
    a_concurrent,
    a_alias,
    a_approx,
    a_related_to,
    a_ass_dim
};

typedef struct
{
   int type;
   char *fwd;
   char *bwd;
}
Association;

#ifndef PERCOLATION
void Gr(FILE *consc,char *from, enum associations assoc, char *to, char *context);
void IGr(FILE *consc,char *from, enum associations assoc, char *to, char *context);
void GrQ(FILE *consc,char *from, enum associations assoc, double to, char *context);
void Number(FILE *consc, double q, char *context);
char *RoleGr(FILE *consc,char *compound_name, char *role, char *attributes, char *ex_context);
char *ContextGr(FILE *consc,char *compound_name);
char *NamedContextGr(FILE *consc,char *compound_name,char *list);
void MakeUniqueGrName(char *lval,void *sorted,char type,char *buffer);
char *ImpositionGr(FILE *fp,char *S, char *R, char *body);
char *AcceptPromiseGr(FILE *fp,char *R, char *S, char *body);
char *GivePromiseGr(FILE *fp,char *S, char *R, char *body);

void InitialGr(FILE *fp);
char *TimeGr(FILE *fp,time_t time);
char *WhereGr(FILE *fp,char *address, char *uqhn, char *domain, char *ipv4, char *ipv6);
char *EventClue(FILE *fp,char *who,char *what, time_t whentime, char *where, char *how, char *why,char *icontext);

// independent of IP

char *ServiceGr(FILE *fp,char *servicename, unsigned int port);

// instance sneed ip/host - open for business at a particular location

char *ClientInstanceGr(FILE *fp,char *servicename,char *clientname,char *where);
char *ServerInstanceGr(FILE *fp,char *servicename, unsigned int portnumber,char *servername,char *where);

// instance promises

char *ClientQuery(FILE *fp,char *client, char *server, char *request, char *servicename, int portnumber);
char *ClientPush(FILE *fp,char *client, char *server, char *request, char *servicename, int portnumber);
char *ServerListenPromise(FILE *fp,char *servername, char *servicename, int port);
char *ServerAcceptPromise(FILE *fp,char *servername, char *fromclient, char *servicename, int port);
char *ServerReplyPromise(FILE *fp,char *server, char *toclient, char *servicename, int port);
char *ClientWritePostData(FILE *fp,char *S, char *R, char *data,char *servicename, int portnumber);
char *ClientReadGetData(FILE *fp,char *client, char *server, char *servicename, char *get, int portnumber);
char *ServerAcceptPostData(FILE *fp,char *server,char *client,char *servicename, char *data);
char *ServerReplyToGetData(FILE *fp,char *server,char *client,char *servicename, char *data);

char *ExceptionGr(FILE *fp,char *origin,char *logmessage);
char *SanitizeString(char *s);

char *SService(char *servicename);
char *SServerInstance(char *service,char *server);
char *SClientInstance(char *service,char *client);
char *SServer(char *service);
char *SClient(char *service);
#endif

/*****************************************************************************/

#ifndef GRAPH
extern Association A[a_ass_dim+1];
#endif

#ifndef CGN_BUFSIZE
# define CGN_BUFSIZE 4096
#endif

#define GR_CONTAINS  3 
#define GR_FOLLOWS   2 // i.e. influenced by
#define GR_EXPRESSES 4 // represents, etc
#define GR_NEAR      1 // approx like
#define GR_CONTEXT   5 // approx like

#define ALL_CONTEXTS "any"

#define CGN_BUFSIZE 4096

// Don't like using /tmp but keep is simple here - personalize to make non-shared etc

#define BASEDIR "/tmp/KMdata"

