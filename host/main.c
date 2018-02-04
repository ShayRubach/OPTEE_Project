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
#define INVALID_ARGS			(-1)
#define INPUT_ERROR				(-1)


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

static int 		validateArgs(int argc);														//check for invalid args count
static int 		etoi(char* command);															// enum to int
static int 		closeFile(FILE* fd);															// safe close file
static void 	fillFilesWithData();															// local data for files
static FILE* 	openFile(const char* filePath,const char* mode);	// safe open file
static size_t getLastByteFromFile(FILE* fd);										// get last byte of file
static void 	printFile(FILE* fd);

//those 2 data strings are to be inserted into the local files, for testings:
static char* heavy_data = "Both wrap() and fill() work by creating a TextWrapper instance and calling a single method on it. That instance is not reused, so for applications that wrap/fill many text strings, it will be more efficient for you to create your own TextWrapper object. 	DATA DATA DATA DATA DATA DATA DATA DATA DATA DATA DATA DATA DATA DATA  ote that tabs and spaces are both treated as whitespace, but they are not equal: the lines are considered to have no common leading whitespace. (This behaviour is new in Python 2.5; older versions of this module incorrectly expanded tabs before searching for common leading whitespace.) Both wrap() and fill() work by creating a TextWrapper instance and calling a single method on it. That instance is not reused, so for applications that wrap/fill many text strings, it will be more efficient for you to create your own TextWrapper objectBoth wrap() and fill() work by creating a TextWrapper instance and calling a single method on it. That instance is not reused, so for applications that wrap/fill many text strings, it will be more efficient for you to create your own TextWrapper object. 	DATA DATA DATA DATA DATA DATA DATA DATA DATA DATA DATA DATA DATA DATA  ote that tabs and spaces are both treated as whitespace, but they are not equal: the lines are considered to have no common leading whitespace. (This behaviour is new in Python 2.5; older versions of this module incorrectly expanded tabs before searching for common leading whitespace.) Both wrap() and fill() work by creating a TextWrapper instance and calling a single method on it. That instance is not reused, so for applications that wrap/fill many text strings, it will be more efficient for you to create your own TextWrapper objectBoth wrap() and fill() work by creating a TextWrapper instance and calling a single method on it. That instance is not reused, so for applications that wrap/fill many text strings, it will be more efficient for you to create your own TextWrapper object. 	DATA DATA DATA DATA DATA DATA DATA DATA DATA DATA DATA DATA DATA DATA  ote that tabs and spaces are both treated as whitespace, but they are not equal: the lines are considered to have no common leading whitespace. (This behaviour is new in Python 2.5; older versions of this module incorrectly expanded tabs before searching for common leading whitespace.) Both wrap() and fill() work by creating a TextWrapper instance and calling a single method on it. That instance is not reused, so for applications that wrap/fill many text strings, it will be more efficient for you to create your own TextWrapper object\n";

static char* light_data = "Uncountable nouns are nouns that are either difficult or impossible to count. Uncountable nouns include intangible things (e.g., information, air), liquids (e.g., milk, wine), and things that are too large or numerous to count (e.g., equipment, sand, wood). Because these things can’t be counted, you should never use a or an with them—remember, the indefinite article is only for singular nouns. Uncountable nouns can be modified by words like some, however. Consider the examples below for reference. Water is an uncountable noun and should not be used with the indefinite article.However, if you describe the water in terms of countable units (like bottles), you can use the indefinite article.\n";


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

	// validate input
	if(INVALID_ARGS == (validateArgs(argc)))
		return INPUT_ERROR;
	if(CPS_INVALID_OPERATION == (	CPS = etoi(argv[1])))
		return INPUT_ERROR;

	fillFilesWithData();


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

	printf("TEEC_InitializeContext called.\n");
	res = TEEC_InitializeContext(NULL, &ctx);
	if ( res != TEEC_SUCCESS)
		errx(1, "TEEC_InitializeContext failed with code 0x%x", res);

	printf("TEEC_OpenSession called.\n");
	res = TEEC_OpenSession(&ctx, &sess, &uuid, TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);
	if(res != TEEC_SUCCESS)
		errx(1, "TEEC_Opensession failed with code 0x%x origin 0x%x", res, err_origin);


	printf("TEEC_RegisterSharedMemory called.\n");
	TEEC_RegisterSharedMemory(&ctx,&shared_mem);

	printf("CPS: %d\n",CPS);
	switch (CPS) {
		case CPS_INIT:	//set password
			printf("case : %s\n",argv[1]);

			memset(shared_mem.buffer,0,CHUNK_SIZE);
			strncpy(shared_mem.buffer,argv[2],strlen(argv[2]));

			printf("TEEC_InvokeCommand called.\n");
			res = TEEC_InvokeCommand(&sess, CPS, &op, &err_origin);
			if (res != TEEC_SUCCESS)
					errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x", res, err_origin);
			break;

		case CPS_VIEW:	//invoke show()
			printf("case : %s\n",argv[1]);
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

		case CPS_PROTECT: //pass a msg

			printf("case : %s\n",argv[1]);
			if(NULL == (fd = openFile(lightFilePath,"r+")))
				return -1;

			printFile(fd);
			size_t lastByte = getLastByteFromFile(fd);

			while(lastByte != ftell(fd)){
				memset(shared_mem.buffer,0,CHUNK_SIZE);
				readBytes = fread(shared_mem.buffer,1,CHUNK_SIZE,fd);
				fseek(fd,readBytes*(-1),SEEK_CUR);

				printf("BEFORE INVOKE: readBytes: %lu, shared_mem.buffer: %s\n",readBytes,(char*)shared_mem.buffer );

				printf("About to InvokeCommand with CPS=%d\n",CPS );
				res = TEEC_InvokeCommand(&sess, CPS, &op, &err_origin);

				if (res != TEEC_SUCCESS)
					errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x", res, err_origin);

				printf("AFTER INVOKE: shared_mem.buffer: %s\n",(char*)shared_mem.buffer);

				fwrite(shared_mem.buffer,1,readBytes,fd);

			}

			closeFile(fd);
			break;

		default:
			printf("default case\n");
			break;
	}


	if(NULL != shared_mem.buffer)
		free(shared_mem.buffer);


	TEEC_ReleaseSharedMemory(&shared_mem);
	TEEC_CloseSession(&sess);
	TEEC_FinalizeContext(&ctx);

	return 0;
}


static int validateArgs(int argc){
	int res = 1;
	if(argc != 3){
		printf("ERROR: invalid number of arguments.\n");
		res = -1;
	}
	return res;
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

static size_t getLastByteFromFile(FILE* fd){
	fseek(fd,0,SEEK_END);
	size_t lastByte = ftell(fd);
	fseek(fd,0,SEEK_SET);
	return lastByte;
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
	fd = openFile(lightFilePath,"a");
	if(1 == isFileEmpty(fd)){
		printf("filling file '%s' with random data\n",lightFilePath );
		fprintf(fd,"%s",light_data);
	}

	closeFile(fd);

	fd = openFile(heavyFilePath,"a");
	if(1 == isFileEmpty(fd)){
		printf("filling file '%s' with random data\n",heavyFilePath );
		fprintf(fd,"%s",heavy_data);
	}
	closeFile(fd);
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
