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
#include <unistd.h>
/* OP-TEE TEE client API (built by optee_client) */
#include <tee_client_api.h>
/* To the the UUID (found the the TA's h-file(s)) */
#include <hello_world_ta.h>


#define REF_OFFSET				(0)
#define CHUNK_SIZE				(16)
#define ERROR_KEY_NOT_GENERATED (-99)
#define INVALID_ARGS			(-1)
#define INPUT_ERROR				(-1)

#define OP_TEE_DEBUG_MODE (0)
#define OP_TEE_DEBUG if(OP_TEE_DEBUG_MODE == 1)

#define SCPS_INIT					"CPS_INIT"
#define SCPS_PROTECT			"CPS_PROTECT"
#define SCPS_VIEW					"CPS_VIEW"
#define SCPS_VIEW_RAW			"raw"
#define SCPS_VIEW_ASCII		"ascii"

static FILE* fd = NULL;


enum CPS_TYPE {
	CPS_INIT = 1,
	CPS_PROTECT,
	CPS_VIEW,
	CPS_VIEW_RAW,
	CPS_VIEW_ASCII,
	CPS_INVALID_OPERATION
};

static int 		etoi(char* command);															// enum to int
static int 		closeFile(FILE* fd);															// safe close file
static FILE* 	openFile(const char* filePath,const char* mode);	// safe open file
static size_t getLastByteFromFile(FILE* fd);										// get last byte of file
static void 	printFile(FILE* fd);

int main(int argc, char *argv[])
{
	TEEC_Result res;
	TEEC_Context ctx;
	TEEC_Session sess;
	TEEC_Operation op;
	TEEC_UUID uuid = TA_HELLO_WORLD_UUID;
	TEEC_SharedMemory shared_mem;
	uint32_t err_origin;
	enum CPS_TYPE CPS;
	size_t readBytes = 0;
	char* file_name = NULL;

	// validate input
	//if(INVALID_ARGS == (validateArgs(argc)))
	//	return INPUT_ERROR;
	if(CPS_INVALID_OPERATION == (	CPS = etoi(argv[1])))
		return INPUT_ERROR;


	shared_mem.buffer = (char*)malloc(CHUNK_SIZE);
	shared_mem.size = CHUNK_SIZE;
	shared_mem.flags = TEEC_MEM_INPUT;

	if(!shared_mem.buffer){
		printf("shared_mem.buffer is null.\n");
		return -1;
	}

		/* Clear the TEEC_Operation struct */
	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INOUT, TEEC_MEMREF_WHOLE, TEEC_NONE, TEEC_NONE);
	//init memref_whole parameter
	op.params[1].memref.parent = &shared_mem;
	op.params[1].memref.size = shared_mem.size;
	op.params[1].memref.offset = REF_OFFSET;

	OP_TEE_DEBUG printf("TEEC_InitializeContext called.\n");
	res = TEEC_InitializeContext(NULL, &ctx);
	if ( res != TEEC_SUCCESS)
		errx(1, "TEEC_InitializeContext failed with code 0x%x", res);

	OP_TEE_DEBUG printf("TEEC_OpenSession called.\n");
	res = TEEC_OpenSession(&ctx, &sess, &uuid, TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);
	if(res != TEEC_SUCCESS)
		errx(1, "TEEC_Opensession failed with code 0x%x origin 0x%x", res, err_origin);


	OP_TEE_DEBUG printf("TEEC_RegisterSharedMemory called.\n");
	TEEC_RegisterSharedMemory(&ctx,&shared_mem);


	switch (CPS) {
		size_t lastByte = 0;
		case CPS_INIT:	//set password

			OP_TEE_DEBUG printf("case : %s\n",argv[1]);

			memset(shared_mem.buffer,0,CHUNK_SIZE);
			strncpy(shared_mem.buffer,argv[2],strlen(argv[2]));

			OP_TEE_DEBUG printf("TEEC_InvokeCommand called.\n");
			res = TEEC_InvokeCommand(&sess, CPS, &op, &err_origin);

			if(res != TEEC_SUCCESS)
				errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x", res, err_origin);

			break;

		case CPS_VIEW:	//invoke show()
			file_name = argv[3];
			if(NULL == (fd = openFile(file_name,"r+")))
				return -1;

			//printFile(fd);
			lastByte = getLastByteFromFile(fd);

			OP_TEE_DEBUG printf("case : %s\n",argv[1]);
			if(strcmp(argv[2],SCPS_VIEW_RAW) == 0){
				CPS = CPS_VIEW_RAW;
			}
			else if(strcmp(argv[2],SCPS_VIEW_ASCII) == 0){
				CPS = CPS_VIEW_ASCII;
			}
			else{
				printf("Invalid argument: %s\n",argv[2]);
				return INPUT_ERROR;
			}

			while(lastByte != ftell(fd)){
				memset(shared_mem.buffer,0,CHUNK_SIZE);
				readBytes = fread(shared_mem.buffer,1,CHUNK_SIZE,fd);
				res = TEEC_InvokeCommand(&sess, CPS, &op, &err_origin);

				if(res != TEEC_SUCCESS){
					errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x", res, err_origin);
					if(res == ERROR_KEY_NOT_GENERATED){
						printf("Generate a key first.\n");
					}

					//force out
					lastByte = ftell(fd);
				}
			}

			closeFile(fd);

			break;

		case CPS_PROTECT: //pass a msg
			file_name = argv[2];
			OP_TEE_DEBUG printf("case : %s\n",argv[1]);

			if(NULL == (fd = openFile(file_name,"r+")))
				return -1;

			lastByte = getLastByteFromFile(fd);

			while(lastByte > ftell(fd)){
				memset(shared_mem.buffer,0,CHUNK_SIZE);
				readBytes = fread(shared_mem.buffer,1,CHUNK_SIZE,fd);
				fseek(fd,readBytes*(-1),SEEK_CUR);

				res = TEEC_InvokeCommand(&sess, CPS, &op, &err_origin);

				if(res != TEEC_SUCCESS){
					errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x", res, err_origin);
					if(res == ERROR_KEY_NOT_GENERATED){
						printf("Generate a key first.\n");
					}

					//force out
					lastByte = ftell(fd);
				}
				else{
					fwrite(shared_mem.buffer,1,CHUNK_SIZE,fd);
				}

			}

			closeFile(fd);
			break;

		default:
			OP_TEE_DEBUG printf("default case\n");
			break;
	}


	if(NULL != shared_mem.buffer)
		free(shared_mem.buffer);


	TEEC_ReleaseSharedMemory(&shared_mem);
	TEEC_CloseSession(&sess);
	TEEC_FinalizeContext(&ctx);

	return 0;
}


static FILE* openFile(const char* file_name,const char* mode){

	OP_TEE_DEBUG printf("opening file %s , with mode: %s\n",file_name,mode);
	fd = fopen(file_name,mode);
	if(fd != NULL){
		OP_TEE_DEBUG printf("file opened successfully.\n");
	}
	else{
		printf("failed to open file.\n");
	}
	return fd;
}

static int closeFile(FILE* fd){
	int ret = -1;
	OP_TEE_DEBUG printf("closing file.\n");
	if(fd != NULL)
		ret = fclose(fd);
	if(ret == 0){
		OP_TEE_DEBUG printf("successfully closed file.\n");
	}
	else{
		printf("failed to close file.\n");
	}
	return ret;
}

static size_t getLastByteFromFile(FILE* fd){
	size_t curr_byte = ftell(fd);
	fseek(fd,0,SEEK_END);
	size_t last_byte = ftell(fd);
	fseek(fd,0,curr_byte);
	return last_byte;
}


void printFile(FILE* fd){
	char line[512];
	int seek = ftell(fd);

	fseek(fd,0,SEEK_SET);
	OP_TEE_DEBUG printf("printing file data:\n");
	if(fd != NULL){
		while(fgets(line,512,fd)){
			printf("%s\n", line);
		}
	}
	else
		printf("fd is null.\n" );

	fseek(fd,seek,SEEK_SET);
}

static int etoi(char* command){
	OP_TEE_DEBUG printf("command: %s\n", command);
	if(strcmp(command,SCPS_INIT) == 0)
		return 1;
	if(strcmp(command,SCPS_PROTECT) == 0)
		return 2;
	if(strcmp(command,SCPS_VIEW) == 0)
		return 3;

	printf("No command '%s' found, did you mean:\n",command );
	printf("CPS_INIT\nCPS_PROTECT\nCPS_VIEW\nClosing application." );
	return CPS_INVALID_OPERATION;
}
