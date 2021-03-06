/*
 * Copyright (C) 2012 Yamagi Burmeister
 * Copyright (C) 2010 skuller.net
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 * USA.
 *
 * =======================================================================
 *
 * Low level, platform depended "qal" API implementation. This files
 * provides functions to load, initialize, shutdown und unload the
 * OpenAL library and connects the "qal" funtion pointers to the
 * OpenAL functions. It shopuld work on Windows and unixoid Systems,
 * other platforms may need an own implementation. This source file
 * was taken from Q2Pro and modified by the YQ2 authors.
 *
 * =======================================================================
 */

#ifdef USE_OPENAL

#if defined (__APPLE__)
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#endif

#include "../../common/header/common.h"
#include "../../client/sound/header/local.h"
#include "header/qal.h"

static ALCcontext *context;
static ALCdevice *device;
static cvar_t *al_device;
static cvar_t *al_driver;
static void *handle;

/* Function pointers for OpenAL management */
static LPALCCREATECONTEXT qalcCreateContext;
static LPALCMAKECONTEXTCURRENT qalcMakeContextCurrent;
static LPALCPROCESSCONTEXT qalcProcessContext;
static LPALCSUSPENDCONTEXT qalcSuspendContext;
static LPALCDESTROYCONTEXT qalcDestroyContext;
static LPALCGETCURRENTCONTEXT qalcGetCurrentContext;
static LPALCGETCONTEXTSDEVICE qalcGetContextsDevice;
static LPALCOPENDEVICE qalcOpenDevice;
static LPALCCLOSEDEVICE qalcCloseDevice;
static LPALCGETERROR qalcGetError;
static LPALCISEXTENSIONPRESENT qalcIsExtensionPresent;
static LPALCGETPROCADDRESS qalcGetProcAddress;
static LPALCGETENUMVALUE qalcGetEnumValue;
static LPALCGETSTRING qalcGetString;
static LPALCGETINTEGERV qalcGetIntegerv;
static LPALCCAPTUREOPENDEVICE qalcCaptureOpenDevice;
static LPALCCAPTURECLOSEDEVICE qalcCaptureCloseDevice;
static LPALCCAPTURESTART qalcCaptureStart;
static LPALCCAPTURESTOP qalcCaptureStop;
static LPALCCAPTURESAMPLES qalcCaptureSamples ;

/* Declaration of function pointers used
   to connect OpenAL to our internal API */
LPALENABLE qalEnable;
LPALDISABLE qalDisable;
LPALISENABLED qalIsEnabled;
LPALGETSTRING qalGetString;
LPALGETBOOLEANV qalGetBooleanv;
LPALGETINTEGERV qalGetIntegerv;
LPALGETFLOATV qalGetFloatv;
LPALGETDOUBLEV qalGetDoublev;
LPALGETBOOLEAN qalGetBoolean;
LPALGETINTEGER qalGetInteger;
LPALGETFLOAT qalGetFloat;
LPALGETDOUBLE qalGetDouble;
LPALGETERROR qalGetError;
LPALISEXTENSIONPRESENT qalIsExtensionPresent;
LPALGETPROCADDRESS qalGetProcAddress;
LPALGETENUMVALUE qalGetEnumValue;
LPALLISTENERF qalListenerf;
LPALLISTENER3F qalListener3f;
LPALLISTENERFV qalListenerfv;
LPALLISTENERI qalListeneri;
LPALLISTENER3I qalListener3i;
LPALLISTENERIV qalListeneriv;
LPALGETLISTENERF qalGetListenerf;
LPALGETLISTENER3F qalGetListener3f;
LPALGETLISTENERFV qalGetListenerfv;
LPALGETLISTENERI qalGetListeneri;
LPALGETLISTENER3I qalGetListener3i;
LPALGETLISTENERIV qalGetListeneriv;
LPALGENSOURCES qalGenSources;
LPALDELETESOURCES qalDeleteSources;
LPALISSOURCE qalIsSource;
LPALSOURCEF qalSourcef;
LPALSOURCE3F qalSource3f;
LPALSOURCEFV qalSourcefv;
LPALSOURCEI qalSourcei;
LPALSOURCE3I qalSource3i;
LPALSOURCEIV qalSourceiv;
LPALGETSOURCEF qalGetSourcef;
LPALGETSOURCE3F qalGetSource3f;
LPALGETSOURCEFV qalGetSourcefv;
LPALGETSOURCEI qalGetSourcei;
LPALGETSOURCE3I qalGetSource3i;
LPALGETSOURCEIV qalGetSourceiv;
LPALSOURCEPLAYV qalSourcePlayv;
LPALSOURCESTOPV qalSourceStopv;
LPALSOURCEREWINDV qalSourceRewindv;
LPALSOURCEPAUSEV qalSourcePausev;
LPALSOURCEPLAY qalSourcePlay;
LPALSOURCESTOP qalSourceStop;
LPALSOURCEREWIND qalSourceRewind;
LPALSOURCEPAUSE qalSourcePause;
LPALSOURCEQUEUEBUFFERS qalSourceQueueBuffers;
LPALSOURCEUNQUEUEBUFFERS qalSourceUnqueueBuffers;
LPALGENBUFFERS qalGenBuffers;
LPALDELETEBUFFERS qalDeleteBuffers;
LPALISBUFFER qalIsBuffer;
LPALBUFFERDATA qalBufferData;
LPALBUFFERF qalBufferf;
LPALBUFFER3F qalBuffer3f;
LPALBUFFERFV qalBufferfv;
LPALBUFFERI qalBufferi;
LPALBUFFER3I qalBuffer3i;
LPALBUFFERIV qalBufferiv;
LPALGETBUFFERF qalGetBufferf;
LPALGETBUFFER3F qalGetBuffer3f;
LPALGETBUFFERFV qalGetBufferfv;
LPALGETBUFFERI qalGetBufferi;
LPALGETBUFFER3I qalGetBuffer3i;
LPALGETBUFFERIV qalGetBufferiv;
LPALDOPPLERFACTOR qalDopplerFactor;
LPALDOPPLERVELOCITY qalDopplerVelocity;
LPALSPEEDOFSOUND qalSpeedOfSound;
LPALDISTANCEMODEL qalDistanceModel;
#if !defined (__APPLE__)
LPALGENFILTERS qalGenFilters;
LPALFILTERI qalFilteri;
LPALFILTERF qalFilterf;
LPALDELETEFILTERS qalDeleteFilters;
#endif

/*
 * Gives information over the OpenAL
 * implementation and it's state
 */
void QAL_SoundInfo()
{
	Com_Printf("OpenAL settings:\n");
    Com_Printf("AL_VENDOR: %s\n", qalGetString(AL_VENDOR));
    Com_Printf("AL_RENDERER: %s\n", qalGetString(AL_RENDERER));
    Com_Printf("AL_VERSION: %s\n", qalGetString(AL_VERSION));
    Com_Printf("AL_EXTENSIONS: %s\n", qalGetString(AL_EXTENSIONS));

	if (qalcIsExtensionPresent(NULL, "ALC_ENUMERATE_ALL_EXT"))
	{
		const char *devs = qalcGetString(NULL, ALC_ALL_DEVICES_SPECIFIER);

		Com_Printf("\nAvailable OpenAL devices:\n");

		if (devs == NULL)
		{
			Com_Printf("- No devices found. Depending on your\n");
			Com_Printf("  platform this may be expected and\n");
			Com_Printf("  doesn't indicate a problem!\n");
		}
		else
		{
			while (devs && *devs)
			{
				Com_Printf("- %s\n", devs);
				devs += strlen(devs) + 1;
			}
		}
	}

   	if (qalcIsExtensionPresent(NULL, "ALC_ENUMERATE_ALL_EXT"))
	{
		const char *devs = qalcGetString(device, ALC_DEVICE_SPECIFIER);

		Com_Printf("\nCurrent OpenAL device:\n");

		if (devs == NULL)
		{
			Com_Printf("- No OpenAL device in use\n");
		}
		else
		{
			Com_Printf("- %s\n", devs);
		}
	}
}

/*
 * Shuts OpenAL down, frees all context and
 * device handles and unloads the shared lib.
 */
void
QAL_Shutdown()
{
    if (context)
   	{
        qalcMakeContextCurrent( NULL );
        qalcDestroyContext( context );
        context = NULL;
    }

	if (device)
	{
        qalcCloseDevice( device );
        device = NULL;
    }

	/* Disconnect function pointers used
	   for OpenAL management calls */
	qalcCreateContext = NULL;
	qalcMakeContextCurrent = NULL;
	qalcProcessContext = NULL;
	qalcSuspendContext = NULL;
	qalcDestroyContext = NULL;
	qalcGetCurrentContext = NULL;
	qalcGetContextsDevice = NULL;
	qalcOpenDevice = NULL;
	qalcCloseDevice = NULL;
	qalcGetError = NULL;
	qalcIsExtensionPresent = NULL;
	qalcGetProcAddress = NULL;
	qalcGetEnumValue = NULL;
	qalcGetString = NULL;
	qalcGetIntegerv = NULL;
	qalcCaptureOpenDevice = NULL;
	qalcCaptureCloseDevice = NULL;
	qalcCaptureStart = NULL;
	qalcCaptureStop = NULL;
	qalcCaptureSamples = NULL;

	/* Disconnect OpenAL
	 * function pointers */
	qalEnable = NULL;
	qalDisable = NULL;
	qalIsEnabled = NULL;
	qalGetString = NULL;
	qalGetBooleanv = NULL;
	qalGetIntegerv = NULL;
	qalGetFloatv = NULL;
	qalGetDoublev = NULL;
	qalGetBoolean = NULL;
	qalGetInteger = NULL;
	qalGetFloat = NULL;
	qalGetDouble = NULL;
	qalGetError = NULL;
	qalIsExtensionPresent = NULL;
	qalGetProcAddress = NULL;
	qalGetEnumValue = NULL;
	qalListenerf = NULL;
	qalListener3f = NULL;
	qalListenerfv = NULL;
	qalListeneri = NULL;
	qalListener3i = NULL;
	qalListeneriv = NULL;
	qalGetListenerf = NULL;
	qalGetListener3f = NULL;
	qalGetListenerfv = NULL;
	qalGetListeneri = NULL;
	qalGetListener3i = NULL;
	qalGetListeneriv = NULL;
	qalGenSources = NULL;
	qalDeleteSources = NULL;
	qalIsSource = NULL;
	qalSourcef = NULL;
	qalSource3f = NULL;
	qalSourcefv = NULL;
	qalSourcei = NULL;
	qalSource3i = NULL;
	qalSourceiv = NULL;
	qalGetSourcef = NULL;
	qalGetSource3f = NULL;
	qalGetSourcefv = NULL;
	qalGetSourcei = NULL;
	qalGetSource3i = NULL;
	qalGetSourceiv = NULL;
	qalSourcePlayv = NULL;
	qalSourceStopv = NULL;
	qalSourceRewindv = NULL;
	qalSourcePausev = NULL;
	qalSourcePlay = NULL;
	qalSourceStop = NULL;
	qalSourceRewind = NULL;
	qalSourcePause = NULL;
	qalSourceQueueBuffers = NULL;
	qalSourceUnqueueBuffers = NULL;
	qalGenBuffers = NULL;
	qalDeleteBuffers = NULL;
	qalIsBuffer = NULL;
	qalBufferData = NULL;
	qalBufferf = NULL;
	qalBuffer3f = NULL;
	qalBufferfv = NULL;
	qalBufferi = NULL;
	qalBuffer3i = NULL;
	qalBufferiv = NULL;
	qalGetBufferf = NULL;
	qalGetBuffer3f = NULL;
	qalGetBufferfv = NULL;
	qalGetBufferi = NULL;
	qalGetBuffer3i = NULL;
	qalGetBufferiv = NULL;
	qalDopplerFactor = NULL;
	qalDopplerVelocity = NULL;
	qalSpeedOfSound = NULL;
	qalDistanceModel = NULL;
#if !defined (__APPLE__)	
	qalGenFilters = NULL;
	qalFilteri = NULL;
	qalFilterf = NULL;
	qalDeleteFilters = NULL;
#endif

	/* Unload the shared lib */
	Sys_FreeLibrary(handle);
    handle = NULL;
}

/*
 * Loads the OpenAL shared lib, creates
 * a context and device handle.
 */
qboolean
QAL_Init()
{
	/* DEFAULT_OPENAL_DRIVER is defined at compile time via the compiler */
	al_driver = Cvar_Get("al_driver", DEFAULT_OPENAL_DRIVER, CVAR_ARCHIVE);
	al_device = Cvar_Get("al_device", "", CVAR_ARCHIVE);

	Com_Printf("LoadLibrary(%s)\n", al_driver->string);

	/* Load the library */
	Sys_LoadLibrary(al_driver->string, NULL, &handle);

	if (!handle)
	{
		Com_Printf("Loading %s failed! Disabling OpenAL.\n", al_driver->string);
		return false;
	}

	/* Connect function pointers to management functions */
	qalcCreateContext = Sys_GetProcAddress(handle, "alcCreateContext");
	qalcMakeContextCurrent = Sys_GetProcAddress(handle, "alcMakeContextCurrent");
	qalcProcessContext = Sys_GetProcAddress(handle, "alcProcessContext");
	qalcSuspendContext = Sys_GetProcAddress(handle, "alcSuspendContext");
	qalcDestroyContext = Sys_GetProcAddress(handle, "alcDestroyContext");
	qalcGetCurrentContext = Sys_GetProcAddress(handle, "alcGetCurrentContext");
	qalcGetContextsDevice = Sys_GetProcAddress(handle, "alcGetContextsDevice");
	qalcOpenDevice = Sys_GetProcAddress(handle, "alcOpenDevice");
	qalcCloseDevice = Sys_GetProcAddress(handle, "alcCloseDevice");
	qalcGetError = Sys_GetProcAddress(handle, "alcGetError");
	qalcIsExtensionPresent = Sys_GetProcAddress(handle, "alcIsExtensionPresent");
	qalcGetProcAddress = Sys_GetProcAddress(handle, "alcGetProcAddress");
	qalcGetEnumValue = Sys_GetProcAddress(handle, "alcGetEnumValue");
	qalcGetString = Sys_GetProcAddress(handle, "alcGetString");
	qalcGetIntegerv = Sys_GetProcAddress(handle, "alcGetIntegerv");
	qalcCaptureOpenDevice = Sys_GetProcAddress(handle, "alcCaptureOpenDevice");
	qalcCaptureCloseDevice = Sys_GetProcAddress(handle, "alcCaptureCloseDevice");
	qalcCaptureStart = Sys_GetProcAddress(handle, "alcCaptureStart");
	qalcCaptureStop = Sys_GetProcAddress(handle, "alcCaptureStop");
	qalcCaptureSamples = Sys_GetProcAddress(handle, "alcCaptureSamples");

	/* Connect function pointers to
	   to OpenAL API functions */
	qalEnable = Sys_GetProcAddress(handle, "alEnable");
	qalDisable = Sys_GetProcAddress(handle, "alDisable");
	qalIsEnabled = Sys_GetProcAddress(handle, "alIsEnabled");
	qalGetString = Sys_GetProcAddress(handle, "alGetString");
	qalGetBooleanv = Sys_GetProcAddress(handle, "alGetBooleanv");
	qalGetIntegerv = Sys_GetProcAddress(handle, "alGetIntegerv");
	qalGetFloatv = Sys_GetProcAddress(handle, "alGetFloatv");
	qalGetDoublev = Sys_GetProcAddress(handle, "alGetDoublev");
	qalGetBoolean = Sys_GetProcAddress(handle, "alGetBoolean");
	qalGetInteger = Sys_GetProcAddress(handle, "alGetInteger");
	qalGetFloat = Sys_GetProcAddress(handle, "alGetFloat");
	qalGetDouble = Sys_GetProcAddress(handle, "alGetDouble");
	qalGetError = Sys_GetProcAddress(handle, "alGetError");
	qalIsExtensionPresent = Sys_GetProcAddress(handle, "alIsExtensionPresent");
	qalGetProcAddress = Sys_GetProcAddress(handle, "alGetProcAddress");
	qalGetEnumValue = Sys_GetProcAddress(handle, "alGetEnumValue");
	qalListenerf = Sys_GetProcAddress(handle, "alListenerf");
	qalListener3f = Sys_GetProcAddress(handle, "alListener3f");
	qalListenerfv = Sys_GetProcAddress(handle, "alListenerfv");
	qalListeneri = Sys_GetProcAddress(handle, "alListeneri");
	qalListener3i = Sys_GetProcAddress(handle, "alListener3i");
	qalListeneriv = Sys_GetProcAddress(handle, "alListeneriv");
	qalGetListenerf = Sys_GetProcAddress(handle, "alGetListenerf");
	qalGetListener3f = Sys_GetProcAddress(handle, "alGetListener3f");
	qalGetListenerfv = Sys_GetProcAddress(handle, "alGetListenerfv");
	qalGetListeneri = Sys_GetProcAddress(handle, "alGetListeneri");
	qalGetListener3i = Sys_GetProcAddress(handle, "alGetListener3i");
	qalGetListeneriv = Sys_GetProcAddress(handle, "alGetListeneriv");
	qalGenSources = Sys_GetProcAddress(handle, "alGenSources");
	qalDeleteSources = Sys_GetProcAddress(handle, "alDeleteSources");
	qalIsSource = Sys_GetProcAddress(handle, "alIsSource");
	qalSourcef = Sys_GetProcAddress(handle, "alSourcef");
	qalSource3f = Sys_GetProcAddress(handle, "alSource3f");
	qalSourcefv = Sys_GetProcAddress(handle, "alSourcefv");
	qalSourcei = Sys_GetProcAddress(handle, "alSourcei");
	qalSource3i = Sys_GetProcAddress(handle, "alSource3i");
	qalSourceiv = Sys_GetProcAddress(handle, "alSourceiv");
	qalGetSourcef = Sys_GetProcAddress(handle, "alGetSourcef");
	qalGetSource3f = Sys_GetProcAddress(handle, "alGetSource3f");
	qalGetSourcefv = Sys_GetProcAddress(handle, "alGetSourcefv");
	qalGetSourcei = Sys_GetProcAddress(handle, "alGetSourcei");
	qalGetSource3i = Sys_GetProcAddress(handle, "alGetSource3i");
	qalGetSourceiv = Sys_GetProcAddress(handle, "alGetSourceiv");
	qalSourcePlayv = Sys_GetProcAddress(handle, "alSourcePlayv");
	qalSourceStopv = Sys_GetProcAddress(handle, "alSourceStopv");
	qalSourceRewindv = Sys_GetProcAddress(handle, "alSourceRewindv");
	qalSourcePausev = Sys_GetProcAddress(handle, "alSourcePausev");
	qalSourcePlay = Sys_GetProcAddress(handle, "alSourcePlay");
	qalSourceStop = Sys_GetProcAddress(handle, "alSourceStop");
	qalSourceRewind = Sys_GetProcAddress(handle, "alSourceRewind");
	qalSourcePause = Sys_GetProcAddress(handle, "alSourcePause");
	qalSourceQueueBuffers = Sys_GetProcAddress(handle, "alSourceQueueBuffers");
	qalSourceUnqueueBuffers = Sys_GetProcAddress(handle, "alSourceUnqueueBuffers");
	qalGenBuffers = Sys_GetProcAddress(handle, "alGenBuffers");
	qalDeleteBuffers = Sys_GetProcAddress(handle, "alDeleteBuffers");
	qalIsBuffer = Sys_GetProcAddress(handle, "alIsBuffer");
	qalBufferData = Sys_GetProcAddress(handle, "alBufferData");
	qalBufferf = Sys_GetProcAddress(handle, "alBufferf");
	qalBuffer3f = Sys_GetProcAddress(handle, "alBuffer3f");
	qalBufferfv = Sys_GetProcAddress(handle, "alBufferfv");
	qalBufferi = Sys_GetProcAddress(handle, "alBufferi");
	qalBuffer3i = Sys_GetProcAddress(handle, "alBuffer3i");
	qalBufferiv = Sys_GetProcAddress(handle, "alBufferiv");
	qalGetBufferf = Sys_GetProcAddress(handle, "alGetBufferf");
	qalGetBuffer3f = Sys_GetProcAddress(handle, "alGetBuffer3f");
	qalGetBufferfv = Sys_GetProcAddress(handle, "alGetBufferfv");
	qalGetBufferi = Sys_GetProcAddress(handle, "alGetBufferi");
	qalGetBuffer3i = Sys_GetProcAddress(handle, "alGetBuffer3i");
	qalGetBufferiv = Sys_GetProcAddress(handle, "alGetBufferiv");
	qalDopplerFactor = Sys_GetProcAddress(handle, "alDopplerFactor");
	qalDopplerVelocity = Sys_GetProcAddress(handle, "alDopplerVelocity");
	qalSpeedOfSound = Sys_GetProcAddress(handle, "alSpeedOfSound");
	qalDistanceModel = Sys_GetProcAddress(handle, "alDistanceModel");

	/* Open the OpenAL device */
    Com_Printf("...opening OpenAL device:");

	device = qalcOpenDevice(al_device->string[0] ? al_device->string : NULL);

	if(!device)
	{
		Com_DPrintf("failed\n");
		QAL_Shutdown();
		return false;
	}

	Com_Printf("ok\n");

	/* Create the OpenAL context */
	Com_Printf("...creating OpenAL context: ");

	context = qalcCreateContext(device, NULL);

	if(!context)
	{
		Com_DPrintf("failed\n");
		QAL_Shutdown();
		return false;
	}

	Com_Printf("ok\n");

	/* Set the created context as current context */
	Com_Printf("...making context current: ");

	if (!qalcMakeContextCurrent(context))
	{
		Com_DPrintf("failed\n");
		QAL_Shutdown();
		return false;
	}

#if !defined (__APPLE__)
    if (qalcIsExtensionPresent(device, "ALC_EXT_EFX") != AL_FALSE) {
        qalGenFilters = qalGetProcAddress("alGenFilters");
        qalFilteri = qalGetProcAddress("alFilteri");
        qalFilterf = qalGetProcAddress("alFilterf");
        qalDeleteFilters = qalGetProcAddress("alDeleteFilters");
    } else {
        qalGenFilters = NULL;
        qalFilteri = NULL;
        qalFilterf = NULL;
        qalDeleteFilters = NULL;
    }
#endif

	Com_Printf("ok\n");

	/* Print OpenAL informations */
	Com_Printf("\n");
	QAL_SoundInfo();
	Com_Printf("\n");

    return true;
}

#endif /* USE_OPENAL */
