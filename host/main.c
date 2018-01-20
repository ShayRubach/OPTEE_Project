/*
 * Copyright (c) 2016, Linaro Limited
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <err.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
/* OP-TEE TEE client API (built by optee_client) */
#include <tee_client_api.h>
/* To the the UUID (found the the TA's h-file(s)) */
#include <hello_world_ta.h>

#define UINT_BLOCK_SIZE			16;
// #define CRYPT_SET_PASS			0;
// #define CRYPT_ENCRYPT				1;
// #define CRYPT_DECRYPT				2;
// #define CRYPT_SHOW					3;

static uint32_t fixed_block_size = 0;

static void validateArgs(int argc);
static void show();


int main(int argc, char *argv[])
{
	char* buf = NULL;
	TEEC_Result res;
	TEEC_Context ctx;
	TEEC_Session sess;
	TEEC_Operation op;
	TEEC_UUID uuid = TA_HELLO_WORLD_UUID;
	TEEC_SharedMemory shared_mem;
	uint32_t err_origin;
	uint32_t COMMAND_TYPE = 0;

	validateArgs(argc);

	while(fixed_block_size < strlen(argv[2])+1){
		fixed_block_size += UINT_BLOCK_SIZE;
	}

	buf = malloc(sizeof(char)*fixed_block_size);

	if( buf == NULL || sizeof(buf) < sizeof(argv[2])){
			printf("failed to allocate 'buf' or size is smaller than argv[2]\n");
			return -1;
	}

	memset(buf,0,sizeof(fixed_block_size));
	strncpy(buf,argv[2],strlen(argv[2]));


	switch (atoi(argv[1])) {
		case 1:	//set password

			COMMAND_TYPE = 1;
			printf("case 1, argv[2]: %s\n",argv[2]);
			break;

		case 2: //pass a msg
			COMMAND_TYPE = 2;
			//strlen(argv[2])+1 + 15 AND zero on 4 most right bits

			break;

		case 3:	//invoke show()
			COMMAND_TYPE = 3;
			printf("case 3, argv[2]: %s\n",argv[2]);
			show();
			break;
		default:
			printf("default case\n");
			break;
	}


	/* init shared memory */
	shared_mem.buffer = buf;
	if(COMMAND_TYPE == 1){
		shared_mem.size = strlen(buf);
		printf("shared_mem.size = strlen(buf) = %zu\n",shared_mem.size );
	}
	else{
		shared_mem.size = sizeof(buf);
		printf("shared_mem.size = sizeof(buf) = %zu\n",shared_mem.size );
	}
	shared_mem.flags = TEEC_MEM_INPUT;

	/* Initialize a context connecting us to the TEE */
	res = TEEC_InitializeContext(NULL, &ctx);

	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InitializeContext failed with code 0x%x", res);

	/*
	 * Open a session to the "hello world" TA, the TA will print "hello
	 * world!" in the log when the session is created.
	 */
	res = TEEC_OpenSession(&ctx, &sess, &uuid, TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_Opensession failed with code 0x%x origin 0x%x", res, err_origin);

	/*
	 * Execute a function in the TA by invoking it, in this case
	 * we're incrementing a number.
	 *
	 * The value of command ID part and how the parameters are
	 * interpreted is part of the interface provided by the TA.
	 */

	/* Clear the TEEC_Operation struct */
	memset(&op, 0, sizeof(op));

	/*
	 * Prepare the argument. Pass a value in the first parameter,
	 * the remaining three parameters are unused.
	 */

	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INOUT, TEEC_MEMREF_WHOLE, TEEC_NONE, TEEC_NONE);

	//init memref_whole parameter
	op.params[1].memref.parent = &shared_mem;
	op.params[1].memref.size = shared_mem.size;
	op.params[1].memref.offset = 0;


	/* register the shared mem */
	TEEC_RegisterSharedMemory(&ctx,&shared_mem);

	res = TEEC_InvokeCommand(&sess, COMMAND_TYPE, &op, &err_origin);

	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x", res, err_origin);

	// printf("buf has now changed to: %s\n",buf);
	// printf("Encryption succeed!\n");

	/*
	 * We're done with the TA, close the session and
	 * destroy the context.
	 *
	 * The TA will print "Goodbye!" in the log when the
	 * session is closed.
	 */

	TEEC_ReleaseSharedMemory(&shared_mem);
	TEEC_CloseSession(&sess);
	TEEC_FinalizeContext(&ctx);

	return 0;
}

static void validateArgs(int argc){
	if(argc != 3){
		printf("ERROR: invalid number of arguments.\n");
		exit(-1);
	}
}

static void show(){

	return;
}
