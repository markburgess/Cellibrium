#!/bin/bash

curl -v -X POST -H X-Stream:true -H "Accept:application/json; charset=UTF-8" -H Content-Type:application/json \
   http://neo4j:n@localhost:7474/db/data/transaction/commit -d \
'
{
   "statements" : [ {
      "statement" : "MERGE (n:asdlk {Name: {identification}.Name}) ON CREATE SET n = {props} ON MATCH SET n = {props} RETURN id(n)",
      "parameters" : {
         "identification" : {
            "Name" : "Fjas"
         },
         "props" : {
            "Name" : "Fjas",
            "prop1" : "1",
            "prop2" : "2"
         }
      }
   } ]
)
'

#'{
#  "statements" : [ {
#      "statement" : "MERGE (n:Test) RETURN id(n)"
#   } ]
#}
#'

#curl -X POST -H X-Stream:true -H "Accept:application/json; charset=UTF-8" -H Content-Type:application/json http://localhost:7474/db/data/batch -d '
#{
#   "method": "POST",
#   "to": "/cypher",
#   "body": {
#      "query": "MATCH (n) WHERE n.Name=\"eth0\" RETURN n"
#   }
#}
#'
