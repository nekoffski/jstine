# This file is generated. Do not edit manually.
# Source: conf/errors.in

from enum import IntEnum


class ErrorCode(IntEnum):
    noError = 0
    badInput = 1
    badConfig = 2
    fileSystemError = 3
    invalidArgument = 4
    osError = 5
    handshakeFailure = 6
    requestNotReady = 7
    eof = 8
    connectionFailure = 9
