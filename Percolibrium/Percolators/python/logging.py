
########################################################################################################    
#
# Examples, how to encode logs as semantic graphs
#
########################################################################################################

import sys
import time
import socket
from cellibrium import Cellibrium

########################################################################################################

c = Cellibrium()

########################################################################################################
#
# HADOOP semantics
#
#    NameNode is the centerpiece of  HDFS.
#    NameNode is also known as the Master
#    NameNode only stores the metadata of HDFS, the directory tree of all files in the file system, and tracks the files across the cluster.
#    NameNode does not store the actual data or the dataset. The data itself is actually stored in the DataNodes.
#    NameNode knows the list of the blocks and its location for any given file in HDFS. NameNode knows how to construct the file from blocks.
#    NameNode is so critical to HDFS and when the NameNode is down, HDFS/Hadoop cluster is inaccessible and considered down.
#    NameNode is a single point of failure in Hadoop cluster.
#
#    DataNode is responsible for storing the actual data in HDFS.
#    DataNode is also known as the Slave
#    NameNode and DataNode are in constant communication.
#    When a DataNode starts up it announce itself to the NameNode along with the list of blocks it is responsible for.
#    When a DataNode is down, redundant backup. NameNode arranges for replication for the blocks managed by the DataNode that is not available.

# DOCS https://www.cloudera.com/documentation/enterprise/latest/topics/cdh_ig_ports_cdh5.html

#######################################################################################################

# This sensor log location for all data parsed

thishost = socket.gethostname()
print "This host is " + thishost
here = c.HereGr(sys.stdout,"NYC cloud")

icontext = "hadoop HDFS service"

 #
 # In these examples IP(thishost) seems to be 192.168.7.210 (
 # nodemanager seems to be 192.168.5.65
 # src address = client address src 192.168.5.176:34987 (write)
 # dest address = datanode dst 192.168.5.55:50010 (but why not the same as 210?) replica ??

# From namenode logs, data replicas are pipelined in order from 1st to last

# 2017-01-20 15:41:30,158 INFO org.apache.hadoop.hdfs.StateChange: BLOCK* allocateBlock: /tmp/hadoop-yarn/staging/crluser/.staging/job_1484893655240_0007/job.split. BP-1060243018-192.168.7.210-1475739466529 blk_1073742597_1775{blockUCState=UNDER_CONSTRUCTION, primaryNodeIndex=-1,
#replicas=[
#    ReplicaUnderConstruction[[DISK]DS-0503a590-18a8-4201-bc23-7da0bbf9dfa5:NORMAL:192.168.5.65:50010|RBW],
#    ReplicaUnderConstruction[[DISK]DS-201f600e-1246-436b-85ed-567351cd75ef:NORMAL:192.168.5.56:50010|RBW],
#    ReplicaUnderConstruction[[DISK]DS-f0f2ca98-8c60-4ed9-917c-a58ebba5e325:NORMAL:192.168.5.175:50010|RBW],
#    ReplicaUnderConstruction[[DISK]DS-fe82d9ee-25bf-4e6a-8dca-9b9426ada118:NORMAL:192.168.5.176:50010|RBW],
#    ReplicaUnderConstruction[[DISK]DS-f515e02d-543e-442f-81a1-f45a826d6aec:NORMAL:192.168.5.55:50010|RBW]]}
 
#2017-01-20 15:41:30,196
#INFO BlockStateChange: BLOCK* addStoredBlock: blockMap updated: 192.168.5.55:50010 is added to blk_1073742597_1775{blockUCState=UNDER_CONSTRUCTION, primaryNodeIndex=-1, replicas=
#[ReplicaUnderConstruction[[DISK]DS-0503a590-18a8-4201-bc23-7da0bbf9dfa5:NORMAL:192.168.5.65:50010|RBW],
# ReplicaUnderConstruction[[DISK]DS-201f600e-1246-436b-85ed-567351cd75ef:NORMAL:192.168.5.56:50010|RBW],
# ReplicaUnderConstruction[[DISK]DS-f0f2ca98-8c60-4ed9-917c-a58ebba5e325:NORMAL:192.168.5.175:50010|RBW],
# ReplicaUnderConstruction[[DISK]DS-fe82d9ee-25bf-4e6a-8dca-9b9426ada118:NORMAL:192.168.5.176:50010|RBW],
# ReplicaUnderConstruction[[DISK]DS-f515e02d-543e-442f-81a1-f45a826d6aec:NORMAL:192.168.5.55:50010|RBW]]} size 0

# The Hadoop replication pipeline

namenodehub = "hadoop namenode %s %s" % (c.HostID("192.168.5.65"),c.IPv4("192.168.5.65"))
attr = "%s,%s" % (c.HostID("192.168.5.65"),c.IPv4("192.168.5.65"))

c.RoleGr(sys.stdout,namenodehub,"hadoop namenode",attr,icontext)

c.ServerListenPromise(sys.stdout,"192.168.5.65","Hadoop Datanode",50010)
c.ServerListenPromise(sys.stdout,"192.168.5.56","Hadoop Datanode",50010)
c.ServerListenPromise(sys.stdout,"192.168.5.175","Hadoop Datanode",50010)
c.ServerListenPromise(sys.stdout,"192.168.5.176","Hadoop Datanode",50010)
c.ServerListenPromise(sys.stdout,"192.168.5.55","Hadoop Datanode",50010)

c.ServerAcceptPostData(sys.stdout,"192.168.5.65","192.168.7.210","Hadoop DataNode","scheduling file for deletion")
c.ServerAcceptPostData(sys.stdout,"192.168.5.56","192.168.7.65","Hadoop DataNode","scheduling file for deletion")
c.ServerAcceptPostData(sys.stdout,"192.168.5.175","192.168.7.56","Hadoop DataNode","scheduling file for deletion")
c.ServerAcceptPostData(sys.stdout,"192.168.5.176","192.168.7.175","Hadoop DataNode","scheduling file for deletion")
c.ServerAcceptPostData(sys.stdout,"192.168.5.55","192.168.7.176","Hadoop DataNode","scheduling file for deletion")

c.ClientPush(sys.stdout,"192.168.7.210","192.168.7.65","replica block","Hadoop DataNode",50010)
c.ClientPush(sys.stdout,"192.168.7.65","192.168.7.56","replica block","Hadoop DataNode",50010)
c.ClientPush(sys.stdout,"192.168.7.56","192.168.7.175","replica block","Hadoop DataNode",50010)
c.ClientPush(sys.stdout,"192.168.7.175","192.168.7.176","replica block","Hadoop DataNode",50010)

#######################################################################################################
# Example 1: 2017-01-20 15:43:33,866 INFO org.apache.hadoop.hdfs.server.datanode.fsdataset.impl.FsDatasetAsyncDiskService: Scheduling blk_1073742582_1758 file /home/extra/dfs/data/current/BP-1060243018-192.168.7.210-1475739466529/current/finalized/subdir0/subdir2/blk_1073742582 for deletion

 # Hadoop datanode is a slave that stores actual data on HDFS

c.ServerInstanceGr(sys.stdout,"Hadoop DataNode",50010,"hdfs.server.datanode",here)
c.ServerInstanceGr(sys.stdout,"secure Hadoop Datanode",1004,"hdfs.server.datanode",here)
c.ServerListenPromise(sys.stdout,thishost,"secure Hadoop Datanode",1004)
c.ServerListenPromise(sys.stdout,thishost,"Hadoop Datanode",50010)

 # the specific event - who is the client?? 192.168.7.210??

now = time.time() # or parse "2017-01-20 15:43:33,866" at some *appropriate* time resolution (meaningless to store every event)
who = "hadoop data node 192.168.7.210";
what = "schedule block deletion";
why = "hadoop.hdfs.server.datanode.fsdataset.impl.FsDatasetAsyncDiskService"; 
how = "/home/extra/dfs/data/current/BP-ref"  # ? some invariant or coarse grain 


c.EventClue(sys.stdout,who,what,0,here,how,why,icontext);
c.RoleGr(sys.stdout,who,"hadoop data node","host identity 192.168.7.210",icontext)

#######################################################################################################
# Example 2: 2017-01-20 15:43:45,791 INFO org.apache.hadoop.hdfs.server.datanode.DataNode: Receiving BP-1060243018-192.168.7.210-1475739466529:blk_1073742593_1771 src: /192.168.5.176:34987 dest: /192.168.5.55:50010

# 3 IP addresses: src 192.168.5.176:34987 dst 192.168.5.55:50010" 

now = time.time() # or parse .. or submit 0 for repeated event
src = "192.168.5.176"
dst = "192.168.5.55"

who = "from %s to %s" % (c.HostID(src),c.HostID(dst))
what = "forward data block";
why = "%s writes data" % c.HostID("192.168.7.210")
where = c.HereGr(sys.stdout,"NYC cloud")
how = "Receiving BP-1060243018-192.168.7.210"


c.EventClue(sys.stdout,who,what,0,where,how,why,icontext);
c.RoleGr(sys.stdout,where,"hadoop data client",c.HostID("192.168.7.210"),icontext)

# implicit

c.ServerAcceptPromise(sys.stdout,dst,src,"Hadoop DataNode",50010)
c.ClientPush(sys.stdout,src,dst,"replica block","Hadoop DataNode",50010)

#######################################################################################################
#Example 4: 127.0.0.1 - - [20/Jan/2017:05:13:34 +0000] "GET /PHP/RUBBoS_logo.jpg HTTP/1.1" 200 10010 "-" "Java/1.7.0_121"

# From RUBBoS 

client = "127.0.0.1";
server = "127.0.0.1";
servicename = "Rubbos"
portnumber = 2712 #??

# Strip out specifics of request, into invariant categories that are RELEVANT to debugging

c.ServerAcceptPromise(sys.stdout,server,client,servicename,portnumber)
c.ClientWritePostData(sys.stdout,client,server,"GET PHP image",servicename,portnumber)
c.ServerReplyToGetData(sys.stdout,server,client,servicename,"PHP image")

now = 0 # for repeated event
src = "127.0.0.1"
dst = "127.0.0.1"

who = "from %s to %s" % (c.HostID(src),c.HostID(dst))
what = "web service GET request";
why = "Rubbos web service request" 
where = c.HereGr(sys.stdout,"NYC cloud")
how = "connect to port 2712?"
icontext = "Rubbos service"

c.EventClue(sys.stdout,who,what,0,where,how,why,icontext);

#######################################################################################################
#Example 5: SELECT * FROM stories ORDER BY date DESC LIMIT 10
# from RUBBoS 

client = "127.0.0.1";
server = "127.0.0.1";
servicename = "mysql"
request = "SELECT * FROM stories ORDER BY date DESC LIMIT 10"
portnumber = 3306

c.ServerAcceptPromise(sys.stdout,server,client,servicename,portnumber)
c.ClientWritePostData(sys.stdout,client,server,request,servicename,portnumber)
c.ServerReplyToGetData(sys.stdout,server,client,servicename,request)

now = 0 # for repeated event
src = "127.0.0.1"
dst = "127.0.0.1"

who = "from %s to %s" % (c.HostID(src),c.HostID(dst))
what = "SQL lookup";
why = "Rubbos web service request" 
where = c.HereGr(sys.stdout,"NYC cloud")
how = "connect to port 3306"
icontext = "Rubbos service"

c.EventClue(sys.stdout,who,what,0,where,how,why,icontext);


###############################################################################

print "extracted log time granule key = " + c.LogTimeKeyGen1("2017-01-20 15:43:33")

###############################################################################

# Register each node, foreach IP

namenodehub = "hadoop namenode %s %s" % (c.HostID("192.168.5.65"),c.IPv4("192.168.5.65"))
attr = "%s,%s" % (c.HostID("192.168.5.65"),c.IPv4("192.168.5.65"))
c.RoleGr(sys.stdout,namenodehub,"hadoop namenode",attr,icontext)


# Events currently recognized



# 1. 'addToInvalidates', '' - see sourcecode http://grepcode.com/file/repo1.maven.org/maven2/org.apache.hadoop/hadoop-hdfs/0.22.0/org/apache/hadoop/hdfs/server/namenode/BlockManager.java#BlockManager.addToInvalidates%28org.apache.hadoop.hdfs.protocol.Block%29
# descr "Adds block to list of blocks which will be invalidated on all its datanodes" 

# 2. 'allocateBlock', 'replica'
# 3. 'addStoredBlock', 'replica'
# 4. 'replicate', 'replica'


# All of these are pipeline pushes (2 x IP addresses and a timestamp)

c.ServerAcceptPostData(sys.stdout,"192.168.5.55","192.168.7.176","Hadoop DataNode","scheduling file for deletion")
c.ClientPush(sys.stdout,"192.168.7.210","192.168.7.65","replica block","Hadoop DataNode",50010)
c.ServerListenPromise(sys.stdout,"192.168.5.65","Hadoop Datanode",50010)
c.LogTimeFormat1(sys.stdout,"2017-01-20 15:43:33")


# THERE MAY BE 2 KINDS OF ANOMALY

# a) There might be some semantic anomalies (message type unknown)
#  see https://issues.apache.org/jira/browse/HDFS-9650 anomaly "Redundant addStoredBlock request received"

who  = "from %s to %s" % (c.HostID(src),c.HostID(dst))
what = "ANOMALOUS LOG MESSAGE";
why = "Redundant addStoredBlock request received " + who
when = datetime.strptime(str,'%Y-%m-%d %H:%M:%S') 
src  = "192.168.5.176"
dst  = "192.168.5.55"
where = c.HereGr(sys.stdout,"NYC cloud")          # on loghost, or adapt to give argument
how = ""

c.EventClue(sys.stdout,who,what,when,where,how,why,icontext);

#
# b) We can try to get more by looking for frequency anomalies (frequency is non-invariant, so we need to detect an invariant set
# of anomaly conditions from the frequencies by preprocessing
#

# Store (timekey, from_IP_to_IP, granule_average)

print "current time granule key = " + c.TimeKeyGen(now)
print "extracted log time granule key = " + c.LogTimeKeyGen1("2017-01-20 15:43:33")
