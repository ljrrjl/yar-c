
/**
 * async.c
 *
 * @author  Laruence
 * @date    2012-10-16 17:47
 * @version $Id$
 */

#include <stdio.h>   	/* for fprintf */
#include <string.h>
#include <getopt.h> 	/* for getopt */
#include <assert.h>     /* for assert */
#include <stdlib.h>
#include <sys/types.h>

#include "yar.h"
#include "msgpack.h" 

struct id_message
{
	char* name;
	int age;
};

struct response
{
	int error_code;
};

int id_message_handler(struct response* rsp, struct id_message* msg)
{
	printf("[info] name => %s, age => %d\n", msg->name, msg->age);
	rsp->error_code = 100;
	return 0;
}

void id_message_handler_(yar_request *request, yar_response *response, void *cookie)
{
	yar_packager* packager = NULL;

	struct id_message msg;	
	msg.name = NULL;
	msg.age = 0;

	const yar_data* parameters = yar_request_get_parameters(request);
	assert(((long)cookie) == 1);

	yar_unpack_iterator* it_array = yar_unpack_iterator_init(parameters);
	do
	{
		const yar_data* map_data = yar_unpack_iterator_current(it_array);
		yar_unpack_iterator* it_map = yar_unpack_iterator_init(map_data);
		do
		{
			struct msgpack_object_kv* kv_data = (struct msgpack_object_kv*)yar_unpack_iterator_current(it_map);
			uint32_t strlength = kv_data->key.via.str.size;
			char* buffer = (char*)malloc(strlength + 1);
			memset(buffer, 0, strlength + 1);
			memcpy(buffer, kv_data->key.via.str.ptr, strlength);
			if(strcmp(buffer, "name") == 0)
			{
				uint32_t val_strlength = kv_data->val.via.str.size;
				msg.name = (char*)malloc(val_strlength + 1);
				memset(msg.name, 0, val_strlength + 1);
				memcpy(msg.name, kv_data->val.via.str.ptr, val_strlength);
			}
			else if(strcmp(buffer, "age") == 0)
			{
				msg.age = kv_data->val.via.i64;
			}
			else
			{
				printf("[error] unpack fail\n");
			}
			free(buffer);
			yar_unpack_iterator_next(it_map); //kv
		}while(yar_unpack_iterator_next(it_map));
		yar_unpack_iterator_free(it_map);
	}while(yar_unpack_iterator_next(it_array));
	yar_unpack_iterator_free(it_array);
	struct response rsp;
	memset(&rsp, 0, sizeof(struct response));
	id_message_handler(&rsp, &msg);
	
	packager = yar_pack_start_map(1);
	yar_pack_push_string(packager, "error_code", strlen("error_code"));
	yar_pack_push_long(packager, rsp.error_code);

	yar_response_set_retval(response, packager);
	yar_pack_free(packager);
}

void yar_handler_example(yar_request *request, yar_response *response, void *cookie) {
	yar_packager *packager;
	const yar_data *parameters = yar_request_get_parameters(request);
	assert(((long)cookie) == 1);

	yar_unpack_iterator* it_array = yar_unpack_iterator_init(parameters);
	do
	{
		const yar_data* map_data = yar_unpack_iterator_current(it_array);
		yar_unpack_iterator* it_map = yar_unpack_iterator_init(map_data);
		do
		{
			struct msgpack_object_kv* kv_data = (struct msgpack_object_kv*)yar_unpack_iterator_current(it_map);
			uint32_t strlength = kv_data->key.via.str.size;
			char* buffer = (char*)malloc(strlength + 1);
			memset(buffer, 0, strlength + 1);
			memcpy(buffer, kv_data->key.via.str.ptr, strlength);
			
			uint32_t val_strlength = kv_data->val.via.str.size;
			char* val_buffer = (char*)malloc(val_strlength + 1);
			memset(val_buffer, 0, strlength + 1);
			memcpy(val_buffer, kv_data->val.via.str.ptr, val_strlength);

			printf("[info] key: %s ==> value: %s\n", buffer, val_buffer);
			free(buffer);
			free(val_buffer);
			yar_unpack_iterator_next(it_map); //kv
		}while(yar_unpack_iterator_next(it_map));
		yar_unpack_iterator_free(it_map);
	}while(yar_unpack_iterator_next(it_array));
	yar_unpack_iterator_free(it_array);

	packager = yar_pack_start_map(1);
	yar_pack_push_string(packager, "status", 6);
	yar_pack_push_string(packager, "ok", 2);

	yar_response_set_retval(response, packager);
	yar_pack_free(packager);
}

yar_server_handler example_handlers[] = {
	{"default", sizeof("default") - 1, yar_handler_example},
	{"register", sizeof("register") - 1, id_message_handler_},
	{NULL, 0, NULL}
};

int main(int argc, char **argv) {
	int opt, max_childs = 5;
	char *hostname = NULL, *yar_pid = NULL, *log_file = NULL;
	char *user = NULL, *group = NULL;
	int standalone = 0;

	while ((opt = getopt(argc, argv, "hS:n:K:p:Xl:u:g:")) != -1) {
		switch (opt) {
			case 'n':
				max_childs = atoi(optarg);
				break;
			case 'S':
				hostname = optarg;
				break;
			case 'p':
				yar_pid = optarg;
				break;
			case 'X':
				standalone = 1;
				break;
			case 'l':
				log_file = optarg;
				break;
			case 'u':
				user = optarg;
				break;
			case 'g':
				group = optarg;
				break;
			default:
				yar_server_print_usage(argv[0]);
				return 0;
		}
	}

	if (!hostname) {
		yar_server_print_usage(argv[0]);
		return 0;
	}	

	if (yar_server_init(hostname)) {
		yar_server_set_opt(YAR_STAND_ALONE, &standalone);
		yar_server_set_opt(YAR_MAX_CHILDREN, &max_childs);
		yar_server_set_opt(YAR_PID_FILE, yar_pid);
		yar_server_set_opt(YAR_LOG_FILE, log_file);
		yar_server_set_opt(YAR_CUSTOM_DATA, (void *)1);
		yar_server_set_opt(YAR_CHILD_USER, user);
		yar_server_set_opt(YAR_CHILD_GROUP, group);
		yar_server_register_handler(example_handlers);
		yar_server_run();
	}
	return 0;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
