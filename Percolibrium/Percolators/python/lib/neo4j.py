# /usr/bin/env python

import urllib, urllib2, json, sys, shlex, re, os, base64

class Neo4j:
	def __init__(self, neo4j_url, neo4j_user, neo4j_pass):
		self.neo4j_user = neo4j_user
		self.neo4j_pass = neo4j_pass
		self.neo4j_url = neo4j_url


	def neo4j_rest_cypher(self, query_data):
		b64 = base64.b64encode('%s:%s' % (self.neo4j_user, self.neo4j_pass))
		request = urllib2.Request(self.neo4j_url + '/cypher',
			data = json.dumps(query_data),
			headers = { 
				'Content-Type': 'application/json', 
				'Accept': 'application/json; charset=UTF-8',
				'X-Stream': 'true',
				'Authorization': 'Basic %s' % b64
			})
		return json.loads(urllib2.urlopen(request).read())

	def neo4j_create_or_update_node(self, query_data):
		if query_data['ids']:
 			ids = [];
			for id in query_data['ids'].keys():
				ids.append('has(n.' + id + ')')
				ids.append('n.' + id + '={' + id + '}')
				clause = ' and '.join(ids) 
				q = {
					'query' : 'START n=node(*) WHERE ' + clause + ' RETURN n',
					'params' : query_data['ids']
				}
				response = self.neo4j_rest_cypher(q)
				if len(response['data']) == 0:
					self.neo4j_create_node(query_data)
				elif len(response['data']) == 1:
					self.neo4j_update_node(query_data)
				else:
					print 'ERROR: Found several matching nodes (' + str(len(response['data'])) + ')'

#MERGE (sp:Switchport {MAC: {sp_id}.MAC, Name: {sp_id}.Name}) SET sp += {sp_props}

	def neo4j_create_node(self,query_data):
		props = []
		params = {}
		if 'ids' in query_data:
			for id in query_data['ids']:
				props.append(id + ': {' + id + '}')
				params[id] = query_data['ids'][id]

			if 'properties' in query_data:
				for prop in query_data['properties']:
					props.append(prop + ': {' + prop + '}')
					params[prop] = query_data['properties'][prop]
      
			query = 'CREATE (n {' + ', '.join(props) + '})'
			q = {
				'query' : query,
				'params' : params
      }
			response = self.neo4j_rest_cypher(q)
         
	def neo4j_update_node(self,query_data):
		ids = []
		props = []
		params = {}
		p = {}
		if 'ids' in query_data:
			for id in query_data['ids']:
				ids.append('has(n.' + id + ')')
				ids.append('n.' + id + '={' + id + '}')
				params[id] = query_data['ids'][id]

			if 'properties' in query_data:
				for prop in query_data['properties']:
					props.append('SET n.' + prop + '={' + prop + '}')
					params[prop] = query_data['properties'][prop]

			query = 'START n=node(*) WHERE ' + ' AND '.join(ids) + ' ' + ' '.join(props)
			q = {
				'query' : query,
				'params' : params
			}

			response = self.neo4j_rest_cypher(q)

	def neo4j_rest_transaction_commit(self,query_data):
		b64 = base64.b64encode('%s:%s' % (self.neo4j_user, self.neo4j_pass))
		request = urllib2.Request(self.neo4j_url + '/transaction/commit',
			data = json.dumps(query_data),
			headers = {
				'Content-Type': 'application/json', 
				'Accept': 'application/json; charset=UTF-8',
				'X-Stream': 'true',
				'Authorization': 'Basic %s' % b64
			} )
		return json.loads(urllib2.urlopen(request).read())
#   def neo4j_get_node_id(self,label,param,value):
#      query = "start n = node(*) where (n:" + label + ") and n." + param + " = {value} return id(n)";
#      query_data = { 'query': query, 'params': { 'value' : value } }
#      return self.neo4j_rest_cypher(query_data)
#   
#   def neo4j_property_set(self,node_url,name,value): 
#      request = urllib2.Request(node_url + '/properties/' + name,
#         data = value,
#         headers = { 'Content-Type': 'application/json' } )
#      request.get_method = lambda: 'PUT'
#      response = urllib2.urlopen(request)
#      return response.getcode() == 204
