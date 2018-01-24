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

#define UINT_BLOCK_SIZE		(16)
#define LIGHT_FILE_MODE 	(1)
#define HEAVY_FILE_MODE 	(2)
#define CHUNK_SIZE				(16)

static FILE* fd = NULL;
static const char* lightFilePath = "lightFile";
static const char* heavyFilePath = "heavyFile";

static void validateArgs(int argc);
static void show();
static FILE* openFile(const char* filePath,const char* mode);
static int closeFile(FILE* fd);
static void fillFilesWithData();
static char* allocateBuf(char* buf,char* argv ,size_t size);

static char* test = "Both wrap() and fill() work by creating a TextWrapper instance and calling a single method on it. That instance is not reused, so for applications that wrap/fill many text strings, it will be more efficient for you to create your own TextWrapper object. 	DATA DATA DATA DATA DATA DATA DATA DATA DATA DATA DATA DATA DATA DATA  ote that tabs and spaces are both treated as whitespace, but they are not equal: the lines are considered to have no common leading whitespace. (This behaviour is new in Python 2.5; older versions of this module incorrectly expanded tabs before searching for common leading whitespace.) Both wrap() and fill() work by creating a TextWrapper instance and calling a single method on it. That instance is not reused, so for applications that wrap/fill many text strings, it will be more efficient for you to create your own TextWrapper objectBoth wrap() and fill() work by creating a TextWrapper instance and calling a single method on it. That instance is not reused, so for applications that wrap/fill many text strings, it will be more efficient for you to create your own TextWrapper object. 	DATA DATA DATA DATA DATA DATA DATA DATA DATA DATA DATA DATA DATA DATA  ote that tabs and spaces are both treated as whitespace, but they are not equal: the lines are considered to have no common leading whitespace. (This behaviour is new in Python 2.5; older versions of this module incorrectly expanded tabs before searching for common leading whitespace.) Both wrap() and fill() work by creating a TextWrapper instance and calling a single method on it. That instance is not reused, so for applications that wrap/fill many text strings, it will be more efficient for you to create your own TextWrapper objectBoth wrap() and fill() work by creating a TextWrapper instance and calling a single method on it. That instance is not reused, so for applications that wrap/fill many text strings, it will be more efficient for you to create your own TextWrapper object. 	DATA DATA DATA DATA DATA DATA DATA DATA DATA DATA DATA DATA DATA DATA  ote that tabs and spaces are both treated as whitespace, but they are not equal: the lines are considered to have no common leading whitespace. (This behaviour is new in Python 2.5; older versions of this module incorrectly expanded tabs before searching for common leading whitespace.) Both wrap() and fill() work by creating a TextWrapper instance and calling a single method on it. That instance is not reused, so for applications that wrap/fill many text strings, it will be more efficient for you to create your own TextWrapper object\n";


int main(int argc, char *argv[])
{
	char line[512];
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

	// injecting some random data to local env files to later be enc/dec
	fillFilesWithData();

	// while(fixed_block_size < strlen(argv[2])+1){
	// 	fixed_block_size += UINT_BLOCK_SIZE;
	// }


	buf = allocateBuf(buf,argv[2],CHUNK_SIZE);
	if(NULL == buf)
		return -1;


	switch (atoi(argv[1])) {
		case 1:	//set password
			COMMAND_TYPE = 1;
			printf("case : %s\n",argv[2]);
			break;

		case 2: //pass a msg
			COMMAND_TYPE = 2;
			printf("case : %s\n",argv[2]);
			fd = openFile(lightFilePath,"a+");

			//sanity print file
			if(fd != NULL){
				while(fgets(line,512,fd)){
					printf("%s\n", line);
				}
		  }

			//writeEncDataToFile(fd,readBytes,seekStart,buf);
			closeFile(fd);

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
	shared_mem.size = sizeof(buf);
	shared_mem.flags = TEEC_MEM_INPUT;

	printf("buf=%s , shared_mem.buffer = %s\nshared_mem.size = %lu",(char*)buf,(char*)shared_mem.buffer,shared_mem.size );

	/* Initialize a context connecting us to the TEE */
	res = TEEC_InitializeContext(NULL, &ctx);
	if ( res != TEEC_SUCCESS)
		errx(1, "TEEC_InitializeContext failed with code 0x%x", res);

	res = TEEC_OpenSession(&ctx, &sess, &uuid, TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);
	if(res != TEEC_SUCCESS)
		errx(1, "TEEC_Opensession failed with code 0x%x origin 0x%x", res, err_origin);

	/* Clear the TEEC_Operation struct */
	memset(&op, 0, sizeof(op));

	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INOUT, TEEC_MEMREF_WHOLE, TEEC_NONE, TEEC_NONE);
	//init memref_whole parameter
	op.params[1].memref.parent = &shared_mem;
	op.params[1].memref.size = shared_mem.size;
	op.params[1].memref.offset = 0;

	TEEC_RegisterSharedMemory(&ctx,&shared_mem);

	res = TEEC_InvokeCommand(&sess, COMMAND_TYPE, &op, &err_origin);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x", res, err_origin);

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

static FILE* openFile(const char* filePath,const char* mode){

	printf("opening file %s , with mode: %s\n",filePath,mode);
	fd = fopen(filePath,mode);
	if(fd != NULL)
		printf("file opened successfully.\n");
	else
		printf("failed to open file.\n");
	return fd;
}

static int closeFile(FILE* fd){
	int ret = -1;
	printf("closing file.\n");
	if(fd != NULL)
		ret = fclose(fd);
	if(ret == 0)
		printf("successfully closed file.\n");
	else
		printf("failed to close file.\n");
	return ret;
}

static void show(){
	printf("show() called\n");
	return;
}

static int isFileEmpty(){
	int size;
	fseek (fd, 0, SEEK_END);
	size = ftell(fd);
	if (0 == size) {
		printf("file is empty\n");
		return 1;
	}
	return 0;
}

static void fillFilesWithData(){
	fd = openFile(lightFilePath,"a+");
	if(1 == isFileEmpty(fd)){
		printf("filling file '%s' with random data\n",lightFilePath );
		fprintf(fd,"%s",test);
	}
	closeFile(fd);

	fd = openFile(heavyFilePath,"a+");
	if(1 == isFileEmpty(fd)){
		printf("filling file '%s' with random data\n",heavyFilePath );
		fprintf(fd,"%s",test);
	}
	closeFile(fd);
}

static char* allocateBuf(char* buf,char* argv ,size_t size) {
		printf("%s called. argv = %s , size = %lu\n",__FUNCTION__,argv,size);
		buf = (char*)malloc(size);
		if(buf == NULL){
			printf("failed to allocate 'buf'\n");
		}
		else{
			memset(buf,0,size);
			strncpy(buf,argv,strlen(argv));
		}
		printf("buf=%s\n",buf );
		return buf;
}
