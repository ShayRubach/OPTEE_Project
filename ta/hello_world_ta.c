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
#define HASH_KEY_SIZE 16
#define CREATE_AND_OPEN 1
#define OPEN_ONLY				2
#define FILE_PATH 			"96c5d1b260aa4de30fedaf67e5b9227613abebff172a2b4e949994b8e561e2fb"
TEE_ObjectHandle p_storage_obj;
TEE_ObjectHandle key;
TEE_ObjectHandle object;	//persistent object holding the enc key
TEE_OperationHandle handle = TEE_HANDLE_NULL;
TEE_Result res;
TEE_ObjectInfo key_info = {0};
char buf[512] = {0};			// in buff
char* keyBuf;
char out[512] = {0};			// out buff
char iv[16] = {0};				//instruction vector
int iv_len = sizeof(iv);

enum CPS_TYPE {
	CPS_INIT = 1,
	CPS_PROTECT,
	CPS_VIEW,
	CPS_VIEW_RAW,
	CPS_VIEW_ASCII,
};


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

		// IMSG("BEFORE DIGEST DO FINAL");
		// for(int i=0 ; i < keyBuflen ; i++){
		// 	IMSG("keyBuf[i] = %02x",keyBuf[i]);
		// }
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


static int createAndOpenObject(char* objID, size_t objID_len, uint32_t flags, char* data_buf, int size,int MODE)
	{
		TEE_Result ret;

		ret = TEE_OpenPersistentObject(TEE_STORAGE_PRIVATE, (void *)objID, objID_len, flags, &object);
		if (ret != TEE_SUCCESS && MODE == CREATE_AND_OPEN) {
			IMSG("Pre TEE_CreatePersistentObject\n");
			EMSG("TEE_OpenPersistentObject failed.\n");
			ret = TEE_CreatePersistentObject(TEE_STORAGE_PRIVATE,
											(void *)objID, objID_len,
											TEE_DATA_FLAG_ACCESS_WRITE_META,
											TEE_HANDLE_NULL, data_buf, size,
											(TEE_ObjectHandle *)NULL);
			EMSG("Pre TEE_CreatePersistentObject RET VALUE: %u",ret);
			ret = TEE_OpenPersistentObject(TEE_STORAGE_PRIVATE,
																		(void *)objID, objID_len,
																		flags, &object);
			EMSG("Pre TEE_OpenPersistentObject RET VALUE: %u",ret);
			return -1;
		}
		if (ret == TEE_SUCCESS && MODE == CREATE_AND_OPEN) {
			EMSG("Key already set!\n");
			return -2;
		}
		else if(ret != TEE_SUCCESS && MODE == OPEN_ONLY){
				EMSG("Generate a key first.\n");
				return 0;
		}
		else {
			EMSG("TEE_OpenPersistentObject succeed!.\n");
		}
		return 1;
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
	int status = 0, len = 0,strLen,result;

	char* p;
	len = len;
	params = params;
	strLen = 0; strLen = strLen;
	param_types = param_types;
	IMSG("Entered: hashUserKey \n");
	IMSG("params[1].memref.size= %d",params[1].memref.size);
	keyBuf = TEE_Malloc(params[1].memref.size,0);
	TEE_MemMove(keyBuf,(char*)(params[1].memref.buffer),params[1].memref.size);
	strLen = params[1].memref.size;
	hashedKeyLen=20;
	hashedKeyBuf = TEE_Malloc(hashedKeyLen, 0);

	hashBufferWithSHA1(strLen,hashedKeyBuf,&hashedKeyLen);

	result = createAndOpenObject(objID, objID_len, flags,hashedKeyBuf,16,CREATE_AND_OPEN);
	if(result != -1){
		status = -1;
		goto done;
	}

	TEE_GetObjectInfo(object, &info);
	p = TEE_Malloc(HASH_KEY_SIZE, 0);

	IMSG("TEE_ReadObjectData2 called.\n");
	ret = TEE_ReadObjectData(object, p, 16, &read_bytes);
	if (ret != TEE_SUCCESS) {
		EMSG("TEE_ReadObjectData2 failed.\n");
		goto done;
	}
	EMSG("Got %d bytes of data:%s\n", read_bytes, (char*)p);

	for (uint32_t i = 0; i < HASH_KEY_SIZE ; ++i){
		IMSG("HASHED KEY ====>  p[%d]=%02x ",i, p[i]);
	}

done:
	// TEE_CloseAndDeletePersistentObject(object);
	// IMSG("Persistent Object has been delete.\n");
	IMSG("Closing Persisten Object..\n");
	TEE_CloseObject(object);

	return status;
}


static TEE_Result setAttribute(TEE_Attribute* aesAttributes, uint32_t id,char* hashedKey,size_t keySize){
	//set attribute vals

	aesAttributes->attributeID = id;
	aesAttributes->content.ref.buffer = hashedKey;
	aesAttributes->content.ref.length = keySize;
	return TEE_SUCCESS;
}


static TEE_Result decryptWithPrivateKey(uint32_t param_types, TEE_Param params[4]) {

		TEE_Attribute aesAttributes;
		uint32_t i,read_bytes;
		uint32_t sz = 16;
		size_t hkbSize = 16;
		size_t objID_len = 64;
		char* hashedKeyBuf = TEE_Malloc(16, 0);
		char fn[] = "decryptWithPrivateKey";
		char objID[] = FILE_PATH;
		uint32_t flags = TEE_DATA_FLAG_ACCESS_WRITE_META | TEE_DATA_FLAG_ACCESS_READ | TEE_DATA_FLAG_ACCESS_WRITE;
		param_types = param_types;
		params = params;
		object = TEE_HANDLE_NULL;
		i =0;i=i;
		TEE_MemFill((char*)hashedKeyBuf,0,hkbSize);

		IMSG("%s: enc ascii\n",fn);
		for(i=0;i<16;++i){
			IMSG("%c , ",((char*)params[1].memref.buffer)[i]);
		}

		IMSG("%s: enc hex\n",fn);
		for(i=0;i<16;++i){
			IMSG("%x , ",((char*)params[1].memref.buffer)[i]);
		}

		IMSG("createAndOpenObject called.\n");
		if((createAndOpenObject(objID, objID_len, flags,NULL,16,OPEN_ONLY)) == -1){
			IMSG("Couldn't open object!");
			return TEE_ERROR_ITEM_NOT_FOUND;
		}

		IMSG("TEE_ReadObjectData called.\n");
		if((TEE_ReadObjectData(object, hashedKeyBuf, hkbSize, &read_bytes)) != TEE_SUCCESS) {
			EMSG("TEE_ReadObjectData failed.\n");
		}

		IMSG("setAttribute called.\n");
		setAttribute(&aesAttributes,TEE_ATTR_SECRET_VALUE,hashedKeyBuf,hkbSize);

		IMSG("TEE_AllocateTransientObject called.\n");
		res = TEE_AllocateTransientObject(TEE_TYPE_AES, 128, &key);
		if (res != TEE_SUCCESS) {
			IMSG("TEE_AllocateTransientObject returned (%08x).",res);
			return res;
		}

		IMSG("TEE_ResetTransientObject called.\n");
		TEE_ResetTransientObject(key);

		IMSG("TEE_PopulateTransientObject called.\n");
		TEE_PopulateTransientObject(key, (TEE_Attribute *)&aesAttributes, 1);

		IMSG("TEE_GetObjectInfo called.\n");
	  TEE_GetObjectInfo(key, &key_info);


		IMSG("TEE_AllocateOperation called.\n");
		if((TEE_AllocateOperation(&handle, TEE_ALG_AES_CBC_NOPAD, TEE_MODE_DECRYPT, 128)) != TEE_SUCCESS ) {
	        IMSG("TEE_AllocateOperation failed.\n");
	        TEE_FreeTransientObject(key);
	        return TEE_ERROR_ITEM_NOT_FOUND;
	  }

		IMSG("TEE_SetOperationKey called.\n");
		if((TEE_SetOperationKey(handle,key)) != TEE_SUCCESS) {
			IMSG("TEE_SetOperationKey failed.\n");
			return TEE_ERROR_ITEM_NOT_FOUND;
	  }

		IMSG("TEE_CipherInit called.\n");
		TEE_CipherInit(handle,iv,iv_len);

		//sz = sizeof(out);
		sz= 16;


		IMSG("TEE_CipherDoFinal called.\n");
		TEE_CipherDoFinal(handle,params[1].memref.buffer,16,params[1].memref.buffer,&(sz));



		IMSG("%s: dec ascii\n",fn);
		for(i=0;i<16;++i){
			IMSG("%c , ",((char*)params[1].memref.buffer)[i]);
		}

		IMSG("%s: dec hex\n",fn);
		for(i=0;i<16;++i){
			IMSG("%x , ",((char*)params[1].memref.buffer)[i]);
		}
		TEE_FreeOperation(handle);
		TEE_FreeTransientObject(key);
		TEE_CloseObject(object);

		return TEE_SUCCESS;
}

static TEE_Result encryptWithPrivateKey(uint32_t param_types, TEE_Param params[4]) {

	TEE_Attribute aesAttributes;
	uint32_t i,read_bytes;
	uint32_t sz = 16;
	size_t hkbSize = 16;
	size_t objID_len = 64;
	char* hashedKeyBuf = TEE_Malloc(16, 0);
	char fn[] = "encryptWithPrivateKey";
	char objID[] = FILE_PATH;
	uint32_t flags = TEE_DATA_FLAG_ACCESS_WRITE_META | TEE_DATA_FLAG_ACCESS_READ | TEE_DATA_FLAG_ACCESS_WRITE;
	param_types = param_types;
	params = params;
	i = 0; i=i;
	object = TEE_HANDLE_NULL;

	IMSG("%s called.\n",fn);

	IMSG("%s: clear ascii\n",fn);
	for(i=0;i<16;++i){
		IMSG("%c , ",((char*)params[1].memref.buffer)[i]);
	}

	IMSG("%s: clear hex\n",fn);
	for(i=0;i<16;++i){
		IMSG("%x , ",((char*)params[1].memref.buffer)[i]);
	}


	TEE_MemFill((char*)hashedKeyBuf,0,hkbSize);

	IMSG("createAndOpenObject called.\n");
	if((createAndOpenObject(objID, objID_len, flags,NULL,16,OPEN_ONLY)) == -1){
		IMSG("Couldn't open object!");
		return TEE_ERROR_ITEM_NOT_FOUND;
	}

	IMSG("TEE_ReadObjectData called.\n");
	if((TEE_ReadObjectData(object, hashedKeyBuf, hkbSize, &read_bytes)) != TEE_SUCCESS) {
		EMSG("TEE_ReadObjectData failed.\n");
	}

	IMSG("setAttribute called.\n");
	setAttribute(&aesAttributes,TEE_ATTR_SECRET_VALUE,hashedKeyBuf,hkbSize);

	IMSG("TEE_AllocateTransientObject called.\n");
	res = TEE_AllocateTransientObject(TEE_TYPE_AES, 128, &key);
	if (res != TEE_SUCCESS) {
		IMSG("TEE_AllocateTransientObject returned (%08x).",res);
		return res;
	}

	IMSG("TEE_ResetTransientObject called.\n");
	TEE_ResetTransientObject(key);

	IMSG("TEE_PopulateTransientObject called.\n");
	TEE_PopulateTransientObject(key, (TEE_Attribute *)&aesAttributes, 1);

	IMSG("TEE_GetObjectInfo called.\n");
  TEE_GetObjectInfo(key, &key_info);


	IMSG("TEE_AllocateOperation called.\n");
	if((TEE_AllocateOperation(&handle, TEE_ALG_AES_CBC_NOPAD, TEE_MODE_ENCRYPT, 128)) != TEE_SUCCESS ) {
        IMSG("TEE_AllocateOperation failed.\n");
        TEE_FreeTransientObject(key);
        return TEE_ERROR_ITEM_NOT_FOUND;
  }

	IMSG("TEE_SetOperationKey called.\n");
	if((TEE_SetOperationKey(handle,key)) != TEE_SUCCESS) {
		IMSG("TEE_SetOperationKey failed.\n");
		return TEE_ERROR_ITEM_NOT_FOUND;
  }

	IMSG("TEE_CipherInit called.\n");
	TEE_CipherInit(handle,iv,iv_len);

	sz= 16;
	IMSG("TEE_CipherDoFinal called.\n");
	TEE_CipherDoFinal(handle,params[1].memref.buffer,16,params[1].memref.buffer,&(sz));

	// IMSG("PRINTING ENCRYPTED DATA:\n");
	// for (i=0; i< 16; i++) {
	// 	IMSG("i = %d ,encrypted data=%x, ",i,((char*)params[1].memref.buffer)[i]);
	// }


	IMSG("%s: enc ascii\n",fn);
	for(i=0;i<16;++i){
		IMSG("%c , ",((char*)params[1].memref.buffer)[i]);
	}

	IMSG("%s: enc hex\n",fn);
	for(i=0;i<16;++i){
		IMSG("%x , ",((char*)params[1].memref.buffer)[i]);
	}


	TEE_FreeOperation(handle);
	TEE_FreeTransientObject(key);
	TEE_CloseObject(object);

	return TEE_SUCCESS;
}

/* int
 * Called when a TA is invoked. sess_ctx hold that value that was
 * assigned by TA_OpenSessionEntryPoint(). The rest of the paramters
 * comes from normal world.
 */


TEE_Result TA_InvokeCommandEntryPoint(void __maybe_unused *sess_ctx,
			uint32_t cmd_id,
			uint32_t param_types, TEE_Param params[4]) {

	(void)&sess_ctx; /* Unused parameter */


	switch (cmd_id) {

		case CPS_INIT: //CRYPT_SET_PASS:

			if(hashUserKey(param_types, params) == 0){
				//print hashed key
				IMSG("OPERATION: Set password | buffer: \n");
				for (uint32_t i = 0; i < params[1].memref.size; i++){
					IMSG("i=%d , keyBuf[i]:%02x ", i,keyBuf[i]);
				}
			}

			return TEE_SUCCESS;
		case CPS_PROTECT: //CRYPT_ENCRYPT:
			IMSG("OPERATION: encrypt | buffer: %s\n",buf);
			return encryptWithPrivateKey(param_types,params);
		case CPS_VIEW_RAW: //CRYPT_DECRYPT RAW:
			IMSG("OPERATION: decrypt(raw) | buffer: %s\n",buf);
			return decryptWithPrivateKey(param_types, params);
		case CPS_VIEW_ASCII: //CRYPT_DECRYPT ASCII:
			IMSG("OPERATION: decrypt(ascii) | buffer: %s\n",buf);
			return decryptWithPrivateKey(param_types, params);

		default:
			IMSG("OPERATION: TEE_ERROR_BAD_PARAMETERS | buffer: %s\n",buf);
			return TEE_ERROR_BAD_PARAMETERS;
	}

	TEE_CloseAndDeletePersistentObject(object);
	IMSG("Persistent Object has been delete.\n");
	return TEE_SUCCESS;
}
