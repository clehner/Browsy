#   File:       Browsy.make
#   Target:     Browsy
#   Created:    Saturday, July 7, 2012 01:10:12 AM


MAKEFILE        = Browsy.make
�MondoBuild�    = {MAKEFILE}  # Make blank to avoid rebuilds when makefile is modified

SrcDir          = :src:
ObjDir          = :build:
Includes        = �
                  -i :src: �
				  -i :include:

Sym-68K         = -sym off

COptions        = {Includes} {Sym-68K} -model near -includes unix


### Source Files ###

SrcFiles        =  �
				  "{SrcDir}Browsy.r" �
				  "{SrcDir}main.c" �
				  "{SrcDir}menus.c" �
				  "{SrcDir}window.c" �
				  "{SrcDir}utils.c" �
				  "{SrcDir}uri.c" �
				  "{SrcDir}document.c" �
				  "{SrcDir}parser.c"


### Object Files ###

ObjFiles-68K    =  �
				  "{ObjDir}main.c.o" �
				  "{ObjDir}menus.c.o" �
				  "{ObjDir}window.c.o" �
				  "{ObjDir}utils.c.o" �
				  "{ObjDir}uri.c.o" �
				  "{ObjDir}document.c.o" �
				  "{ObjDir}parser.c.o"


### Libraries ###

LibFiles-68K    = "{Libraries}MathLib.o" �
				  "{CLibraries}StdCLib.o" �
				  "{Libraries}MacRuntime.o" �
				  "{Libraries}IntEnv.o" �
				  "{Libraries}ToolLibs.o" �
				  "{Libraries}Interface.o" �
				  #"{SharedLibraries}MenusLib" �
				  #:lib:tidy.o


### Default Rules ###

.c.o  �  .c  {�MondoBuild�}
	{C} {depDir}{default}.c -o {targDir}{default}.c.o {COptions}


### Build Rules ###

Browsy  ��  {ObjFiles-68K} {LibFiles-68K} {�MondoBuild�}
	ILink �
		-o {Targ} �
		#-weaklib MenusLib �
		{LibFiles-68K} �
		{ObjFiles-68K} �
		{Sym-68K} �
		-mf -d �
		-t 'APPL' �
		-c 'WWW6' �
		-model near �
		-state rewrite �
		-compact -pad 0
	If "{Sym-68K}" =~ /-sym �[nNuU]�/
		ILinkToSYM {Targ}.NJ -mf -sym 3.2 -c 'sade'
	End


Browsy  ��  "{SrcDir}Browsy.rsrc" {�MondoBuild�}
	Rez "{SrcDir}Browsy.r" -o {Targ} {Includes} -append


### Required Dependencies ###

"{ObjDir}main.c.o"  �  "{SrcDir}main.c"
"{ObjDir}menus.c.o"  �  "{SrcDir}menus.c"
"{ObjDir}window.c.o"  �  "{SrcDir}window.c"
"{ObjDir}utils.c.o"  �  "{SrcDir}utils.c"
"{ObjDir}uri.c.o"  �  "{SrcDir}uri.c"
"{ObjDir}document.c.o"  �  "{SrcDir}document.c"
"{ObjDir}parser.c.o"  �  "{SrcDir}parser.c"


### Optional Dependencies ###
### Build this target to generate "include file" dependencies. ###

Dependencies  �  $OutOfDate
	MakeDepend �
		-append {MAKEFILE} �
		-ignore "{CIncludes}" �
		-ignore "{RIncludes}" �
		-objdir "{ObjDir}" �
		-objext .o �
		{Includes} �
		{SrcFiles}


