#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

// Calls into BinderFilter kernel driver on behalf of binderfilter.py (which cannot make these kernel calls)
// based on the Android JNI/NDK version https://github.com/dxwu/Picky/blob/master/app/src/main/jni/picky-jni.c 
// no validation, no frills

// based on ../src/binder_filter.h
struct bf_user_filter {
    int action;
    int uid;
    char* message;
    char* data;

    int context;
    int context_type;
    int context_int_value;
    char* context_string_value;
};

#define FILTER_ENABLE_FAILED -1
#define CHMOD_FAILED -2
#define CHCON_FAILED -3
#define OPEN_FAILED -4
#define CREATE_POLICY_FILE_FAILED -5

int fd = FILTER_ENABLE_FAILED;

int setup_dev_permissions() {
     // enable binderfilter
     if (popen("adb shell \"su -c \'echo 1 > /sys/module/binder_filter/parameters/filter_enable\'\"", "r") == NULL) {
          return FILTER_ENABLE_FAILED;
     }

     // chmod binderfilter
     if (popen("adb shell \"su -c \'chmod 666 /dev/binderfilter\'\"", "r") == NULL) {
          return CHMOD_FAILED;
     }

     // change SELinux policy to allow our driver (take on type of binder driver)
     if (popen("adb shell \"su -c \'chcon u:object_r:binder_device:s0 /dev/binderfilter\'\"", "r") == NULL) {
          return CHCON_FAILED;
     }

     // touch persistent policy file
     if (popen("adb shell \"su -c \'if [ ! -f /data/local/tmp/bf.policy ]; then touch /data/local/tmp/bf.policy; chmod 777 /data/local/tmp/bf.policy; fi\'\"", "r") == NULL) {
          return CREATE_POLICY_FILE_FAILED;
     }

     // open driver
     fd = open("/dev/binderfilter", O_RDWR);
     if (fd >= 0) {
          return fd;
     } else {
          return OPEN_FAILED;
     }
}

int main (int argc, char **argv)
{
     char *message = NULL;
     char *uid = NULL;
     char *action = NULL;
     char *modifyData = NULL;
     char *context = NULL;
     char *contextType = NULL;
     char *contextValue = NULL;

     int c;
     while ((c = getopt (argc, argv, "m:u:a:d:c:t:v:")) != -1) {
          switch (c)
          {
          case 'm':
               message = optarg;
               break;
          case 'u':
               uid = optarg;
               break;
          case 'a':
               action = optarg;
               break;
          case 'd':
               modifyData = optarg;
               break;
          case 'c':
               context = optarg;
               break;
          case 't':
               contextType = optarg;
               break;
          case 'v':
               contextValue = optarg;
               break;
          case '?':
               if (isprint (optopt)) {
                    fprintf (stderr, "Unknown option `-%c'.\n", optopt);
               }
               else {
                    fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
               }
               return 1;
          default:
               abort ();
          }
     }

     printf("message: %s, uid: %s, action: %s, modifyData: %s, context: %s, contextType: %s, contextValue: %s\n",
             message, uid, action, modifyData, context, contextType, contextValue);


     if (fd < 0) {
          int ret = setup_dev_permissions();
          if(ret < 0) {
               printf("Setup failed with return code %d\n", ret);
               exit(1);
          }
     }

     return 0;
}
