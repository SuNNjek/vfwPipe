#ifndef __VERSION_H__
#define __VERSION_H__

#define STRINGIZEMACRO(s) #s
#define STRINGIZE(s) STRINGIZEMACRO(s)

//Build number and hash automatically inserted by .targets-file, include version_tmp.h instead of version.h and ignore the errors, they'll disappear once you build it :P

#define VFWPIPE_VERSION_MAJOR 1
#define VFWPIPE_VERSION_MINOR 0
#define VFWPIPE_VERSION_PATCH 1
#define VFWPIPE_VERSION_BUILD $version$

//Hash of last git commit to uniquely identify version
#define VFWPIPE_VERSION_HASH "$hash$"

#define VFWPIPE_VERSION_STRING "v" STRINGIZE(VFWPIPE_VERSION_MAJOR) "." STRINGIZE(VFWPIPE_VERSION_MINOR) "." STRINGIZE(VFWPIPE_VERSION_PATCH) "." STRINGIZE(VFWPIPE_VERSION_BUILD)
#define VFWPIPE_VERSION_STRING_WITH_HASH VFWPIPE_VERSION_STRING "-" VFWPIPE_VERSION_HASH

#define VFWPIPE_AUTHOR "Sunner"
#define VFWPIPE_AUTHOR_MAIL "sunnerlp@gmail.com"
#define VFWPIPE_AUTHOR_STRING VFWPIPE_AUTHOR " (" VFWPIPE_AUTHOR_MAIL ")"

#endif // !__VERSION_H__
