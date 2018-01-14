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

#define STR_TRACE_USER_TA "HELLO_WORLD"

#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>
#include <string.h>
#include "hello_world_ta.h"


TEE_ObjectHandle p_storage_obj;
TEE_ObjectHandle key;
TEE_Result res;
TEE_ObjectHandle object;	//persistent object holding the enc key
TEE_OperationHandle handle = TEE_HANDLE_NULL;
TEE_ObjectInfo key_info = {0};
char buf[512] = {0};			// in buff
char* keyBuf;
char out[512] = {0};			// out buff
char iv[16] = {0};				//instruction vector
int iv_len = sizeof(iv);

static int ALREADY_SET_PASSWORD = 0;
static int isPasswordSet(void);
/*
 * Called when the instance of the TA is created. This is the first call in
 * the TA.
 */
TEE_Result TA_CreateEntryPoint(void)
{
	DMSG("has been called");

	return TEE_SUCCESS;
}

/*
 * Called when the instance of the TA is destroyed if the TA has not
 * crashed or panicked. This is the last call in the TA.
 */
void TA_DestroyEntryPoint(void)
{
	DMSG("has been called");
}

/*
 * Called when a new session is opened to the TA. *sess_ctx can be updated
 * with a value to be able to identify this session in subsequent calls to the
 * TA. In this function you will normally do the global initialization for the
 * TA.
 */
TEE_Result TA_OpenSessionEntryPoint(uint32_t param_types,
		TEE_Param __maybe_unused params[4],
		void __maybe_unused **sess_ctx)
{
	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE,
							 TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE);

	DMSG("has been called");

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

	/* Unused parameters */
	(void)&params;
	(void)&sess_ctx;

	/*
	 * The DMSG() macro is non-standard, TEE Internal API doesn't
	 * specify any means to logging from a TA.
	 */
	//IMSG("Hellooooooooooo World!\n");

	/* If return value != TEE_SUCCESS the session will not be created. */
	return TEE_SUCCESS;
}

/*
 * Called when a session is closed, sess_ctx hold the value that was
 * assigned by TA_OpenSessionEntryPoint().
 */
void TA_CloseSessionEntryPoint(void __maybe_unused *sess_ctx)
{
	(void)&sess_ctx; /* Unused parameter */
	IMSG("Goooooooooooooodbye!\n");
}


static TEE_Result dec(uint32_t param_types, TEE_Param params[4]) {
	param_types = param_types;
	params = params;

	return TEE_SUCCESS;
}

static TEE_Result setPassword(uint32_t param_types, TEE_Param params[4]) {
	param_types = param_types;
	params = params;

	return TEE_SUCCESS;
}

static TEE_Result show(uint32_t param_types, TEE_Param params[4]){
	param_types = param_types;
	params = params;


	return TEE_SUCCESS;
}

// static int myStrlen(const char* m)
// {
// 	int c;
// 	for (c = 0; *m; ++c, ++m);
// 	return c;
// }

// static void myMemcpy(void* dest, void* src, unsigned len)
// {
// 	for (unsigned i = 0; i < len; ++i)
// 		((char*)dest)[i] = ((char*)src)[i];
// }

static void myMemset(void* dest, char b, unsigned len)
{
	for (unsigned i = 0; i < len; ++i)
		((char*)dest)[i] = b;
}

static TEE_Result hashBufferWithSHA1(
	int keyBuflen,
	char* hashedKeyBuf,
	uint32_t* hashedKeyLen){
		TEE_Result ret;
		TEE_OperationHandle hashedHandle = TEE_HANDLE_NULL;

		ret = TEE_AllocateOperation(&hashedHandle, TEE_ALG_SHA1, TEE_MODE_DIGEST, 0);
		if (ret != TEE_SUCCESS) {
	        IMSG("TEE_AllocateOperation returned (%08x).",ret);
	        TEE_FreeOperation(hashedHandle);
	        return ret;
	  }

		IMSG("BEFORE DIGEST DO FINAL");
		for(int i=0 ; i < keyBuflen ; i++){
			IMSG("keyBuf[i] = %02x",keyBuf[i]);
		}
		//TEE_DigestUpdate(hashedHandle,keyBuf,keyBuflen);
		ret = TEE_DigestDoFinal(hashedHandle,keyBuf,keyBuflen,hashedKeyBuf,hashedKeyLen);
	  if(ret != TEE_SUCCESS) {
			IMSG("TEE_DigestDoFinal returned (%08x).",ret);
			TEE_FreeOperation(hashedHandle);
			return(ret);
		}
		TEE_FreeOperation(hashedHandle);

		return TEE_SUCCESS;
	}

static int hashUserKey(uint32_t param_types, TEE_Param params[4])
{
	TEE_Result ret;
	char* hashedKeyBuf;
	char objID[] = "96c5d1b260aa4de30fedaf67e5b9227613abebff172a2b4e949994b8e561e2fb";
	size_t objID_len = 64;
	uint32_t flags = TEE_DATA_FLAG_ACCESS_WRITE_META | TEE_DATA_FLAG_ACCESS_READ | TEE_DATA_FLAG_ACCESS_WRITE;
	uint32_t read_bytes = 0,hashedKeyLen;
	TEE_ObjectInfo info;
	int status = 1, len,strLen;

	void* p;
	params = params;
	strLen = 0; strLen = strLen;
	param_types = param_types;

	IMSG("params[1].memref.size= %d",params[1].memref.size);
	keyBuf = TEE_Malloc(params[1].memref.size,0);
	TEE_MemMove(keyBuf,(char*)(params[1].memref.buffer),params[1].memref.size);
	strLen = params[1].memref.size;
	hashedKeyLen=20;
	hashedKeyBuf = TEE_Malloc(hashedKeyLen, 0);

	//TODO: Digest the buffer string here so it will be inserted to the persistent object encrypted already.
	//start hash
	hashBufferWithSHA1(strLen,hashedKeyBuf,&hashedKeyLen);

	for (uint32_t i = 0; i < hashedKeyLen; ++i){
		IMSG("i=%d , hashedKeyBuf[i]=%02x ",i, hashedKeyBuf[i]);
		IMSG("\n");
	}

	//end hash


	//allocate operation for hashing with mode digest, size of 128
	// ret = TEE_AllocateOperation(&handle, TEE_ALG_AES_CBC_NOPAD, TEE_MODE_DIGEST, 128);
	// if (ret != TEE_SUCCESS) {
  //       IMSG("TEE_AllocateOperation returned (%08x).",ret);
  //       TEE_FreeOperation(handle);
  //       return ret;
  // }
  //
	// TEE_DigestUpdate(handle,keyBuf,strLen);
	// ret = TEE_DigestDoFinal(handle,keyBuf,strLen,hashedKeyBuf,&hashKeyLen);
  // if(ret != TEE_SUCCESS) {
	// 	TEE_FreeOperation(handle);
	// 	return(ret);
	// }
	//end hash

	IMSG("hashUserKey!!!\n");
	//myMemcpy(buf, &strLen, sizeof(strLen));
	//myMemcpy(buf + sizeof(int), (char*)buf, strLen);

	IMSG("Pre TEE_CreatePersistentObject\n");
	ret = TEE_CreatePersistentObject(TEE_STORAGE_PRIVATE, (void *)objID, objID_len,
									TEE_DATA_FLAG_ACCESS_WRITE_META,
									(TEE_ObjectHandle)NULL, keyBuf, 16,
									(TEE_ObjectHandle *)NULL);

	if (ret != TEE_SUCCESS)
	{
		EMSG("TEE_CreatePersistentObject failed.\n");
		goto done;
	}
	ret = TEE_OpenPersistentObject(TEE_STORAGE_PRIVATE,
							       (void *)objID, objID_len, flags, &object);
	if (ret != TEE_SUCCESS)
	{
		EMSG("TEE_OpenPersistentObject failed.\n");
		goto done;
	}
	TEE_GetObjectInfo(object, &info);
	ret = TEE_ReadObjectData(object, &len, sizeof(len), &read_bytes);
	if (ret != TEE_SUCCESS)
	{
		EMSG("TEE_ReadObjectData1 failed.\n");
		goto done;
	}
	IMSG("Got %d bytes of data:%d\n", read_bytes, len);
	p = TEE_Malloc(len + 1, 0);
	if (p == NULL)
		goto done;
	myMemset(p, 0, len);
	ret = TEE_ReadObjectData(object, p, len, &read_bytes);
	if (ret != TEE_SUCCESS)
	{
		EMSG("TEE_ReadObjectData2 failed.\n");
		goto done;
	}
	EMSG("Got %d bytes of data:%s\n", read_bytes, (char*)p);
	status = 0;
done:
	// TEE_CloseAndDeletePersistentObject(object);
	// IMSG("Persistent Object has been delete.\n");
	return status;
}

static TEE_Result enc(uint32_t param_types, TEE_Param params[4]) {

	uint32_t sz = sizeof(out);
	uint32_t i;

	param_types = param_types;
	params = params;

	// try to print that raw data
	IMSG("i received:%s\n",(char*)(params[1].memref.buffer));

	//change buffer content
	TEE_MemMove(buf,(char*)(params[1].memref.buffer),params[1].memref.size);

	//now we need to allocate shared memory to write to the client!!
	param_types = param_types;
	params = params;
	buf[0] = buf[0];


	IMSG("TEE_AllocateTransientObject\n");
	res = TEE_AllocateTransientObject(TEE_TYPE_AES, 128, &key);
	if (res != TEE_SUCCESS) {
        IMSG("TEE_AllocateTransientObject returned (%08x).",res);
        return res;
    }

	IMSG("TEE_GenerateKey\n");
	res = TEE_GenerateKey(key, 128, NULL,0);
	if (res != TEE_SUCCESS) {
        IMSG("TEE_GenerateKey returned (%08x).",res);
        return res;
  }

	IMSG("TEE_GetObjectInfo\n");
    TEE_GetObjectInfo(key, &key_info);

	//sanity print
	IMSG("%s: max_object_size = \n (%08x) object_size = (%08x).",
            __func__, key_info.maxObjectSize, key_info.objectSize);


	IMSG("TEE_AllocateOperation\n");
	res = TEE_AllocateOperation(&handle, TEE_ALG_AES_CBC_NOPAD, TEE_MODE_ENCRYPT, 128);
	if (res != TEE_SUCCESS) {
        IMSG("TEE_AllocateOperation returned (%08x).",res);
        TEE_FreeTransientObject(key);
        return res;
  }

  //sanity print
  IMSG("AES operation allocated\n");

	IMSG("TEE_SetOperationKey\n");
	res = TEE_SetOperationKey(handle,key);
	if (res != TEE_SUCCESS) {
		IMSG("TEE_SetOperationKey returned (%08x).",res);
		return res;
  }

  //sanity check
	IMSG("key operation key set!\n");

	IMSG("TEE_CipherInit\n");
	//encrypt the data
	TEE_CipherInit(handle,iv,iv_len);
	if (res != TEE_SUCCESS) {
	    IMSG("TEE_CipherInit returned (%08x).",res);
	    return res;
	}
	IMSG("AES initialized\n");

	sz = sizeof(out);

	IMSG("TEE_CipherUpdate START\n");
	res = TEE_CipherUpdate(handle,buf, sizeof(buf), out, &sz);
	IMSG("TEE_CipherUpdate DONE\n");

	IMSG("PRINTING ENCRYPTED DATA:\n");
	for (i=0; i<sz; i++) {
		IMSG("i = %d ,data=%x, ",i,out[i]);
	}

	//TODO: send the information to the back to the NT

	TEE_FreeOperation(handle);
	TEE_FreeTransientObject(key);

	return TEE_SUCCESS;
}


/*
 * Called when a TA is invoked. sess_ctx hold that value that was
 * assigned by TA_OpenSessionEntryPoint(). The rest of the paramters
 * comes from normal world.
 */

static int isPasswordSet(void){
	if(ALREADY_SET_PASSWORD > 0){
		return 1;
	}
	else return 0;
}

TEE_Result TA_InvokeCommandEntryPoint(void __maybe_unused *sess_ctx,
			uint32_t cmd_id,
			uint32_t param_types, TEE_Param params[4]) {

	(void)&sess_ctx; /* Unused parameter */



	switch (cmd_id) {
		case 1: //CRYPT_SET_PASS:

			IMSG("is password set? : %d",ALREADY_SET_PASSWORD);
			if(isPasswordSet()){
				return TEE_SUCCESS;
				IMSG("Key already set\n");
			}

			ALREADY_SET_PASSWORD = 1;
			hashUserKey(param_types, params);
			IMSG("OPERATION: Set password | buffer: \n");
			for (uint32_t i = 0; i < params[1].memref.size; i++){
				IMSG("i=%d , keyBuf[i]:%02x ", i,keyBuf[i]);
				IMSG("\n");
			}


			return setPassword(param_types, params);
		case 2: //CRYPT_ENCRYPT:
			IMSG("OPERATION: encrypt | buffer: %s\n",buf);
			return enc(param_types, params);
		case 3: //CRYPT_DECRYPT:
			IMSG("OPERATION: decrypt | buffer: %s\n",buf);
			return dec(param_types, params);
		case 4: //CRYPT_SHOW:
			IMSG("OPERATION: show() | buffer: %s\n",buf);
			return show(param_types, params);
		default:
			IMSG("OPERATION: TEE_ERROR_BAD_PARAMETERS | buffer: %s\n",buf);
			return TEE_ERROR_BAD_PARAMETERS;
	}

	TEE_CloseAndDeletePersistentObject(object);
	IMSG("Persistent Object has been delete.\n");
	return TEE_SUCCESS;
}