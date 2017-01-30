# Generating example data. Start here

INCLUDES = -I. -I../../RobIoTs/CGNgine/libpromises -I../../RobIoTs/CGNgine/libutils -I../../RobIoTs/CGNgine/libcfnet

all:
# CGNgine data, generated internally
	cp ~/.CGNgine/state/env_graph ExampleTupleData/
	cp ~/.CGNgine/policy_graph ExampleTupleData/
# Build
	gcc $(INCLUDES) -o gen_bdk -g generate_binary_domain_knowledge.c
	gcc $(INCLUDES) -o gen_dsdk -g generate_doctor_domain_knowledge.c
	gcc $(INCLUDES) -o gen_cgndk -g generate_cgn_domain_knowledge.c
	gcc $(INCLUDES) -o gen_msdk -g generate_microservice_domain_knowledge.c
# Manual doman knowledge
	./gen_bdk > ExampleTupleData/dependencies_graph
	./gen_dsdk > ExampleTupleData/doctorservice_graph
	./gen_cgndk > ExampleTupleData/domain_knowledge_graph 
	./gen_msdk > ExampleTupleData/microservice_component_graph
# Now build the graph
	gcc -o conceptualize -g -std=c99 conceptualize-fs.c
	./conceptualize ExampleTupleData/*

# The go to ../Narrators for search