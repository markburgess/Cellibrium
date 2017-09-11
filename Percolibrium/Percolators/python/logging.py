
########################################################################################################    
#
# Examples, how to encode logs as semantic graphs
#
########################################################################################################

import sys
import time
from cellibrium import Cellibrium

########################################################################################################

c = Cellibrium()

########################################################################################################
#
# HADOOP semantics
#
# Namenode
#    NameNode is the centerpiece of  HDFS.
#    NameNode is also known as the Master
#    NameNode only stores the metadata of HDFS – the directory tree of all files in the file system, and tracks the files across the cluster.
#    NameNode does not store the actual data or the dataset. The data itself is actually stored in the DataNodes.
#    NameNode knows the list of the blocks and its location for any given file in HDFS. With this information NameNode knows how to construct the file from blocks.
#    NameNode is so critical to HDFS and when the NameNode is down, HDFS/Hadoop cluster is inaccessible and considered down.
#    NameNode is a single point of failure in Hadoop cluster.
# DataNode
#    DataNode is responsible for storing the actual data in HDFS.
#    DataNode is also known as the Slave
#    NameNode and DataNode are in constant communication.
#    When a DataNode starts up it announce itself to the NameNode along with the list of blocks it is responsible for.
#    When a DataNode is down, it does not affect the availability of data or the cluster. NameNode will arrange for replication for the blocks managed by the DataNode that is not available.

########################################################################################################
    
# Example 1: 2017-01-20 15:43:33,866 INFO org.apache.hadoop.hdfs.server.datanode.fsdataset.impl.FsDatasetAsyncDiskService: Scheduling blk_1073742582_1758 file /home/extra/dfs/data/current/BP-1060243018-192.168.7.210-1475739466529/current/finalized/subdir0/subdir2/blk_1073742582 for deletion

# Hadoop datanode is a slave that stores actual data on HDFS

now = time.time() # or parse "2017-01-20 15:43:33,866" at some *appropriate* time resolution (meaningless to store every event)
who = "hadoop data node 192.168.7.210";
what = "schedule block deletion";
why = "hadoop.hdfs.server.datanode.fsdataset.impl.FsDatasetAsyncDiskService"; 
where = HereGr(sys.stdout,"NYC cloud")       # loghost
how = "/home/extra/dfs/data/current/BP-ref"  # ? some invariant or coarse grain 
icontext = "hadoop hdfs service";

c.EventClue(sys.stdout,who,what,when,where,how,why,icontext);
c.RoleGr(sys.stdout,who,"hadoop data node","host identity 192.168.7.210","hadoop hdfs service")

# Example 2: 2017-01-20 15:43:45,791 INFO org.apache.hadoop.hdfs.server.datanode.DataNode: Receiving BP-1060243018-192.168.7.210-1475739466529:blk_1073742593_1771 src: /192.168.5.176:34987 dest: /192.168.5.55:50010

now = time.time() # or parse ..

who = "hadoop data node";
what = "schedule block deletion";
why = "hadoop.hdfs.server.datanode.fsdataset.impl.FsDatasetAsyncDiskService"; 
where = HereGr(sys.stdout,"NYC cloud")
how = "/home/extra/dfs/data/current/BP-ref"  # ? some invariant or coarse grain 
icontext = "hadoop hdfs service";



# Causation 

c.Gr(sys.stdout,"performance anomaly at downstream host","a_origin","performance anomaly at upstream host","distributed system causation")
c.RoleGr(sys.stdout,"performance anomaly at upstream host","performance anomaly","at upstream host","distributed system")
c.RoleGr(sys.stdout,"performance anomaly at downstream host","performance anomaly","at downstream host","distributed system")

where = c.HereGr(sys.stdout,"mark's laptop")

print "------------------------------------------------------"

# Service relationships

# Servername = sshd
# servicename = ssh (port nr 22)

# server -> role, client/server, attr -> host identity ...

where = c.WhereGr(sys.stdout,
          "London",
          "myserver",
          "example.com",
          "123.456.789.10/24",
          "2001:::7/64",
          )

c.ServerInstanceGr(sys.stdout,
           "ssh",
           22,
           "/usr/local/sshd",
           where   
           );

where = c.WhereGr(sys.stdout,
           "San Jose",
           "desktop",
           "example.com",
           "321.654.987.99/24",
           "2001:0db8:0:f101::1/64"
           );
                  
c.ClientInstanceGr(sys.stdout,
           "ssh",
           "/usr/bin/ssh",
           where
           )

hostidentity = "123.456.789.55/24"

where = c.WhereGr(sys.stdout,
          "NYC datacentre",
          "node45-abc",
          "cloudprovider.com",
          hostidentity,
          "",
          )

c.ServerInstanceGr(sys.stdout,
           "nodemanager",
           50345,
           "/usr/local/cldstack/cloudmgrd",
           c.HereGr(sys.stdout,"Florida datacentre")
           )

print "------------------------------------------------------"

