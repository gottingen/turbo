# platform

defines macro demonstrate platform.

## platform list:

* linux
* windows
* mac os
* cygwin
* mingw
* android
* iphone
* xbox

## cpu architecture

* x86
* x86_64
* arm

## os types

* windows 
* unix series (unix linux apple)

## macros

* ABEL_PLATFORM_CYGWIN/ABEL_PLATFORM_MINGW
 pseudo platform, on windows mock like a unix like system to build and run.

* ABEL_PLATFORM_NAME define the system name.

     system | system name 
     :--- |     :----  
     ABEL_PLATFORM_PS4 | PS4
     ABEL_PLATFORM_XBOXONE | XBox One
     ABEL_PLATFORM_ANDROID|Android
     ABEL_PLATFORM_IPHONE|iPhone
     ABEL_PLATFORM_OSX|OSX
     ABEL_PLATFORM_LINUX|Linux
     ABEL_PLATFORM_BSD|BSD Unix
     ABEL_PLATFORM_WINDOWS_PHONE|Windows Phone
     ABEL_PLATFORM_WINDOWS|Windows
     
* cpu architecture
    * ABEL_PROCESSOR_POWERPC
    * ABEL_PROCESSOR_ARM64
    * ABEL_PROCESSOR_ARM32
    * ABEL_PROCESSOR_X86_64
    * ABEL_PROCESSOR_X86
    
* ABEL_CACHE_LINE_SIZE as its name cpu cache line size

* ABEL_PLATFORM_MIN_MALLOC_ALIGNMENT 
the minus This defines the minimal alignment that the platform's malloc 
implementation will return. This should be used when writing custom allocators to ensure that the alignment matches 
that of malloc     

* ABEL_PLATFORM_PTR_SIZE pointer size
     