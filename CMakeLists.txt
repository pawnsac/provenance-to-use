################################################################################
# DOC / META
################################################################################

# cmake switches:
#   PRIVATE:   affects .c
#   INTERFACE: affects .h
#   PUBLIC:    affects .h, .c

# minimum cmake version required to build project
cmake_minimum_required(VERSION 3.1.3)

################################################################################
# BUILD INPUTS / OUTPUTS
################################################################################

# cmake project name, source languages
project(ptu_project C)

# source files
file(GLOB ptu_sources "strace-4.6/*.c")
file(GLOB readelf_sources "readelf-mini/*.c")

# libs
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# executable (i.e. "target") to build from source files / libs
add_executable(ptu ${readelf_sources} ${ptu_sources})

# link libs to target
target_link_libraries(ptu Threads::Threads)

################################################################################
# ARCHITECTURE
################################################################################

# set I386 / X86_64 macro for system arch
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  target_compile_definitions(ptu PUBLIC "X86_64=1")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
  target_compile_definitions(ptu PUBLIC "I386=1")
else()
  message(FATAL_ERROR "System architecture not 32-bit or 64-bit (based on size of void*)")
endif()

################################################################################
# OS
################################################################################

# enforce compilation on Linux system due to required libs
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  target_compile_definitions(ptu PUBLIC "LINUX=1")
else()
  message(FATAL_ERROR "PTU requires Linux libraries.")
endif()

################################################################################
# COMPILER OPTIONS
################################################################################

# warnings: enable most warnings for ptu
if(${CMAKE_C_COMPILER_ID} MATCHES "GNU")
  target_compile_options(ptu PUBLIC "-Wall")
endif()

################################################################################
# SOURCE CONFIG
################################################################################

# c language standards for specific sources
set(CMAKE_CSTANDARD_REQURIED ON)
set_source_files_properties(${ptu_sources} PROPERTIES CMAKE_C_STANDARD 11)
set_source_files_properties(${readelf_sources} PROPERTIES CMAKE_C_STANDARD 11)

# system include standards for specific sources
set_source_files_properties(${ptu_sources} PROPERTIES COMPILE_DEFINITIONS "_GNU_SOURCE")
set_source_files_properties(${readelf_sources} PROPERTIES COMPILE_DEFINITIONS "_GNU_SOURCE")

# custom include dirs for specific sources
target_include_directories(ptu PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
target_include_directories(ptu PRIVATE "readelf-mini/include")
target_include_directories(ptu PRIVATE "strace-4.6/linux/x86_64" PRIVATE "strace-4.6/linux")

# silence "unused var" warns on legacy source
if(${CMAKE_C_COMPILER_ID} MATCHES "GNU")
  set_source_files_properties(${readelf_sources} PROPERTIES COMPILE_FLAGS "-Wno-unused-but-set-variable")
  set_source_files_properties("strace-4.6/syscall.c" PROPERTIES COMPILE_FLAGS "-Wno-unused-variable -Wno-unused-but-set-variable")
endif()

# use cmake built-in modules to check availability
# --> checks make defs used by config.h.in to produce config.h

INCLUDE(CheckTypeSize)
INCLUDE(CheckVariableExists)
INCLUDE(CheckFunctionExists)
INCLUDE(CheckIncludeFiles)
INCLUDE(CheckLibraryExists)
INCLUDE(CheckPrototypeDefinition)
INCLUDE(TestBigEndian)

# check type availability
# --> also checks sys/types.h, stdint.h, stddef.h unless BUILTIN_TYPES_ONLY

check_type_size("gid_t" GID_T LANGUAGE C)

set(CMAKE_EXTRA_INCLUDE_FILES "signal.h")
check_type_size("struct ia64_fpreg" STRUCT_IA64_FPREG LANGUAGE C)
set(CMAKE_EXTRA_INCLUDE_FILES)

check_type_size("long" LONG BUILTIN_TYPES_ONLY LANGUAGE C)

check_type_size("long long" LONG_LONG BUILTIN_TYPES_ONLY LANGUAGE C)

check_type_size("mode_t" MODE_T LANGUAGE C)

set(CMAKE_EXTRA_INCLUDE_FILES "sys/socket.h")
check_type_size("((struct msghdr*)0)->msg_control" STRUCT_MSGHDR_MSG_CONTROL LANGUAGE C)
set(CMAKE_EXTRA_INCLUDE_FILES)

check_type_size("off_t" OFF_T LANGUAGE C)

if( HAVE_LONG_LONG AND (OFF_T EQUAL LONG_LONG) AND (OFF_T GREATER LONG) )
  set(HAVE_LONG_LONG_OFF_T 1)
endif()

set(CMAKE_EXTRA_INCLUDE_FILES "asm/stat.h")
check_type_size("struct __old_kernel_stat" STRUCT___OLD_KERNEL_STAT LANGUAGE C)
set(CMAKE_EXTRA_INCLUDE_FILES)

set(CMAKE_EXTRA_INCLUDE_FILES "sys/socket.h")
check_type_size("struct opthdr" STRUCT_OPTHDR LANGUAGE C)
set(CMAKE_EXTRA_INCLUDE_FILES)

set(CMAKE_EXTRA_INCLUDE_FILES "sys/procfs.h")
check_type_size("((struct prstatus*)0)->pr_syscall" PR_SYSCALL LANGUAGE C)
set(CMAKE_EXTRA_INCLUDE_FILES)

set(CMAKE_EXTRA_INCLUDE_FILES "sys/ptrace.h")
check_type_size("struct pt_all_user_regs" STRUCT_PT_ALL_USER_REGS LANGUAGE C)
set(CMAKE_EXTRA_INCLUDE_FILES)

set(CMAKE_EXTRA_INCLUDE_FILES "sys/resource.h")
check_type_size("rlim_t" RLIM_T LANGUAGE C)
set(CMAKE_EXTRA_INCLUDE_FILES)

if( HAVE_RLIM_T AND HAVE_LONG_LONG AND (RLIM_T EQUAL LONG_LONG) AND (RLIM_T GREATER LONG) )
  set(HAVE_LONG_LONG_RLIM_T 1)
endif()

set(CMAKE_EXTRA_INCLUDE_FILES "signal.h")
check_type_size("sig_atomic_t" SIG_ATOMIC_T LANGUAGE C)
set(CMAKE_EXTRA_INCLUDE_FILES)

set(CMAKE_EXTRA_INCLUDE_FILES "asm/sigcontext.h")
check_type_size("struct sigcontext" STRUCT_SIGCONTEXT LANGUAGE C)
set(CMAKE_EXTRA_INCLUDE_FILES)

set(CMAKE_EXTRA_INCLUDE_FILES "asm/sigcontext.h")
check_type_size("struct sigcontext_struct" STRUCT_SIGCONTEXT_STRUCT LANGUAGE C)
set(CMAKE_EXTRA_INCLUDE_FILES)

set(CMAKE_EXTRA_INCLUDE_FILES "signal.h")
check_type_size("siginfo_t" HAVE_SIGINFO_T LANGUAGE C)
set(CMAKE_EXTRA_INCLUDE_FILES)

set(CMAKE_EXTRA_INCLUDE_FILES "netinet/in.h")
check_type_size("((struct sockaddr_in6*)0)->sin6_scope_id" STRUCT_SOCKADDR_IN6_SIN6_SCOPE_ID LANGUAGE C)
set(CMAKE_EXTRA_INCLUDE_FILES)

set(CMAKE_EXTRA_INCLUDE_FILES "sys/stat.h")
check_type_size("((struct stat*)0)->st_aclcnt" STRUCT_STAT_ST_ACLCNT LANGUAGE C)
set(CMAKE_EXTRA_INCLUDE_FILES)

set(CMAKE_EXTRA_INCLUDE_FILES "sys/stat.h")
check_type_size("((struct stat*)0)->st_blksize" STRUCT_STAT_ST_BLKSIZE LANGUAGE C)
set(CMAKE_EXTRA_INCLUDE_FILES)

set(CMAKE_EXTRA_INCLUDE_FILES "sys/stat.h")
check_type_size("((struct stat*)0)->st_blocks" STRUCT_STAT_ST_BLOCKS LANGUAGE C)
set(CMAKE_EXTRA_INCLUDE_FILES)

set(CMAKE_EXTRA_INCLUDE_FILES "sys/stat.h")
check_type_size("((struct stat*)0)->st_flags" STRUCT_STAT_ST_FLAGS LANGUAGE C)
set(CMAKE_EXTRA_INCLUDE_FILES)

set(CMAKE_EXTRA_INCLUDE_FILES "sys/stat.h")
check_type_size("((struct stat*)0)->st_fstype" STRUCT_STAT_ST_FSTYPE LANGUAGE C)
set(CMAKE_EXTRA_INCLUDE_FILES)

set(CMAKE_EXTRA_INCLUDE_FILES "sys/stat.h")
check_type_size("((struct stat*)0)->st_gen" STRUCT_STAT_ST_GEN LANGUAGE C)
set(CMAKE_EXTRA_INCLUDE_FILES)

set(CMAKE_EXTRA_INCLUDE_FILES "sys/stat.h")
check_type_size("((struct stat*)0)->st_level" STRUCT_STAT_ST_LEVEL LANGUAGE C)
set(CMAKE_EXTRA_INCLUDE_FILES)

set(CMAKE_EXTRA_INCLUDE_FILES "sys/stat.h")
check_type_size("((struct stat*)0)->st_rdev" STRUCT_STAT_ST_RDEV LANGUAGE C)
set(CMAKE_EXTRA_INCLUDE_FILES)

set(CMAKE_EXTRA_INCLUDE_FILES "asm/stat.h")
check_type_size("stat64" STAT64 LANGUAGE C)
set(CMAKE_EXTRA_INCLUDE_FILES)

set(CMAKE_EXTRA_INCLUDE_FILES "sys/vfs.h" "sys/statfs.h")
check_type_size("statfs64" STATFS64 LANGUAGE C)
set(CMAKE_EXTRA_INCLUDE_FILES)

set(CMAKE_EXTRA_INCLUDE_FILES "sys/stream.h")
check_type_size("((struct T_conn_res*)0)->ACCEPTOR_id" STRUCT_T_CONN_RES_ACCEPTOR_ID LANGUAGE C)
set(CMAKE_EXTRA_INCLUDE_FILES)

set(CMAKE_EXTRA_INCLUDE_FILES "sys/stream.h")
check_type_size("((struct T_conn_res*)0)->QUEUE_ptr" STRUCT_T_CONN_RES_QUEUE_PTR LANGUAGE C)
set(CMAKE_EXTRA_INCLUDE_FILES)

set(CMAKE_EXTRA_INCLUDE_FILES "sys/tiuser.h")
check_type_size("struct t_opthdr" STRUCT_T_OPTHDR LANGUAGE C)
set(CMAKE_EXTRA_INCLUDE_FILES)

check_type_size("uid_t" UID_T LANGUAGE C)

set(CMAKE_EXTRA_INCLUDE_FILES "asm/ldt.h")
check_type_size("struct user_desc" STRUCT_USER_DESC LANGUAGE C)
set(CMAKE_EXTRA_INCLUDE_FILES)

# type replacements (for unavailable types)

if(HAVE_GID_T)
  target_compile_definitions(ptu PUBLIC "GETGROUPS_T=gid_t")
else()
  target_compile_definitions(ptu PUBLIC "GETGROUPS_T=int")
  target_compile_definitions(ptu PUBLIC "gid_t=int")
endif()

if(NOT HAVE_MODE_T)
  target_compile_definitions(ptu PUBLIC "mode_t=int")
endif()

if(NOT HAVE_UID_T)
  target_compile_definitions(ptu PUBLIC "uid_t=int")
endif()

# check var availability

check_variable_exists("environ" HAVE_DECL_ENVIRON)

set(CMAKE_EXTRA_INCLUDE_FILES "mkdev.h")
check_variable_exists("major" HAVE_MKDEV_MAJOR)
check_variable_exists("minor" HAVE_MKDEV_MINOR)
check_variable_exists("makedev" HAVE_MKDEV_MAKEDEV)
if( HAVE_MKDEV_MAJOR AND HAVE_MKDEV_MINOR AND HAVE_MKDEV_MAKEDEV)
  set(MAJOR_IN_MKDEV 1)
endif()
set(CMAKE_EXTRA_INCLUDE_FILES)

set(CMAKE_EXTRA_INCLUDE_FILES "sysmacros.h")
check_variable_exists("major" HAVE_SYSMACROS_MAJOR)
check_variable_exists("minor" HAVE_SYSMACROS_MINOR)
check_variable_exists("makedev" HAVE_SYSMACROS_MAKEDEV)
if( HAVE_SYSMACROS_MAJOR AND HAVE_SYSMACROS_MINOR AND HAVE_SYSMACROS_MAKEDEV)
  set(MAJOR_IN_SYSMACROS 1)
endif()
set(CMAKE_EXTRA_INCLUDE_FILES)

check_variable_exists("sys_siglist" HAVE_DECL_SYS_SIGLIST)

check_variable_exists("_sys_siglist" HAVE_DECL__SYS_SIGLIST)

# check function availability

check_function_exists("alloca" HAVE_ALLOCA)
check_function_exists("fork" HAVE_FORK)
check_function_exists("fprintf" HAVE_DECL_FPRINTF)
check_function_exists("getenv" HAVE_DECL_GETENV)
check_function_exists("getopt" HAVE_DECL_GETOPT)
check_function_exists("if_indextoname" HAVE_IF_INDEXTONAME)
check_function_exists("inet_ntop" HAVE_INET_NTOP)
check_function_exists("prctl" HAVE_PRCTL)
check_function_exists("sbrk" HAVE_DECL_SBRK)
check_function_exists("sbrk" HAVE_SBRK)
check_function_exists("sendmsg" HAVE_SENDMSG)
check_function_exists("sigaction" HAVE_SIGACTION)
check_function_exists("snprintf" HAVE_DECL_SNPRINTF)
check_function_exists("stpcpy" HAVE_DECL_STPCPY)
check_function_exists("strerror" HAVE_STRERROR)
check_function_exists("strsignal" HAVE_STRSIGNAL)
check_function_exists("strstr" HAVE_DECL_STRSTR)
check_function_exists("sys_errlist" HAVE_DECL_SYS_ERRLIST)
check_function_exists("sys_siglist" HAVE_SYS_SIGLIST)
check_function_exists("_sys_siglist" HAVE__SYS_SIGLIST)
check_function_exists("vsnprintf" HAVE_DECL_VSNPRINTF)

# check include availability

list(APPEND STDC_LIST "assert.h;complex.h;ctype.h;errno.h;fenv.h;float.h;")
list(APPEND STDC_LIST "inttypes.h;iso646.h;limits.h;locale.h;math.h;setjmp.h;")
list(APPEND STDC_LIST "signal.h;stdarg.h;stdbool.h;stddef.h;stdio.h;stdlib.h;")
list(APPEND STDC_LIST "string.h;tgmath.h;time.h;wchar.h;wctype.h")
check_include_files("${STDC_LIST}" STDC_HEADERS)

check_include_files("alloca.h" HAVE_ALLOCA_H)
check_include_files("asm/cachectl.h" HAVE_ASM_CACHECTL_H)
check_include_files("asm/sigcontext.h" HAVE_ASM_SIGCONTEXT_H)
check_include_files("asm/sysmips.h" HAVE_ASM_SYSMIPS_H)
check_include_files("fcntl.h" HAVE_FCNTL_H)
check_include_files("inttypes.h" HAVE_INTTYPES_H)
check_include_files("ioctls.h" HAVE_IOCTLS_H)
check_include_files("libaio.h" HAVE_LIBAIO_H)
check_include_files("libintl.h" HAVE_LIBINTL_H)
check_include_files("limits.h" HAVE_LIMITS_H)
check_include_files("linux/capability.h" HAVE_LINUX_CAPABILITY_H)
check_include_files("linux/icmp.h" HAVE_LINUX_ICMP_H)
check_include_files("linux/if_packet.h" HAVE_LINUX_IF_PACKET_H)
check_include_files("linux/in6.h" HAVE_LINUX_IN6_H)
check_include_files("linux/netlink.h" HAVE_LINUX_NETLINK_H)
check_include_files("linux/ptrace.h" HAVE_LINUX_PTRACE_H)
check_include_files("mqueue.h" HAVE_MQUEUE_H)
check_include_files("netinet/sctp.h" HAVE_NETINET_SCTP_H)
check_include_files("netinet/tcp.h" HAVE_NETINET_TCP_H)
check_include_files("netinet/udp.h" HAVE_NETINET_UDP_H)
check_include_files("poll.h" HAVE_POLL_H)
check_include_files("stdbool.h" HAVE_STDBOOL_H)
check_include_files("stdint.h" HAVE_STDINT_H)
check_include_files("stdlib.h" HAVE_STDLIB_H)
check_include_files("string.h" HAVE_STRING_H)
check_include_files("strings.h" HAVE_STRINGS_H)
check_include_files("stropts.h" HAVE_STROPTS_H)
check_include_files("sys/acl.h" HAVE_SYS_ACL_H)
check_include_files("sys/aio.h" HAVE_SYS_AIO_H)
check_include_files("sys/asynch.h" HAVE_SYS_ASYNCH_H)
check_include_files("sys/conf.h" HAVE_SYS_CONF_H)
check_include_files("sys/door.h" HAVE_SYS_DOOR_H)
check_include_files("sys/epoll.h" HAVE_SYS_EPOLL_H)
check_include_files("sys/file.h" HAVE_SYS_FILE_H)
check_include_files("sys/filio.h" HAVE_SYS_FILIO_H)
check_include_files("sys/ioctl.h" HAVE_SYS_IOCTL_H)
check_include_files("sys/nscsys.h" HAVE_SYS_NSCSYS_H)
check_include_files("sys/param.h" HAVE_SYS_PARAM_H)
check_include_files("sys/poll.h" HAVE_SYS_POLL_H)
check_include_files("sys/reg.h" HAVE_SYS_REG_H)
check_include_files("sys/stream.h" HAVE_SYS_STREAM_H)
check_include_files("sys/sysconfig.h" HAVE_SYS_SYSCONFIG_H)
check_include_files("sys/tihdr.h" HAVE_SYS_TIHDR_H)
check_include_files("sys/tiuser.h" HAVE_SYS_TIUSER_H)
check_include_files("sys/uio.h" HAVE_SYS_UIO_H)
check_include_files("sys/vfs.h" HAVE_SYS_VFS_H)
check_include_files("termio.h" HAVE_TERMIO_H)
check_include_files("unistd.h" HAVE_UNISTD_H)
check_include_files("zlib.h" HAVE_ZLIB_H)

# check linked library availability

check_library_exists("nsl" "gethostbyname" "" HAVE_LIBNSL)

# check misc conditions

test_big_endian(HAVE_BIG_ENDIAN)
if( (NOT HAVE_BIG_ENDIAN) AND HAVE_LONG_LONG )
  set(HAVE_LITTLE_ENDIAN_LONG_LONG 1)
endif()

if(HAVE_LIBINTL_H)
  set(ENABLE_NLS 1)
endif()

# process all the defs made by all the checks

# def all the availables in config.h (using rules in config.h.in)
configure_file("${PROJECT_SOURCE_DIR}/config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.h")

# def that config.h was generated by cmake
target_compile_definitions(ptu PUBLIC "HAVE_CONFIG_H=1")

