#

isEmpty(PREFIX) {
  unix:PREFIX = /usr/local
  win32:PREFIX = .
}

TEMPLATE = subdirs
CONFIG   += ordered
SUBDIRS  += qextserialport qdlt src plugin
CONFIG += c++11
