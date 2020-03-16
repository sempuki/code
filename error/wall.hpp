#pragma once

#include <experimental/source_location>
#include <map>
#include <string_view>

namespace std {
using experimental::source_location;
}

namespace ctl {

class wall;

class enclosure {
 public:
  virtual bool equivalent(wall const& this_wall, wall const& that_wall,
                          enclosure const& that_enclosure) = 0;

  virtual std::string const& message(wall const& wall) = 0;
  virtual std::source_location const& location(wall const& wall) = 0;

 protected:
  struct wall_code {
    int16_t kind;
    int16_t inst;
  };

  wall create(wall_code const& wall);
  wall_code reveal(wall const& wall);
};

class wall final {
 public:
  wall() = default;
  ~wall() = default;

  wall(wall const&) = default;
  wall& operator=(wall const&) = default;

  explicit operator bool() const noexcept { enclosure_ != nullptr; }

  bool operator==(wall const& that) const noexcept {
    return (enclosure_ == that.enclosure_ && kind_ == that.kind_);
  }

  std::string const& message() const noexcept {
    return enclosure_->message(*this);
  }

  std::source_location const& location() const noexcept {
    return enclosure_->location(*this);
  }

 private:
  friend enclosure;

  wall(enclosure* enclosure, int16_t kind, int16_t inst)
      : enclosure_{enclosure}, kind_{kind}, instance_{inst} {}

  enclosure* enclosure_ = nullptr;
  int16_t kind_;
  int16_t instance_;
};

struct kind_entry {
  // ~64 characters?
  std::string message;
};

struct inst_entry {
  std::source_location location;
  std::string extra;
};

class posix_enclosure final : public enclosure {
 public:
  wall raise(int error, std::source_location location,
             std::string_view extra = {}) {
    wall wall = create(enclosure::wall_code{
        static_cast<int16_t>(error), static_cast<int16_t>(next_instance_)});
    instances_[next_instance_].location = location;
    instances_[next_instance_].extra = extra;
    next_instance_++;
    return wall;
  }

  virtual bool equivalent(wall const& this_wall, wall const& that_wall,
                          enclosure const& that_enclosure) {
    return false;
  }

  std::string const& message(wall const& wall) override {
    auto [kind, _] = enclosure::reveal(wall);
    return kinds_[kind].message;
  }

  std::source_location const& location(wall const& wall) override {
    auto [_, inst] = enclosure::reveal(wall);
    return instances_[inst].location;
  }

 private:
  static std::array<kind_entry, 125> kinds_;

  std::array<inst_entry, 128> instances_;
  std::size_t next_instance_ = 0;
};

// class win32_enclosure final : public enclosure {
//  public:
//   virtual bool equivalent(wall const& this_wall, wall const& that_wall,
//                           enclosure const& that_enclosure) { return false; }
//
//   virtual std::source_location const& location(wall const& wall) { return ; }
//
//  private:
//   std::array<kind_entry, 301> kinds_;
//   std::array<inst_entry, 128> instances_;
// };

}  // namespace ctl

/* Win32                                       // POSIX
0    ERROR_SUCCESS                             EAFNOSUPPORT
1    ERROR_INVALID_FUNCTION                    EADDRINUSE
2    ERROR_FILE_NOT_FOUND                      EADDRNOTAVAIL
3    ERROR_PATH_NOT_FOUND                      EISCONN
4    ERROR_TOO_MANY_OPEN_FILES                 E2BIG
5    ERROR_ACCESS_DENIED                       EDOM
6    ERROR_INVALID_HANDLE                      EFAULT
7    ERROR_ARENA_TRASHED                       EBADF
8    ERROR_NOT_ENOUGH_MEMORY                   EBADMSG
9    ERROR_INVALID_BLOCK                       EPIPE
10   ERROR_BAD_ENVIRONMENT                     ECONNABORTED
11   ERROR_BAD_FORMAT                          EALREADY
12   ERROR_INVALID_ACCESS                      ECONNREFUSED
13   ERROR_INVALID_DATA                        ECONNRESET
14   ERROR_OUTOFMEMORY                         EXDEV
15   ERROR_INVALID_DRIVE                       EDESTADDRREQ
16   ERROR_CURRENT_DIRECTORY                   EBUSY
17   ERROR_NOT_SAME_DEVICE                     ENOTEMPTY
18   ERROR_NO_MORE_FILES                       ENOEXEC
19   ERROR_WRITE_PROTECT                       EEXIST
20   ERROR_BAD_UNIT                            EFBIG
21   ERROR_NOT_READY                           ENAMETOOLONG
22   ERROR_BAD_COMMAND                         ENOSYS
23   ERROR_CRC                                 EHOSTUNREACH
24   ERROR_BAD_LENGTH                          EIDRM
25   ERROR_SEEK                                EILSEQ
26   ERROR_NOT_DOS_DISK                        ENOTTY
27   ERROR_SECTOR_NOT_FOUND                    EINTR
28   ERROR_OUT_OF_PAPER                        EINVAL
29   ERROR_WRITE_FAULT                         ESPIPE
30   ERROR_READ_FAULT                          EIO
31   ERROR_GEN_FAILURE                         EISDIR
32   ERROR_SHARING_VIOLATION                   EMSGSIZE
33   ERROR_LOCK_VIOLATION                      ENETDOWN
34   ERROR_WRONG_DISK                          ENETRESET
36   ERROR_SHARING_BUFFER_EXCEEDED             ENETUNREACH
38   ERROR_HANDLE_EOF                          ENOBUFS
39   ERROR_HANDLE_DISK_FULL                    ECHILD
50   ERROR_NOT_SUPPORTED                       ENOLINK
51   ERROR_REM_NOT_LIST                        ENOLCK
52   ERROR_DUP_NAME                            ENODATA
53   ERROR_BAD_NETPATH                         ENOMSG
54   ERROR_NETWORK_BUSY                        ENOPROTOOPT
55   ERROR_DEV_NOT_EXIST                       ENOSPC
56   ERROR_TOO_MANY_CMDS                       ENOSR
57   ERROR_ADAP_HDW_ERR                        ENXIO
58   ERROR_BAD_NET_RESP                        ENODEV
59   ERROR_UNEXP_NET_ERR                       ENOENT
60   ERROR_BAD_REM_ADAP                        ESRCH
61   ERROR_PRINTQ_FULL                         ENOTDIR
62   ERROR_NO_SPOOL_SPACE                      ENOTSOCK
63   ERROR_PRINT_CANCELED                      ENOSTR
64   ERROR_NETNAME_DELETED                     ENOTCONN
65   ERROR_NETWORK_ACCESS_DENIED               ENOMEM
66   ERROR_BAD_DEV_TYPE                        ENOTSUP
67   ERROR_BAD_NET_NAME                        ECANCELED
68   ERROR_TOO_MANY_NAMES                      EINPROGRESS
69   ERROR_TOO_MANY_SESS                       EPERM
70   ERROR_SHARING_PAUSED                      EOPNOTSUPP
71   ERROR_REQ_NOT_ACCEP                       EWOULDBLOCK
72   ERROR_REDIR_PAUSED                        EOWNERDEAD
80   ERROR_FILE_EXISTS                         EACCES
82   ERROR_CANNOT_MAKE                         EPROTO
83   ERROR_FAIL_I24                            EPROTONOSUPPORT
84   ERROR_OUT_OF_STRUCTURES                   EROFS
85   ERROR_ALREADY_ASSIGNED                    EDEADLK
86   ERROR_INVALID_PASSWORD                    EAGAIN
87   ERROR_INVALID_PARAMETER                   ERANGE
88   ERROR_NET_WRITE_FAULT                     ENOTRECOVERABLE
89   ERROR_NO_PROC_SLOTS                       ETIME
100  ERROR_TOO_MANY_SEMAPHORES                 ETXTBSY
101  ERROR_EXCL_SEM_ALREADY_OWNED              ETIMEDOUT
102  ERROR_SEM_IS_SET                          ENFILE
103  ERROR_TOO_MANY_SEM_REQUESTS               EMFILE
104  ERROR_INVALID_AT_INTERRUPT_TIME           EMLINK
105  ERROR_SEM_OWNER_DIED                      ELOOP
106  ERROR_SEM_USER_LIMIT                      EOVERFLOW
107  ERROR_DISK_CHANGE                         EPROTOTYPE
108  ERROR_DRIVE_LOCKED
109  ERROR_BROKEN_PIPE
110  ERROR_OPEN_FAILED
111  ERROR_BUFFER_OVERFLOW
112  ERROR_DISK_FULL
113  ERROR_NO_MORE_SEARCH_HANDLES
114  ERROR_INVALID_TARGET_HANDLE
117  ERROR_INVALID_CATEGORY
118  ERROR_INVALID_VERIFY_SWITCH
119  ERROR_BAD_DRIVER_LEVEL
120  ERROR_CALL_NOT_IMPLEMENTED
121  ERROR_SEM_TIMEOUT
122  ERROR_INSUFFICIENT_BUFFER
123  ERROR_INVALID_NAME
124  ERROR_INVALID_LEVEL
125  ERROR_NO_VOLUME_LABEL
126  ERROR_MOD_NOT_FOUND
127  ERROR_PROC_NOT_FOUND
128  ERROR_WAIT_NO_CHILDREN
129  ERROR_CHILD_NOT_COMPLETE
130  ERROR_DIRECT_ACCESS_HANDLE
131  ERROR_NEGATIVE_SEEK
132  ERROR_SEEK_ON_DEVICE
133  ERROR_IS_JOIN_TARGET
134  ERROR_IS_JOINED
135  ERROR_IS_SUBSTED
136  ERROR_NOT_JOINED
137  ERROR_NOT_SUBSTED
138  ERROR_JOIN_TO_JOIN
139  ERROR_SUBST_TO_SUBST
140  ERROR_JOIN_TO_SUBST
141  ERROR_SUBST_TO_JOIN
142  ERROR_BUSY_DRIVE
143  ERROR_SAME_DRIVE
144  ERROR_DIR_NOT_ROOT
145  ERROR_DIR_NOT_EMPTY
146  ERROR_IS_SUBST_PATH
147  ERROR_IS_JOIN_PATH
148  ERROR_PATH_BUSY
149  ERROR_IS_SUBST_TARGET
150  ERROR_SYSTEM_TRACE
151  ERROR_INVALID_EVENT_COUNT
152  ERROR_TOO_MANY_MUXWAITERS
153  ERROR_INVALID_LIST_FORMAT
154  ERROR_LABEL_TOO_LONG
155  ERROR_TOO_MANY_TCBS
156  ERROR_SIGNAL_REFUSED
157  ERROR_DISCARDED
158  ERROR_NOT_LOCKED
159  ERROR_BAD_THREADID_ADDR
160  ERROR_BAD_ARGUMENTS
161  ERROR_BAD_PATHNAME
162  ERROR_SIGNAL_PENDING
164  ERROR_MAX_THRDS_REACHED
167  ERROR_LOCK_FAILED
170  ERROR_BUSY
173  ERROR_CANCEL_VIOLATION
174  ERROR_ATOMIC_LOCKS_NOT_SUPPORTED
180  ERROR_INVALID_SEGMENT_NUMBER
182  ERROR_INVALID_ORDINAL
183  ERROR_ALREADY_EXISTS
186  ERROR_INVALID_FLAG_NUMBER
187  ERROR_SEM_NOT_FOUND
188  ERROR_INVALID_STARTING_CODESEG
189  ERROR_INVALID_STACKSEG
190  ERROR_INVALID_MODULETYPE
191  ERROR_INVALID_EXE_SIGNATURE
192  ERROR_EXE_MARKED_INVALID
193  ERROR_BAD_EXE_FORMAT
194  ERROR_ITERATED_DATA_EXCEEDS_64k
195  ERROR_INVALID_MINALLOCSIZE
196  ERROR_DYNLINK_FROM_INVALID_RING
197  ERROR_IOPL_NOT_ENABLED
198  ERROR_INVALID_SEGDPL
199  ERROR_AUTODATASEG_EXCEEDS_64k
200  ERROR_RING2SEG_MUST_BE_MOVABLE
201  ERROR_RELOC_CHAIN_XEEDS_SEGLIM
202  ERROR_INFLOOP_IN_RELOC_CHAIN
203  ERROR_ENVVAR_NOT_FOUND
205  ERROR_NO_SIGNAL_SENT
206  ERROR_FILENAME_EXCED_RANGE
207  ERROR_RING2_STACK_IN_USE
208  ERROR_META_EXPANSION_TOO_LONG
209  ERROR_INVALID_SIGNAL_NUMBER
210  ERROR_THREAD_1_INACTIVE
212  ERROR_LOCKED
214  ERROR_TOO_MANY_MODULES
215  ERROR_NESTING_NOT_ALLOWED
216  ERROR_EXE_MACHINE_TYPE_MISMATCH
230  ERROR_BAD_PIPE
231  ERROR_PIPE_BUSY
232  ERROR_NO_DATA
233  ERROR_PIPE_NOT_CONNECTED
234  ERROR_MORE_DATA
240  ERROR_VC_DISCONNECTED
254  ERROR_INVALID_EA_NAME
255  ERROR_EA_LIST_INCONSISTENT
258  WAIT_TIMEOUT
259  ERROR_NO_MORE_ITEMS
266  ERROR_CANNOT_COPY
267  ERROR_DIRECTORY
275  ERROR_EAS_DIDNT_FIT
276  ERROR_EA_FILE_CORRUPT
277  ERROR_EA_TABLE_FULL
278  ERROR_INVALID_EA_HANDLE
282  ERROR_EAS_NOT_SUPPORTED
288  ERROR_NOT_OWNER
298  ERROR_TOO_MANY_POSTS
299  ERROR_PARTIAL_COPY
300  ERROR_OPLOCK_NOT_GRANTED
301  ERROR_INVALID_OPLOCK_PROTOCOL
317  ERROR_MR_MID_NOT_FOUND
487  ERROR_INVALID_ADDRESS
534  ERROR_ARITHMETIC_OVERFLOW
535  ERROR_PIPE_CONNECTED
536  ERROR_PIPE_LISTENING
994  ERROR_EA_ACCESS_DENIED
995  ERROR_OPERATION_ABORTED
996  ERROR_IO_INCOMPLETE
997  ERROR_IO_PENDING
998  ERROR_NOACCESS
999  ERROR_SWAPERROR
*/
