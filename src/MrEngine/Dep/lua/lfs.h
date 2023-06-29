/*
** LuaFileSystem
** Copyright Kepler Project 2003 - 2017 (http://keplerproject.github.io/luafilesystem)
*/

/* Define 'chdir' for systems that do not implement it */
#ifdef NO_CHDIR
  #define chdir(p)	(-1)
  #define chdir_error	"Function 'chdir' not provided by system"
#else
  #define chdir_error	strerror(errno)
#endif

#ifdef _WIN32
  #define chdir(p) (_chdir(p))
  #define getcwd(d, s) (_getcwd(d, s))
  #define rmdir(p) (_rmdir(p))
  #define LFS_EXPORT __declspec (dllexport)
  #ifndef fileno
    #define fileno(f) (_fileno(f))
  #endif
#else
  #define LFS_EXPORT
#endif

namespace OrangeFilter {

#ifdef __cplusplus
#ifndef OF_LUA_CPP
extern "C" {
#endif
#endif

LUAMOD_API  int luaopen_lfs (lua_State *L);

#ifdef __cplusplus
#ifndef OF_LUA_CPP
}
#endif

} // namespace OrangeFilter {

#endif
