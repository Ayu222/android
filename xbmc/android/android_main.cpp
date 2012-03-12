#include <android_native_app_glue.h>
#include <android/log.h>
#include <jni.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>

#include "xbmc.h"

typedef struct {
  struct android_app* app;
  
  XBMC_PLATFORM* platform;
  XBMC_Run_t run;
  XBMC_Initialize_t initialize;
} android_context;

static int android_printf(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  int result = __android_log_vprint(ANDROID_LOG_VERBOSE, "XBMC", format, args);
  va_end(args);
  return result;
}

void* tryopen(const char *path)
{
  void *handle;
  handle = dlopen(path, RTLD_NOW);
  if (!handle)
  {
    __android_log_print(ANDROID_LOG_VERBOSE, "XBMC", "could not load %s. Error: %s",path, dlerror());
    exit(1);
  }
  __android_log_print(ANDROID_LOG_VERBOSE, "XBMC", "loaded lib: %s",path);
  return handle;
}

void preload()
{
  tryopen("/data/data/org.xbmc/lib/libyajl.so");
  tryopen("/data/data/org.xbmc/lib/libtiff.so");
  tryopen("/data/data/org.xbmc/lib/libjpeg.so");
  tryopen("/data/data/org.xbmc/lib/libgpg-error.so");
  tryopen("/data/data/org.xbmc/lib/libgcrypt.so");
  tryopen("/data/data/org.xbmc/lib/libiconv.so");
  tryopen("/data/data/org.xbmc/lib/libfreetype.so");
  tryopen("/data/data/org.xbmc/lib/libfontconfig.so");
  tryopen("/data/data/org.xbmc/lib/libfribidi.so");
  tryopen("/data/data/org.xbmc/lib/libsqlite3.so");
  tryopen("/data/data/org.xbmc/lib/libpng.so");
  tryopen("/data/data/org.xbmc/lib/libpcre.so");
  tryopen("/data/data/org.xbmc/lib/libsamplerate.so");
  tryopen("/data/data/org.xbmc/lib/libpython2.6.so");
  tryopen("/data/data/org.xbmc/lib/libpcrecpp.so");
  tryopen("/data/data/org.xbmc/lib/libzip.so");

  // gdb sleeps before attaching for some reason. let it attach before we try
  // to load libxbmc.
  sleep(1);
}

void setup_env(struct android_app* state)
{
  JavaVM* vm  = state->activity->vm;
  JNIEnv* env = state->activity->env;

  vm->AttachCurrentThread(&env, NULL);
  jobject activity = state->activity->clazz;
  jclass activityClass = env->GetObjectClass(activity);

  char cstr[1024];
  jmethodID getPackageResourcePath = env->GetMethodID(activityClass, "getPackageResourcePath", "()Ljava/lang/String;");
  jstring jApkPath = (jstring)env->CallObjectMethod(activity, getPackageResourcePath);
  const char* apkPath = env->GetStringUTFChars(jApkPath, NULL);
  strcpy(cstr, apkPath);
  env->ReleaseStringUTFChars(jApkPath, apkPath);
  strcat(cstr, "/assets");
  setenv("XBMC_BIN_HOME", cstr, 0);
  setenv("XBMC_HOME"    , cstr, 0);
  
  memset(cstr, 0, 1024);

  // Get the path to the temp/cache directory
  jmethodID getCacheDir = env->GetMethodID(activityClass, "getCacheDir", "()Ljava/io/File;");
  jobject jCacheDir = env->CallObjectMethod(activity, getCacheDir);
  
  jclass fileClass = env->GetObjectClass(jCacheDir);
  jmethodID getAbsolutePath = env->GetMethodID(fileClass, "getAbsolutePath", "()Ljava/lang/String;");
  
  jstring jCachePath = (jstring)env->CallObjectMethod(jCacheDir, getAbsolutePath);
  const char* cachePath = env->GetStringUTFChars(jCachePath, NULL);
  setenv("XBMC_TEMP", cachePath, 0);
  env->ReleaseStringUTFChars(jCachePath, cachePath);
  
  // Get the path to the external storage
  if (state->activity->externalDataPath != NULL)
    strcpy(cstr, state->activity->externalDataPath);
  else
  {
    // We need to employ JNI to get the path because of a (known) bug in Android 2.3.x
    jmethodID getExternalFilesDir = env->GetMethodID(activityClass, "getExternalFilesDir", "(Ljava/lang/String;)Ljava/io/File;");
    jobject jExternalDir = env->CallObjectMethod(activity, getExternalFilesDir, (jstring)NULL);
    
    jstring jExternalPath = (jstring)env->CallObjectMethod(jExternalDir, getAbsolutePath);
    const char *externalPath = env->GetStringUTFChars(jExternalPath, NULL);
    strcpy(cstr, externalPath);
    env->ReleaseStringUTFChars(jExternalPath, externalPath);
  }

  // Check if we don't have a valid path yet
  if (strlen(cstr) <= 0)
  {
    // Get the path to the internal storage
    if (state->activity->internalDataPath != NULL)
      strcpy(cstr, state->activity->internalDataPath);
    else
    {
      // We need to employ JNI to get the path because of a (known) bug in Android 2.3.x
      jstring jstrName = env->NewStringUTF("org.xbmc");
      jmethodID getDir = env->GetMethodID(activityClass, "getDir", "(Ljava/lang/String;I)Ljava/io/File;");
      jobject jInternalDir = env->CallObjectMethod(activity, getDir, jstrName, 1 /* MODE_WORLD_READABLE */);
    
      jstring jInternalPath = (jstring)env->CallObjectMethod(jInternalDir, getAbsolutePath);
      const char *internalPath = env->GetStringUTFChars(jInternalPath, NULL);
      strcpy(cstr, internalPath);
      env->ReleaseStringUTFChars(jInternalPath, internalPath);
    }
  }
  
  // Check if we have a valid home path
  if (strlen(cstr) > 0)
    setenv("HOME", cstr, 0);
  else
    setenv("HOME", getenv("XBMC_TEMP"), 0);
}

static void android_handle_cmd(struct android_app* app, int32_t cmd)
{
  android_context* context = (android_context*)app->userData;
  switch (cmd)
  {
    case APP_CMD_INIT_WINDOW:
      // The window is being shown, get it ready.
      android_printf("DEBUG: APP_CMD_INIT_WINDOW");
      if (context->app->window != NULL)
      {
        context->platform->window = context->app->window;
      
        int status = 0;
        status = context->initialize(context->platform, NULL, NULL);
        if (status == 0)
        {
          try
          {
            status = context->run();
          }
          catch(...)
          {
            android_printf("ERROR: Exception caught on main loop. Exiting");
          }
        }
        else
          android_printf("ERROR: XBMC_Initialize failed. Exiting");
          
        // TODO: Make sure we exit android_main()
      }
      break;
      
    case APP_CMD_TERM_WINDOW:
      // The window is being hidden or closed, clean it up.
      android_printf("DEBUG: APP_CMD_TERM_WINDOW");
      // TODO
      break;
      
    case APP_CMD_WINDOW_RESIZED:
      // The window has been resized
      android_printf("DEBUG: APP_CMD_WINDOW_RESIZED");
      // TODO
      break;
      
    case APP_CMD_GAINED_FOCUS:
      android_printf("DEBUG: APP_CMD_GAINED_FOCUS");
      // TODO
      break;
      
    case APP_CMD_LOST_FOCUS:
      android_printf("DEBUG: APP_CMD_LOST_FOCUS");
      // TODO
      break;
      
    case APP_CMD_CONFIG_CHANGED:
      android_printf("DEBUG: APP_CMD_CONFIG_CHANGED");
      // TODO
      break;
      
    case APP_CMD_START:
      android_printf("DEBUG: APP_CMD_START");
      // TODO
      break;
      
    case APP_CMD_RESUME:
      android_printf("DEBUG: APP_CMD_RESUME");
      // TODO
      break;
        
    case APP_CMD_SAVE_STATE:
      // The system has asked us to save our current state. Do so.
      android_printf("DEBUG: APP_CMD_SAVE_STATE");
      // TODO
      break;
      
    case APP_CMD_PAUSE:
      android_printf("DEBUG: APP_CMD_PAUSE");
      // TODO
      break;
      
    case APP_CMD_STOP:
      android_printf("DEBUG: APP_CMD_STOP");
      // TODO
      break;
      
    case APP_CMD_DESTROY:
      android_printf("DEBUG: APP_CMD_DESTROY");
      // TODO
      break;
  }
}

static int32_t android_handle_input(struct android_app* app, AInputEvent* event)
{
  android_context* context = (android_context*)app->userData;
  int32_t input = AInputEvent_getType(event);
  
  android_printf("DEBUG: onInputEvent of type %d", input);
  // TODO
  
  // We didn't handle the event
  return 0;
}

extern void android_main(struct android_app* state)
{
  // make sure that the linker doesn't strip out our glue
  app_dummy();

  preload();
  setup_env(state);

  void* soHandle;
  soHandle = tryopen("/data/data/org.xbmc/lib/libxbmc.so");

  // Setting up the context
  android_context context;
  memset(&context, 0, sizeof(context));
  
  // fetch xbmc entry points
  context.run = (XBMC_Run_t)dlsym(soHandle, "XBMC_Run");
  context.initialize = (XBMC_Initialize_t)dlsym(soHandle, "XBMC_Initialize");
  if (context.run == NULL || context.initialize == NULL)
  {
    android_printf("could not find XBMC_Run and/or XBMC_Initialize functions. Error: %s", dlerror());
    exit(1);
  }

  // XBMC_PLATFORM MUST exist for app lifetime.
  static XBMC_PLATFORM platform = {XBMCRunNull, 0};
  // stardard platform stuff
  platform.flags  = XBMCRunAsApp;
  platform.log_name = "XBMC";
  // android specific
  platform.window = state->window;
  // callbacks into android
  platform.android_printf = &android_printf;
  platform.android_setBuffersGeometry = &ANativeWindow_setBuffersGeometry;
  
  state->userData = &context;
  state->onAppCmd = android_handle_cmd;
  state->onInputEvent = android_handle_input;
  context.app = state;
  context.platform = &platform;
  
  while (1) {
    bool stop = false;
    // Read all pending events.
    int ident;
    int events;
    struct android_poll_source* source;

    // We will block forever waiting for events.
    while ((ident = ALooper_pollAll(-1, NULL, &events, (void**)&source)) >= 0) 
    {
      // Process this event.
      if (source != NULL)
        source->process(state, source);

      // If a sensor has data, process it now.
      if (ident == LOOPER_ID_USER)
      {
        // TODO
      }

      // Check if we are exiting.
      if (state->destroyRequested != 0)
      {
        android_printf("WARNING: We are being destroyed");
        stop = true;
        break;
      }
    }
    
    if (stop)
      break;
  }

  android_printf("DEBUG: Exiting gracefully");
  state->activity->vm->DetachCurrentThread();
  return;
}
