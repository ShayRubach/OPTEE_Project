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

#define UINT_BLOCK_SIZE		(16)
#define LIGHT_FILE_MODE 	(1)
#define HEAVY_FILE_MODE 	(2)
#define CHUNK_SIZE				(16)
#define SCPS_INIT					"CPS_INIT"
#define SCPS_PROTECT			"CPS_PROTECT"
#define SCPS_VIEW					"CPS_VIEW"
#define SCPS_VIEW_RAW			"raw"
#define SCPS_VIEW_ASCII		"ascii"



static FILE* fd = NULL;
static const char* lightFilePath = "lightFile";
static const char* heavyFilePath = "heavyFile";




enum CPS_TYPE {
	CPS_INIT = 1,
	CPS_PROTECT,
	CPS_VIEW,
	CPS_VIEW_RAW,
	CPS_VIEW_ASCII,
	CPS_INVALID_OPERATION
};

//static void validateArgs(const char* argv,int argc);
static void show();
static FILE* openFile(const char* filePath,const char* mode);
static int closeFile(FILE* fd);
static void fillFilesWithData();
static char* allocateBuf(char* buf,size_t size);

static char* test = "Both wrap() and fill() work by creating a TextWrapper instance and calling a single method on it. That instance is not reused, so for applications that wrap/fill many text strings, it will be more efficient for you to create your own TextWrapper object. 	DATA DATA DATA DATA DATA DATA DATA DATA DATA DATA DATA DATA DATA DATA  ote that tabs and spaces are both treated as whitespace, but they are not equal: the lines are considered to have no common leading whitespace. (This behaviour is new in Python 2.5; older versions of this module incorrectly expanded tabs before searching for common leading whitespace.) Both wrap() and fill() work by creating a TextWrapper instance and calling a single method on it. That instance is not reused, so for applications that wrap/fill many text strings, it will be more efficient for you to create your own TextWrapper objectBoth wrap() and fill() work by creating a TextWrapper instance and calling a single method on it. That instance is not reused, so for applications that wrap/fill many text strings, it will be more efficient for you to create your own TextWrapper object. 	DATA DATA DATA DATA DATA DATA DATA DATA DATA DATA DATA DATA DATA DATA  ote that tabs and spaces are both treated as whitespace, but they are not equal: the lines are considered to have no common leading whitespace. (This behaviour is new in Python 2.5; older versions of this module incorrectly expanded tabs before searching for common leading whitespace.) Both wrap() and fill() work by creating a TextWrapper instance and calling a single method on it. That instance is not reused, so for applications that wrap/fill many text strings, it will be more efficient for you to create your own TextWrapper objectBoth wrap() and fill() work by creating a TextWrapper instance and calling a single method on it. That instance is not reused, so for applications that wrap/fill many text strings, it will be more efficient for you to create your own TextWrapper object. 	DATA DATA DATA DATA DATA DATA DATA DATA DATA DATA DATA DATA DATA DATA  ote that tabs and spaces are both treated as whitespace, but they are not equal: the lines are considered to have no common leading whitespace. (This behaviour is new in Python 2.5; older versions of this module incorrectly expanded tabs before searching for common leading whitespace.) Both wrap() and fill() work by creating a TextWrapper instance and calling a single method on it. That instance is not reused, so for applications that wrap/fill many text strings, it will be more efficient for you to create your own TextWrapper object\n";

//@bad logic
// static void validateArgs(const char* argv, int argc){
// 	if( ((strcmp(argv,SCPS_INIT) == 0) && argc != 3 )
// 		||((strcmp(argv,SCPS_VIEW) == 0) && argc != 3 )
// 		|| argc != 2){
// 		printf("ERROR: invalid number of arguments.\n");
// 		exit(-1);
// 	}
// }

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
	fd = openFile(lightFilePath,"w+");
	if(1 == isFileEmpty(fd)){
		printf("filling file '%s' with random data\n",lightFilePath );
		fprintf(fd,"%s",test);
	}
	closeFile(fd);

	fd = openFile(heavyFilePath,"w+");
	if(1 == isFileEmpty(fd)){
		printf("filling file '%s' with random data\n",heavyFilePath );
		fprintf(fd,"%s",test);
	}
	closeFile(fd);
}

static char* allocateBuf(char* buf,size_t size) {
		buf = (char*)malloc(size);
		if(buf == NULL){
			printf("failed to allocate 'buf'\n");
		}
		else{
			memset(buf,0,size);
		}

		return buf;
}

void printFile(FILE* fd){
	char line[512];
	int seek = ftell(fd);

	fseek(fd,0,SEEK_SET);
	printf("printing file data:\n");
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
	printf("command: %s\n", command);
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
	enum CPS_TYPE CPS;

	//validateArgs(argv[1],argc);

	CPS = etoi(argv[1]);
	if(CPS_INVALID_OPERATION == CPS)
		return -1;

	fillFilesWithData();

	buf = allocateBuf(buf,CHUNK_SIZE);
	if(NULL == buf)
		return -1;

	printf("CPS: %d\n",CPS);
	switch (CPS) {
		case CPS_INIT:	//set password
			strncpy(buf,argv[2],strlen(argv[2]));
			printf("case : %s\n",argv[1]);
			break;

		case CPS_PROTECT: //pass a msg

			printf("case : %s\n",argv[1]);
			break;

		case CPS_VIEW:	//invoke show()
			printf("case : %s\n",argv[1]);
			if(strcmp(argv[2],SCPS_VIEW_RAW) == 0){
				CPS = CPS_VIEW_RAW;
				show();
			}
			else if(strcmp(argv[2],SCPS_VIEW_ASCII) == 0){
				CPS = CPS_VIEW_ASCII;
				show();
			}
			else
				return -1;

			break;
		default:
			printf("default case\n");
			break;
	}

	shared_mem.buffer = buf;
	shared_mem.size = CHUNK_SIZE;
	shared_mem.flags = TEEC_MEM_INPUT;
	printf("buf=%s | shared_mem.buffer = %s | shared_mem.size = %lu\n",(char*)buf,(char*)shared_mem.buffer,shared_mem.size );

	printf("TEEC_InitializeContext called.\n");
	res = TEEC_InitializeContext(NULL, &ctx);
	if ( res != TEEC_SUCCESS)
		errx(1, "TEEC_InitializeContext failed with code 0x%x", res);

	printf("TEEC_OpenSession called.\n");
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

	printf("TEEC_RegisterSharedMemory called.\n");
	TEEC_RegisterSharedMemory(&ctx,&shared_mem);

	size_t readBytes = 0;
	switch(CPS){
		printf("entered switch 2 with CPS=%d\n",CPS );
		case CPS_PROTECT:


		fd = openFile(lightFilePath,"r+");
		fseek(fd,0,SEEK_SET);

		if(NULL == fd)
			return -1;

		readBytes=0;
		char in_buf[CHUNK_SIZE] = {0};

		// while(!feof(fd)){
		// 	readBytes = fread(in_buf,1,CHUNK_SIZE,fd);
		// 	memset(in_buf,0,readBytes);
		// 	printf("readBytes: %lu\n",readBytes );
		// }
    //

		// while(!feof(fd)){
		// 	readBytes = fread(shared_mem.buffer,1,CHUNK_SIZE,fd);
		// 	memset(shared_mem.buffer,0,readBytes);
		// 	printf("%s ",(char*)shared_mem.buffer );
		// 	printf("readBytes: %lu\n",readBytes );
		// }




		//works
		// int c;
		// while((c = fgetc(fd)) != EOF){
		// 	printf("%c",c );
		// }

		while(!feof(fd)){


			readBytes = fread(shared_mem.buffer,1,CHUNK_SIZE,fd);

			printf("ftell(fd) 2 =%lu\n",ftell(fd));
			printf("shared_mem.buffer after fread(): %s\n",(char*)shared_mem.buffer);
			printf("fread(): read %lu bytes from file.\n",readBytes);
			printf("seeking back..\n");
			fseek(fd,readBytes*(-1),SEEK_CUR);

			//fseek(fd, CHUNK_SIZE*(-1) ,SEEK_CUR);

			printf("ftell(fd) 3 =%lu\n",ftell(fd));

			res = TEEC_InvokeCommand(&sess, CPS, &op, &err_origin);
			if (res != TEEC_SUCCESS)
				errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x", res, err_origin);

			// for (size_t i = 0; i < shared_mem.size; i++) {
			// 	printf("%x__",((char*)shared_mem.buffer)[i]);
			// }

			printf("shared buf:%s\n shared size:%lu\n strlen shared buff:%lu\n",(char*)shared_mem.buffer,shared_mem.size,strlen((char*)shared_mem.buffer) );
			strncpy(in_buf,shared_mem.buffer,readBytes);


			fwrite(in_buf,1,readBytes,fd);
			memset(shared_mem.buffer,0,CHUNK_SIZE);

			printf("ftell(fd) 6      = %lu\n",ftell(fd));

		}

		closeFile(fd);

		break;

		case CPS_VIEW_RAW:
		break;

		case CPS_VIEW_ASCII:
		break;

		case CPS_INIT:
		printf("TEEC_InvokeCommand called.\n");
		res = TEEC_InvokeCommand(&sess, CPS, &op, &err_origin);
		if (res != TEEC_SUCCESS)
			errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x", res, err_origin);

		default:
			break;
	}

	if(NULL != buf)
		free(buf);

	TEEC_ReleaseSharedMemory(&shared_mem);
	TEEC_CloseSession(&sess);
	TEEC_FinalizeContext(&ctx);

	return 0;
}
