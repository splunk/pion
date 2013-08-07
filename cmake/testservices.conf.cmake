##
## Simple configuration file to test pion-net web services
##

## Adds path to compiled web services built from source tarball
##
path @PION_PLUGIN_PATH@

## Hello World Service
##
service /hello HelloService

## Service to echo requests
##
service /echo EchoService

## Service to display & update cookies
##
service /cookie CookieService

## Service to display log events
##
service /log LogService

## Service to serve sample pion-net documentation
##
service /doc FileService
option /doc directory=@PION_SRC_ROOT@/tests/doc/html
option /doc file=@PION_SRC_ROOT@/tests/doc/html/index.html
option /doc cache=2
option /doc scan=3

## Use testservices.html as an index page
## 
service /index.html FileService
option /index.html file=@PION_SRC_ROOT@/utils/testservices.html
##
service / FileService
option / file=@PION_SRC_ROOT@/utils/testservices.html

## Service to demonstrate Authentication interface
##
service /auth EchoService

##
## define type of authentication
##
## MUST be a first command in configuration file
## auth type can be defined once and only once!
##
auth cookie

##
## Add /auth resource to set of password protected
##
restrict /auth

##
## Add /protected resource to set of password protected
##
restrict /protected

##
## define user
##
user stas 123456

##
## define user
##
user mike 123456
