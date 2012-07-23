#!/bin/sh
cd src/main/cpp
g++ -shared -nostdlib /usr/lib/gcc/x86_64-redhat-linux/4.1.2/../../../../lib64/crti.o /usr/lib/gcc/x86_64-redhat-linux/4.1.2/crtbeginS.o \
.libs/action.o .libs/appenderattachableimpl.o .libs/appenderskeleton.o .libs/aprinitializer.o .libs/asyncappender.o .libs/basicconfigurator.o \
.libs/bufferedwriter.o .libs/bytearrayinputstream.o .libs/bytearrayoutputstream.o .libs/bytebuffer.o .libs/cacheddateformat.o \
.libs/charsetdecoder.o .libs/charsetencoder.o .libs/class.o .libs/classnamepatternconverter.o .libs/classregistration.o .libs/condition.o \
.libs/configurator.o .libs/consoleappender.o .libs/cyclicbuffer.o .libs/dailyrollingfileappender.o .libs/datagrampacket.o .libs/datagramsocket.o \
.libs/date.o .libs/dateformat.o .libs/datelayout.o .libs/datepatternconverter.o .libs/defaultloggerfactory.o .libs/defaultconfigurator.o \
.libs/defaultrepositoryselector.o .libs/domconfigurator.o .libs/exception.o .libs/fallbackerrorhandler.o .libs/file.o .libs/fileappender.o \
.libs/filedatepatternconverter.o .libs/fileinputstream.o .libs/filelocationpatternconverter.o .libs/fileoutputstream.o .libs/filerenameaction.o \
.libs/filewatchdog.o .libs/filter.o .libs/filterbasedtriggeringpolicy.o .libs/fixedwindowrollingpolicy.o .libs/formattinginfo.o \
.libs/fulllocationpatternconverter.o .libs/gzcompressaction.o .libs/hierarchy.o .libs/htmllayout.o .libs/inetaddress.o .libs/inputstream.o \
.libs/inputstreamreader.o .libs/integer.o .libs/integerpatternconverter.o .libs/layout.o .libs/level.o .libs/levelmatchfilter.o \
.libs/levelrangefilter.o .libs/levelpatternconverter.o .libs/linelocationpatternconverter.o .libs/lineseparatorpatternconverter.o \
.libs/literalpatternconverter.o .libs/loggerpatternconverter.o .libs/loggingeventpatternconverter.o .libs/loader.o .libs/locale.o \
.libs/locationinfo.o .libs/logger.o .libs/loggingevent.o .libs/loglog.o .libs/logmanager.o .libs/logstream.o .libs/manualtriggeringpolicy.o \
.libs/messagebuffer.o .libs/messagepatternconverter.o .libs/methodlocationpatternconverter.o .libs/mdc.o .libs/mutex.o .libs/nameabbreviator.o \
.libs/namepatternconverter.o .libs/ndcpatternconverter.o .libs/ndc.o .libs/nteventlogappender.o .libs/objectimpl.o .libs/objectptr.o \
.libs/objectoutputstream.o .libs/obsoleterollingfileappender.o .libs/odbcappender.o .libs/onlyonceerrorhandler.o .libs/optionconverter.o \
.libs/outputdebugstringappender.o .libs/outputstream.o .libs/outputstreamwriter.o .libs/patternconverter.o .libs/patternlayout.o \
.libs/patternparser.o .libs/pool.o .libs/properties.o .libs/propertiespatternconverter.o .libs/propertyconfigurator.o \
.libs/propertyresourcebundle.o .libs/propertysetter.o .libs/reader.o .libs/relativetimedateformat.o .libs/relativetimepatternconverter.o \
.libs/resourcebundle.o .libs/rollingfileappender.o .libs/rollingpolicy.o .libs/rollingpolicybase.o .libs/rolloverdescription.o .libs/rootlogger.o \
.libs/serversocket.o .libs/simpledateformat.o .libs/simplelayout.o .libs/sizebasedtriggeringpolicy.o .libs/smtpappender.o .libs/socket.o \
.libs/socketappender.o .libs/socketappenderskeleton.o .libs/sockethubappender.o .libs/socketoutputstream.o .libs/strftimedateformat.o \
.libs/stringhelper.o .libs/stringmatchfilter.o .libs/stringtokenizer.o .libs/synchronized.o .libs/syslogappender.o .libs/syslogwriter.o \
.libs/system.o .libs/systemerrwriter.o .libs/systemoutwriter.o .libs/telnetappender.o .libs/threadcxx.o .libs/threadlocal.o \
.libs/threadspecificdata.o .libs/threadpatternconverter.o .libs/throwableinformationpatternconverter.o .libs/timezone.o \
.libs/timebasedrollingpolicy.o .libs/transform.o .libs/triggeringpolicy.o .libs/transcoder.o .libs/ttcclayout.o .libs/writer.o \
.libs/writerappender.o .libs/xmllayout.o .libs/xmlsocketappender.o .libs/zipcompressaction.o  /usr/lib64/libaprutil-1.so -lldap -llber -ldb-4.3 \
-lexpat /usr/lib64/libapr-1.so -lpthread -ldl -L/usr/lib/gcc/x86_64-redhat-linux/4.1.2 -L/usr/lib/gcc/x86_64-redhat-linux/4.1.2/../../../../lib64 \
-L/lib/../lib64 -L/usr/lib/../lib64 -lstdc++ -lm -lc -lgcc_s /usr/lib/gcc/x86_64-redhat-linux/4.1.2/crtendS.o \
/usr/lib/gcc/x86_64-redhat-linux/4.1.2/../../../../lib64/crtn.o  -Wl,-soname -Wl,liblog4cxx.so.10 -o .libs/liblog4cxx.so.10.0.0
