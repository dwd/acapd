cmake_minimum_required(VERSION 2.8)
project(acapd)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -pedantic -O0 -g -pthread")

set(SOURCE_UTILS_FILES
        src/impl/utils/base64.cc
        src/impl/utils/cast-exception.cc
        src/impl/utils/memsuite.cc
        src/impl/utils/stringrep.cc
        src/impl/utils/utf8.cc
        src/impl/utils/xml-parser.cc)
set(SOURCE_THREADING_FILES
        src/impl/threading/thread.cc)
set(SOURCE_STREAM_FILES
        src/impl/stream/fdstreambuf.cc
        src/impl/stream/saslstreambuf.cc
        src/impl/stream/tcpstream.cc
        src/impl/stream/tlsstreambuf.cc)
set(SOURCE_SERVER_FILES
        src/impl/server/command.cc
        src/impl/server/context.cc
        src/impl/server/log.cc
        src/impl/server/master.cc
        src/impl/server/return.cc
        src/impl/server/session.cc
        src/impl/server/token.cc
        src/impl/server/worker.cc)
set(SOURCE_META_FILES
        src/impl/meta/meta.cc)
set(SOURCE_DATASTORE_FILES
        src/impl/datastore/acl.cc
        src/impl/datastore/attribute.cc
        src/impl/datastore/authz.cc
        src/impl/datastore/comparator.cc
        src/impl/datastore/constants.cc
        src/impl/datastore/criteria.cc
        src/impl/datastore/dataset.cc
        src/impl/datastore/datastore.cc
        src/impl/datastore/entry.cc
        src/impl/datastore/exceptions.cc
        src/impl/datastore/modtime.cc
        src/impl/datastore/notify-sink.cc
        src/impl/datastore/notify-source.cc
        src/impl/datastore/path.cc
        src/impl/datastore/sort.cc
        src/impl/datastore/subcontext.cc
        src/impl/datastore/trans-logger.cc
        src/impl/datastore/transaction.cc
        src/impl/datastore/xml-dump.cc
        src/impl/datastore/xml-loader.cc)
set(SOURCE_COMMAND_FILES
        src/impl/commands/authenticate.cc
        src/impl/commands/compress.cc
        src/impl/commands/deleteacl.cc
        src/impl/commands/deletedsince.cc
        src/impl/commands/freecontext.cc
        src/impl/commands/getquota.cc
        src/impl/commands/listrights.cc
        src/impl/commands/logout.cc
        src/impl/commands/myrights.cc
        src/impl/commands/noop.cc
        src/impl/commands/path.cc
        src/impl/commands/search.cc
        src/impl/commands/setacl.cc
        src/impl/commands/sleep.cc
        src/impl/commands/starttls.cc
        src/impl/commands/store.cc
        src/impl/commands/sync-sleep.cc
        src/impl/commands/updatecontext.cc)

include_directories(src/include)

add_executable(acapd-real ${SOURCE_COMMAND_FILES} ${SOURCE_DATASTORE_FILES} ${SOURCE_SERVER_FILES} ${SOURCE_STREAM_FILES} ${SOURCE_THREADING_FILES} ${SOURCE_UTILS_FILES})
target_link_libraries(acapd-real ssl crypto expat sasl2 z)
add_executable(acapd ${SOURCE_META_FILES})
