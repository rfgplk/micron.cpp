#pragma once

namespace micron
{
namespace posix
{
constexpr int nr_open = 1024;

constexpr int ngroups_max = 65536;    /* supplemental group ids are available */
constexpr int arg_max = 131072;       /* # bytes of args + environ for exec() */
constexpr int link_max = 127;         /* # links a file may have */
constexpr int max_canon = 255;        /* size of the canonical input queue */
constexpr int max_input = 255;        /* size of the type-ahead buffer */
constexpr int name_max = 255;         /* # chars in a file name */
constexpr int path_max = 4096;        /* # chars in a path name including nul */
constexpr int pipe_buf = 4096;        /* # bytes in atomic write to a pipe */
constexpr int xattr_name_max = 255;   /* # chars in an extended attribute name */
constexpr int xattr_size_max = 65536; /* size of an extended attribute value (64k) */
constexpr int xattr_list_max = 65536; /* size of extended attribute namelist (64k) */

constexpr int rtsig_max = 32;

};
};
