/* JTox.c
 *
 *  Copyright (C) 2013 Tox project All Rights Reserved.
 *
 *  This file is part of jToxcore
 *
 *  jToxcore is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  jToxcore is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with jToxcore.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
#else
#include <arpa/inet.h>
#endif
#include <tox/tox.h>

#include "JTox.h"
#include "callbacks.h"

#define ADDR_SIZE_HEX (TOX_FRIEND_ADDRESS_SIZE * 2 + 1)

/**
 * Begin Utilities section
 */

/**
 * Convert a given binary address to a human-readable, \0-terminated hexadecimal string
 */
void addr_to_hex(uint8_t *addr, char *buf) {
	uint32_t i;

	for (i = 0; i < TOX_FRIEND_ADDRESS_SIZE; i++) {
		char xx[3];
		snprintf(xx, sizeof(xx), "%02X", addr[i] & 0xff);
		strcat(buf, xx);
	}
}

/**
 * Null-terminate the given string. Length is the length of the original string,
 * out must be allocated with a size of at least length+1
 */
void nullterminate(uint8_t *in, uint16_t length, char *out) {
	memcpy(out, in, length);
	out[length] = '\0';
}

/**
 * End Utilities section
 */

/**
 * Begin maintenance section
 */

JNIEXPORT jlong JNICALL Java_im_tox_jtoxcore_JTox_tox_1new(JNIEnv * env,
		jobject jobj) {
	tox_jni_globals_t *globals = malloc(sizeof(tox_jni_globals_t));
	JavaVM *jvm;
	jclass clazz = (*env)->GetObjectClass(env, jobj);
	jfieldID id = (*env)->GetFieldID(env, clazz, "handler",
			"Lim/tox/jtoxcore/callbacks/CallbackHandler;");
	jobject handler = (*env)->GetObjectField(env, jobj, id);
	jobject ref = (*env)->NewGlobalRef(env, handler);
	(*env)->GetJavaVM(env, &jvm);
	globals->tox = tox_new(1);
	globals->jvm = jvm;
	globals->handler = ref;

	tox_callback_action(globals->tox, (void *) callback_action, globals);
	tox_callback_connectionstatus(globals->tox,
			(void *) callback_connectionstatus, globals);
	tox_callback_friendmessage(globals->tox, (void *) callback_friendmessage,
			globals);
	tox_callback_friendrequest(globals->tox, (void *) callback_friendrequest,
			globals);
	tox_callback_namechange(globals->tox, (void *) callback_namechange,
			globals);
	tox_callback_read_receipt(globals->tox, (void *) callback_read_receipt,
			globals);
	tox_callback_statusmessage(globals->tox, (void *) callback_statusmessage,
			globals);
	tox_callback_userstatus(globals->tox, (void *) callback_userstatus,
			globals);
	return ((jlong) ((intptr_t) globals));
}

JNIEXPORT jint JNICALL Java_im_tox_jtoxcore_JTox_tox_1bootstrap(JNIEnv * env,
		jobject obj, jlong messenger, jstring ip, jint port, jbyteArray address) {
	const char *_ip = (*env)->GetStringUTFChars(env, ip, 0);
	uint8_t *_address = (*env)->GetByteArrayElements(env, address, 0);
	uint16_t _port = htons((uint16_t) port);

	jint result = tox_bootstrap_from_address(
			((tox_jni_globals_t *) ((intptr_t) messenger))->tox, _ip, 1, _port,
			_address);

	(*env)->ReleaseStringUTFChars(env, ip, _ip);
	(*env)->ReleaseByteArrayElements(env, address, _address, JNI_ABORT);
	return result;
}

JNIEXPORT void JNICALL Java_im_tox_jtoxcore_JTox_tox_1do(JNIEnv * env,
		jobject obj, jlong messenger) {
	tox_do(((tox_jni_globals_t *) ((intptr_t) messenger))->tox);
}

JNIEXPORT jint JNICALL Java_im_tox_jtoxcore_JTox_tox_1isconnected(JNIEnv * env,
		jobject obj, jlong messenger) {
	return tox_isconnected(((tox_jni_globals_t *) ((intptr_t) messenger))->tox);
}

JNIEXPORT void JNICALL Java_im_tox_jtoxcore_JTox_tox_1kill(JNIEnv * env,
		jobject jobj, jlong messenger) {
	tox_jni_globals_t *globals = (tox_jni_globals_t *) ((intptr_t) messenger);
	tox_kill(globals->tox);
	(*env)->DeleteGlobalRef(env, globals->handler);
	free(globals);
}

JNIEXPORT jbyteArray JNICALL Java_im_tox_jtoxcore_JTox_tox_1save(JNIEnv *env,
		jobject obj, jlong messenger) {
	Tox *tox = ((tox_jni_globals_t *) ((intptr_t) messenger))->tox;
	uint32_t size = tox_size(tox);
	uint8_t *data = malloc(size);
	tox_save(tox, data);
	jbyteArray bytes = (*env)->NewByteArray(env, size);
	(*env)->SetByteArrayRegion(env, bytes, 0, size, data);
	free(data);
	return bytes;
}

JNIEXPORT jboolean JNICALL Java_im_tox_jtoxcore_JTox_tox_1load(JNIEnv *env,
		jobject obj, jlong messenger, jbyteArray bytes, jint length) {
	uint8_t *data = (*env)->GetByteArrayElements(env, bytes, 0);
	return tox_load(((tox_jni_globals_t *) ((intptr_t) messenger))->tox, data,
			length) == 0 ?
	JNI_FALSE :
							JNI_TRUE;
}

/**
 * End maintenance section
 */

/**
 * Begin general section
 */

JNIEXPORT jint JNICALL Java_im_tox_jtoxcore_JTox_tox_1addfriend(JNIEnv * env,
		jobject obj, jlong messenger, jbyteArray address, jbyteArray data,
		jint length) {
	uint8_t *_address = (*env)->GetByteArrayElements(env, address, 0);
	uint8_t *_data = (*env)->GetByteArrayElements(env, data, 0);

	int ret = tox_addfriend(((tox_jni_globals_t *) ((intptr_t) messenger))->tox,
			_address, _data, length);

	(*env)->ReleaseByteArrayElements(env, address, _address, JNI_ABORT);
	(*env)->ReleaseByteArrayElements(env, data, _data, JNI_ABORT);
	return ret;
}

JNIEXPORT jint JNICALL Java_im_tox_jtoxcore_JTox_tox_1addfriend_1norequest(
		JNIEnv * env, jobject obj, jlong messenger, jbyteArray address) {
	uint8_t *_address = (*env)->GetByteArrayElements(env, address, 0);

	int ret = tox_addfriend_norequest(
			((tox_jni_globals_t *) ((intptr_t) messenger))->tox, _address);
	(*env)->ReleaseByteArrayElements(env, address, _address, JNI_ABORT);
	return ret;
}

JNIEXPORT jstring JNICALL Java_im_tox_jtoxcore_JTox_tox_1getaddress(
		JNIEnv * env, jobject obj, jlong messenger) {
	uint8_t addr[TOX_FRIEND_ADDRESS_SIZE];
	tox_getaddress(((tox_jni_globals_t *) ((intptr_t) messenger))->tox, addr);
	char id[ADDR_SIZE_HEX] = { 0 };
	addr_to_hex(addr, id);

	jstring result = (*env)->NewStringUTF(env, id);
	return result;
}

JNIEXPORT jint JNICALL Java_im_tox_jtoxcore_JTox_tox_1getfriend_1id(
		JNIEnv * env, jobject obj, jlong messenger, jstring address) {
	uint8_t *_address = (*env)->GetByteArrayElements(env, address, 0);

	int ret = tox_getfriend_id(
			((tox_jni_globals_t *) ((intptr_t) messenger))->tox, _address);
	(*env)->ReleaseByteArrayElements(env, address, _address, JNI_ABORT);
	return ret;
}

JNIEXPORT jstring JNICALL Java_im_tox_jtoxcore_JTox_tox_1getclient_1id(
		JNIEnv * env, jobject obj, jlong messenger, jint friendnumber) {
	uint8_t address[TOX_FRIEND_ADDRESS_SIZE];

	if (tox_getclient_id(((tox_jni_globals_t *) ((intptr_t) messenger))->tox,
			friendnumber, address) != 0) {
		return 0;
	} else {
		char _address[ADDR_SIZE_HEX] = { 0 };
		addr_to_hex(address, _address);
		jstring result = (*env)->NewStringUTF(env, _address);
		return result;
	}
}

JNIEXPORT jboolean JNICALL Java_im_tox_jtoxcore_JTox_tox_1delfriend(
		JNIEnv * env, jobject obj, jlong messenger, jint friendnumber) {
	return tox_delfriend(((tox_jni_globals_t *) ((intptr_t) messenger))->tox,
			friendnumber) == 0 ? 0 : 1;
}

JNIEXPORT jint JNICALL Java_im_tox_jtoxcore_JTox_tox_1sendmessage__JI_3BI(
		JNIEnv *env, jobject obj, jlong messenger, jint friendnumber,
		jbyteArray message, jint length) {
	uint8_t *_message = (*env)->GetByteArrayElements(env, message, 0);

	uint32_t mess_id = tox_sendmessage(
			((tox_jni_globals_t *) ((intptr_t) messenger))->tox, friendnumber,
			_message, length);
	(*env)->ReleaseByteArrayElements(env, message, _message, JNI_ABORT);
	return mess_id;
}

JNIEXPORT jint JNICALL Java_im_tox_jtoxcore_JTox_tox_1sendmessage__JI_3BII(
		JNIEnv *env, jobject obj, jlong messenger, jint friendnumber,
		jbyteArray message, jint length, jint messageID) {
	uint8_t *_message = (*env)->GetByteArrayElements(env, message, 0);

	uint32_t mess_id = tox_sendmessage_withid(
			((tox_jni_globals_t *) ((intptr_t) messenger))->tox, friendnumber,
			messageID, _message, length);
	(*env)->ReleaseByteArrayElements(env, message, _message, JNI_ABORT);
	return mess_id;
}

JNIEXPORT jboolean JNICALL Java_im_tox_jtoxcore_JTox_tox_1sendaction(
		JNIEnv * env, jobject obj, jlong messenger, jint friendnumber,
		jbyteArray action, jint length) {
	uint8_t *_action = (*env)->GetByteArrayElements(env, action, 0);

	jboolean ret = tox_sendaction(
			((tox_jni_globals_t *) ((intptr_t) messenger))->tox, friendnumber,
			_action, length);
	(*env)->ReleaseByteArrayElements(env, action, _action, JNI_ABORT);
	return ret;
}

JNIEXPORT jboolean JNICALL Java_im_tox_jtoxcore_JTox_tox_1setname(JNIEnv *env,
		jobject obj, jlong messenger, jbyteArray newname, jint length) {
	jbyte *_newname = (*env)->GetByteArrayElements(env, newname, 0);

	jboolean ret =
			tox_setname(((tox_jni_globals_t *) ((intptr_t) messenger))->tox,
					_newname, length) == 0 ? JNI_FALSE : JNI_TRUE;
	(*env)->ReleaseByteArrayElements(env, newname, _newname, JNI_ABORT);

	return ret;
}

JNIEXPORT jstring JNICALL Java_im_tox_jtoxcore_JTox_tox_1getselfname(
		JNIEnv *env, jobject obj, jlong messenger) {
	uint8_t *name = malloc(TOX_MAX_NAME_LENGTH);
	uint16_t length = tox_getselfname(
			((tox_jni_globals_t *) ((intptr_t) messenger))->tox, name,
			TOX_MAX_NAME_LENGTH);

	if (length == 0) {
		free(name);
		return 0;
	}
	char *_name = malloc(TOX_MAX_NAME_LENGTH + 1);
	nullterminate(name, length, _name);
	jstring __name = (*env)->NewStringUTF(env, _name);
	free(_name);
	free(name);

	return __name;
}

JNIEXPORT jboolean JNICALL Java_im_tox_jtoxcore_JTox_tox_1set_1statusmessage(
		JNIEnv *env, jobject obj, jlong messenger, jbyteArray newstatus,
		jint length) {
	uint8_t *_newstatus = (*env)->GetByteArrayElements(env, newstatus, 0);
	jboolean ret =
			tox_set_statusmessage(
					((tox_jni_globals_t *) ((intptr_t) messenger))->tox,
					_newstatus, length) == 0 ?
			JNI_FALSE :
												JNI_TRUE;
	(*env)->ReleaseByteArrayElements(env, newstatus, _newstatus, JNI_ABORT);
	return ret;
}

JNIEXPORT jbyteArray JNICALL Java_im_tox_jtoxcore_JTox_tox_1getname(
		JNIEnv * env, jobject obj, jlong messenger, jint friendnumber) {
	uint8_t *name = malloc(TOX_MAX_NAME_LENGTH);
	int ret = tox_getname(((tox_jni_globals_t *) ((intptr_t) messenger))->tox,
			friendnumber, name);

	if (ret == -1) {
		free(name);
		return 0;
	} else {
		jbyteArray _name = (*env)->NewByteArray(env, ret);
		(*env)->SetByteArrayRegion(env, _name, 0, ret, name);
		free(name);
		return _name;
	}
}

JNIEXPORT jboolean JNICALL Java_im_tox_jtoxcore_JTox_tox_1set_1userstatus(
		JNIEnv * env, jobject obj, jlong messenger, jint userstatus) {
	return tox_set_userstatus(
			((tox_jni_globals_t *) ((intptr_t) messenger))->tox, userstatus)
			== 0 ? JNI_FALSE : JNI_TRUE;
}

JNIEXPORT jbyteArray JNICALL Java_im_tox_jtoxcore_JTox_tox_1getstatusmessage(
		JNIEnv *env, jobject obj, jlong messenger, jint friendnumber) {
	Tox *tox = ((tox_jni_globals_t *) ((intptr_t) messenger))->tox;
	int size = tox_get_statusmessage_size(tox, friendnumber);
	uint8_t *statusmessage = malloc(size);
	int ret = tox_copy_statusmessage(tox, friendnumber, statusmessage, size);

	if (ret == -1) {
		free(statusmessage);
		return 0;
	} else {
		jbyteArray _statusmessage = (*env)->NewByteArray(env, ret);
		(*env)->SetByteArrayRegion(env, _statusmessage, 0, ret, statusmessage);
		free(statusmessage);
		return _statusmessage;
	}
}

JNIEXPORT jboolean JNICALL Java_im_tox_jtoxcore_JTox_tox_1friendexists(
		JNIEnv *env, jobject obj, jlong messenger, jint friendnumber) {
	return tox_friend_exists(
			((tox_jni_globals_t *) ((intptr_t) messenger))->tox, friendnumber);
}

JNIEXPORT jbyteArray JNICALL Java_im_tox_jtoxcore_JTox_tox_1getselfstatusmessage(
		JNIEnv *env, jobject obj, jlong messenger) {
	Tox *tox = ((tox_jni_globals_t *) ((intptr_t) messenger))->tox;
	uint8_t *status = malloc(TOX_MAX_STATUSMESSAGE_LENGTH);
	int length = tox_copy_self_statusmessage(tox, status,
	TOX_MAX_STATUSMESSAGE_LENGTH);

	if (length == -1) {
		free(status);
		return 0;
	} else {
		jbyteArray _status = (*env)->NewByteArray(env, length);
		(*env)->SetByteArrayRegion(env, _status, 0, length, status);
		free(status);
		return _status;
	}
}

JNIEXPORT jobject JNICALL Java_im_tox_jtoxcore_JTox_tox_1get_1userstatus(
		JNIEnv *env, jobject obj, jlong messenger, jint friendnumber) {
	Tox *tox = ((tox_jni_globals_t *) ((intptr_t) messenger))->tox;
	char *status;

	switch (tox_get_userstatus(tox, friendnumber)) {
	case TOX_USERSTATUS_NONE:
		status = "TOX_USERSTATUS_NONE";
		break;
	case TOX_USERSTATUS_AWAY:
		status = "TOX_USERSTATUS_AWAY";
		break;
	case TOX_USERSTATUS_BUSY:
		status = "TOX_USERSTATUS_BUSY";
		break;
	default:
		status = "TOX_USERSTATUS_INVALID";
		break;
	}

	jclass us_enum = (*env)->FindClass(env, "Lim/tox/jtoxcore/ToxUserStatus");
	jfieldID fieldID = (*env)->GetStaticFieldID(env, us_enum, status,
			"Lim/tox/jtoxcore/ToxUserStatus");
	return (*env)->GetStaticObjectField(env, us_enum, fieldID);
}

JNIEXPORT jobject JNICALL Java_im_tox_jtoxcore_JTox_tox_1get_1selfuserstatus(
		JNIEnv *env, jobject obj, jlong messenger) {
	Tox *tox = ((tox_jni_globals_t *) ((intptr_t) messenger))->tox;
	char *status;

	switch (tox_get_selfuserstatus(tox)) {
	case TOX_USERSTATUS_NONE:
		status = "TOX_USERSTATUS_NONE";
		break;
	case TOX_USERSTATUS_AWAY:
		status = "TOX_USERSTATUS_AWAY";
		break;
	case TOX_USERSTATUS_BUSY:
		status = "TOX_USERSTATUS_BUSY";
		break;
	default:
		status = "TOX_USERSTATUS_INVALID";
		break;
	}

	jclass us_enum = (*env)->FindClass(env, "Lim/tox/jtoxcore/ToxUserStatus");
	jfieldID fieldID = (*env)->GetStaticFieldID(env, us_enum, status,
			"Lim/tox/jtoxcore/ToxUserStatus");
	return (*env)->GetStaticObjectField(env, us_enum, fieldID);
}

JNIEXPORT void JNICALL Java_im_tox_jtoxcore_JTox_tox_1set_1sends_1receipts(
		JNIEnv *env, jobject obj, jlong messenger, jboolean send,
		jint friendnumber) {
	tox_set_sends_receipts(((tox_jni_globals_t *) ((intptr_t) messenger))->tox,
			friendnumber, send);
}

JNIEXPORT jintArray JNICALL Java_im_tox_jtoxcore_JTox_tox_1get_1friendlist(
		JNIEnv *env, jobject obj, jlong messenger) {
	Tox *tox = ((tox_jni_globals_t *) ((intptr_t) messenger))->tox;
	uint32_t length = tox_count_friendlist(tox);
	int *list = malloc(length);
	uint32_t actual_length = tox_copy_friendlist(tox, list, length);
	if (actual_length == -1) {
		free(list);
		return 0;
	} else {
		jintArray arr = (*env)->NewIntArray(env, actual_length);
		(*env)->SetIntArrayRegion(env, arr, 0, actual_length, (jint *) list);
		free(list);
		return arr;
	}
}

/**
 * End general section
 */

/**
 * Begin Callback Section
 */

static void callback_friendrequest(uint8_t *pubkey, uint8_t *message,
		uint16_t length, tox_jni_globals_t *ptr) {
	JNIEnv *env;
	(*ptr->jvm)->AttachCurrentThread(ptr->jvm, (void **) &env, 0);
	jclass clazz = (*env)->GetObjectClass(env, ptr->handler);
	jmethodID meth = (*env)->GetMethodID(env, clazz, "onFriendRequest",
			"(Ljava/lang/String;[B)V");

	char buf[ADDR_SIZE_HEX] = { 0 };
	addr_to_hex(pubkey, buf);
	jstring _pubkey = (*env)->NewStringUTF(env, buf);
	jbyteArray _message = (*env)->NewByteArray(env, length - 1);
	uint8_t *temp = malloc(length);
	(*env)->SetByteArrayRegion(env, _message, 0, length - 1, message);

	(*env)->CallVoidMethod(env, ptr->handler, meth, _pubkey, _message);
}

static void callback_friendmessage(Tox * tox, int friendnumber,
		uint8_t *message, uint16_t length, tox_jni_globals_t *ptr) {
	JNIEnv *env;
	(*ptr->jvm)->AttachCurrentThread(ptr->jvm, (void **) &env, 0);
	jclass class = (*env)->GetObjectClass(env, ptr->handler);
	jmethodID meth = (*env)->GetMethodID(env, class, "onMessage", "(I[B)V");

	jbyteArray _message = (*env)->NewByteArray(env, length - 1);
	(*env)->SetByteArrayRegion(env, _message, 0, length - 1, message);
	(*env)->CallVoidMethod(env, ptr->handler, meth, friendnumber, _message);
}

static void callback_action(Tox * tox, int friendnumber, uint8_t *action,
		uint16_t length, tox_jni_globals_t *ptr) {
	JNIEnv *env;
	(*ptr->jvm)->AttachCurrentThread(ptr->jvm, (void **) &env, 0);
	jclass class = (*env)->GetObjectClass(env, ptr->handler);
	jmethodID meth = (*env)->GetMethodID(env, class, "onAction", "(I[B)V");

	jbyteArray _action = (*env)->NewByteArray(env, length - 1);
	(*env)->SetByteArrayRegion(env, _action, 0, length - 1, action);
	(*env)->CallVoidMethod(env, ptr->handler, meth, friendnumber, _action);
}

static void callback_namechange(Tox * tox, int friendnumber, uint8_t *newname,
		uint16_t length, tox_jni_globals_t *ptr) {
	JNIEnv *env;
	(*ptr->jvm)->AttachCurrentThread(ptr->jvm, (void **) &env, 0);
	jclass class = (*env)->GetObjectClass(env, ptr->handler);
	jmethodID meth = (*env)->GetMethodID(env, class, "onNameChange", "(I[B)V");

	jbyteArray _newname = (*env)->NewByteArray(env, length - 1);
	(*env)->SetByteArrayRegion(env, _newname, 0, length - 1, newname);
	(*env)->CallVoidMethod(env, ptr->handler, meth, friendnumber, _newname);
}

static void callback_statusmessage(Tox *tox, int friendnumber,
		uint8_t *newstatus, uint16_t length, tox_jni_globals_t *ptr) {
	JNIEnv *env;
	(*ptr->jvm)->AttachCurrentThread(ptr->jvm, (void **) &env, 0);
	jclass class = (*env)->GetObjectClass(env, ptr->handler);
	jmethodID meth = (*env)->GetMethodID(env, class, "onStatusMessage",
			"(I[B)V");

	jbyteArray _newstatus = (*env)->NewByteArray(env, length - 1);
	(*env)->SetByteArrayRegion(env, _newstatus, 0, length - 1, newstatus);
	(*env)->CallVoidMethod(env, ptr->handler, meth, friendnumber, _newstatus);
}

static void callback_userstatus(Tox *tox, int friendnumber,
		TOX_USERSTATUS status, tox_jni_globals_t *ptr) {
	JNIEnv *env;
	(*ptr->jvm)->AttachCurrentThread(ptr->jvm, (void **) &env, 0);
	jclass class = (*env)->GetObjectClass(env, ptr->handler);
	jmethodID meth = (*env)->GetMethodID(env, class, "onUserStatus",
			"(ILim/tox/jtoxcore/ToxUserStatus;)V");
	jclass us_enum = (*env)->FindClass(env, "Lim/tox/jtoxcore/ToxUserStatus;");

	char *enum_name;
	switch (status) {
	case TOX_USERSTATUS_NONE:
		enum_name = "TOX_USERSTATUS_NONE";
		break;
	case TOX_USERSTATUS_AWAY:
		enum_name = "TOX_USERSTATUS_AWAY";
		break;
	case TOX_USERSTATUS_BUSY:
		enum_name = "TOX_USERSTATUS_BUSY";
		break;
	default:
		enum_name = "TOX_USERSTATUS_INVALID";
		break;
	}

	jfieldID fieldID = (*env)->GetStaticFieldID(env, us_enum, enum_name,
			"Lim/tox/jtoxcore/ToxUserStatus;");
	jobject enum_val = (*env)->GetStaticObjectField(env, us_enum, fieldID);
	(*env)->CallVoidMethod(env, ptr->handler, meth, friendnumber, enum_val);
}

static void callback_read_receipt(Tox *tox, int friendnumber, uint32_t receipt,
		tox_jni_globals_t *ptr) {
	JNIEnv *env;
	(*ptr->jvm)->AttachCurrentThread(ptr->jvm, (void **) &env, 0);
	jclass class = (*env)->GetObjectClass(env, ptr->handler);
	jmethodID meth = (*env)->GetMethodID(env, class, "onReadReceipt", "(II)V");
	(*env)->CallVoidMethod(env, ptr->handler, meth, friendnumber, receipt);
}

static void callback_connectionstatus(Tox *tox, int friendnumber,
		uint8_t newstatus, tox_jni_globals_t *ptr) {
	JNIEnv *env;
	(*ptr->jvm)->AttachCurrentThread(ptr->jvm, (void **) &env, 0);
	jclass class = (*env)->GetObjectClass(env, ptr->handler);
	jmethodID meth = (*env)->GetMethodID(env, class, "onConnectionStatus",
			"(IZ)V");
	jboolean _newstatus = newstatus == 0 ? JNI_FALSE : JNI_TRUE;
	(*env)->CallVoidMethod(env, ptr->handler, meth, friendnumber, _newstatus);
}
