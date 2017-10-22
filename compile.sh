#!/bin/sh -e

# /*******************************************************************************/
# /*                                                                             */
# /*  Copyright 2004-2017 Pascal Gloor                                           */
# /*                                                                             */
# /*  Licensed under the Apache License, Version 2.0 (the "License");            */
# /*  you may not use this file except in compliance with the License.           */
# /*  You may obtain a copy of the License at                                    */
# /*                                                                             */
# /*     http://www.apache.org/licenses/LICENSE-2.0                              */
# /*                                                                             */
# /*  Unless required by applicable law or agreed to in writing, software        */
# /*  distributed under the License is distributed on an "AS IS" BASIS,          */
# /*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   */
# /*  See the License for the specific language governing permissions and        */
# /*  limitations under the License.                                             */
# /*                                                                             */
# /*******************************************************************************/



progs="grep awk sed cp mkdir chmod printf";

tab_print()
{
	first=1;

	for name in $2
	do
		if [ "$first" -eq "1" ]
		then
			printf "%-9s : %s\n" $1 $name
		else
			printf "            %s\n" $name
		
		fi

		first=0;
	done
}

addpath()
{
	if [ -r "${1}.in" ]
	then
		MYPATH=`echo ${DIR} | sed "s/\//\\\\\\\\\\//g"`;
		cat ${1}.in | sed "s/\%PATH\%/${MYPATH}/g" > ${1};
	else
		echo "error cannot read ${1}.in";
		exit 1;
	fi
}

create_dir()
{
	printf "creating directory ";
	if [ -r "${1}" ]
	then
		if [ -d "${1}" ]
		then
			echo "${1} ok (exists)";
		else
			echo "${1} exist and is NOT a directory, sorry.";
			exit 1;
		fi
	else
		mkdir -p ${1};
		if [ "$?" != 0 ]
		then
			echo "error creating ${1}, sorry.";
			exit 1;
		fi
		echo "${1} ok (created)";
	fi
}

copy_file()
{
	printf "installing %s (mode %s)\n" ${2} ${3}

	cp -f ${1} ${2};
	if [ "$?" != 0 ]; then echo "error copying file, sorry"; exit 1; fi

	chmod ${3} ${2};
	if [ "$?" != 0 ]; then echo "error changing file mode, sorry"; exit 1; fi
}

locate_prog()
{
	LOC=`which ${1} 2>&1`;
	if [ ! -x "${LOC}" ]
	then
		echo "looking for ${1}... not found ;(";
		echo "Please check your PATH environement variable";
		exit 1;
	fi
}

for myprog in $progs
do
	locate_prog $myprog
done

P_VER_MA=`grep P_VER_MA inc/p_defs.h | awk '{print $3}' | sed "s/\"//g" `;
P_VER_MI=`grep P_VER_MI inc/p_defs.h | awk '{print $3}' | sed "s/\"//g" `;
P_VER_PL=`grep P_VER_PL inc/p_defs.h | awk '{print $3}' | sed "s/\"//g" `;

if [ -z "$1" ]; then
	echo "Piranha v${P_VER_MA}.${P_VER_MI}.${P_VER_PL} BGP Daemon, compilation/installation script.";
	echo ""
	echo "syntax: $0 <directory> [debug]";
	echo ""
	echo "directory : the place where you want to install Piranha,";
	echo "            /usr/local/piranha sounds a good place.";
	echo "debug     : optional"
	echo "";
	exit 1;
fi

if [ -n "$2" ]; then DODEBUG=1; fi

DIR=`echo $1 | sed "s/\/$//" | sed "s/\/\//\//g"`;

cat <<EOF

/*******************************************************************************/
/*                                                                             */
/*        ::::::    ::                                ::                       */
/*        ::    ::      ::  ::::    ::::::  ::::::    ::::::      ::::::       */
/*        ::::::    ::  ::::      ::    ::  ::    ::  ::    ::  ::    ::       */
/*        ::        ::  ::        ::    ::  ::    ::  ::    ::  ::    ::       */
/*        ::        ::  ::          ::::::  ::    ::  ::    ::    ::::::       */
/*                                                                             */
/*                                                                             */
/*******************************************************************************/
/*                                                                             */
/*  Copyright 2004-2017 Pascal Gloor                                           */
/*                                                                             */
/*  Licensed under the Apache License, Version 2.0 (the "License");            */
/*  you may not use this file except in compliance with the License.           */
/*  You may obtain a copy of the License at                                    */
/*                                                                             */
/*     http://www.apache.org/licenses/LICENSE-2.0                              */
/*                                                                             */
/*  Unless required by applicable law or agreed to in writing, software        */
/*  distributed under the License is distributed on an "AS IS" BASIS,          */
/*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   */
/*  See the License for the specific language governing permissions and        */
/*  limitations under the License.                                             */
/*                                                                             */
/*******************************************************************************/
EOF

echo ""
echo ""

OS=`uname -s | tr [a-z] [A-Z]`
OSVER=`uname -r`

if [ -z "$OS" ]
then
	echo "ERROR: unable to determine Operating System. Sorry"
	exit 1
fi


if [ -x "`which gcc 2>&1`" ]
then
	cc="gcc";
	CC="GCC";
	debug="-g -DDEBUG";

	if [ "$OS" = "FREEBSD" ]
	then
		warn="-Wall -Wshadow -Wcast-qual -Wcast-align -Wpointer-arith \
			-Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations \
			-Wredundant-decls -Wnested-externs -Werror";
		opt="-O2 -ansi -pedantic -fsigned-char";
		lib="-pthread -D_THREAD_SAFE";
	elif [ "$OS" = "NETBSD" ]
	then
		warn="-Wall -Wshadow -Wcast-qual -Wcast-align -Wpointer-arith \
			-Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations \
			-Wredundant-decls -Wnested-externs -Werror";
		opt="-O2 -ansi -pedantic -fsigned-char";
		lib="-lpthread";
	elif [ "$OS" = "LINUX" ]
	then
		warn="-Wall -Wshadow -Wcast-qual -Wcast-align -Wpointer-arith \
			-Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations \
			-Wnested-externs ";
		lib="-lpthread";
		opt="-O2 -fsigned-char";
	elif [ "$OS" = "DARWIN" ]
	then
		warn="-Wall -Wshadow -Wcast-qual -Wpointer-arith \
			-Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations \
			-Wnested-externs -Werror";
#		lib="-lpthread";
		opt="-O2 -fsigned-char";
	else
		echo "WARNING: You're running an untested OS (${OS} ${OSVER}). " \
			"The compilation might not work.";
		warn="-Wall"
		opt="-O2 -ansi -pedantic -fsigned-char";
		lib="-lpthread";
	fi

elif [ -x "`which cc 2>&1`" ]
then
cat <<EOF
WARNING: You're running a unsupported compiler (not GCC) on an
unsupported OS (${OS} ${OSVER}). We're not yet compatible with your OS
feel free to contact us to make it compatible with your platform.
mailto: <pascal.gloor@spale.com>
EOF
	exit 1;
else

	echo "ERROR: No C compiler found. Sorry."
	exit 1;
fi

if [ -z "$DODEBUG" ]; then debug=""; fi

echo "compiler and options we'll use :"
echo ""
echo "system    : $OS $OSVER"
echo "compiler  : $cc";
tab_print "options" "$opt"
tab_print "warnings" "$warn"
tab_print "libraries" "$lib"

if [ -n "${DODEBUG}" ]
then
	echo "debug     -> $debug";
fi

echo ""

echo "Creating required directories"
create_dir 'bin'
create_dir 'obj'


incdir="-Iinc"
libdir=""
srcdir="src"
objdir="obj"

src_1="p_tools p_config p_socket p_log p_dump p_piranha"
src_2="p_tools p_undump p_ptoa"
bin_1="bin/piranha"
bin_2="bin/ptoa"
tmp_1=""
tmp_2=""

opt="$opt -D${OS} -D${CC}";

for compile in ${src_1}
do
	if [ -n "${DODEBUG}" ]
	then
		echo "$cc $debug -DPATH=\"${DIR}\" $libdir $incdir $opt $warn " \
			"-o $objdir/$compile.o -c $srcdir/$compile.c";
	else
		printf "compiling %-17s   ->   %-17s ... " \
			$srcdir/$compile.c $objdir/$compile.o
	fi
	
	$cc $debug -DPATH=\"${DIR}\" $libdir $incdir $opt $warn \
		-o $objdir/$compile.o -c $srcdir/$compile.c

	if [ "$?" -ne "0" ]
	then
		echo "ERROR: compilation error"
		exit 1;
	else
		echo "ok";
	fi

	tmp_1="${tmp_1} $objdir/$compile.o"
done

if [ -n "${DODEBUG}" ]
then
	echo "$cc $debug $libdir $incdir $lib $warn -o ${bin_1} ${tmp_1}"
else
	echo "";
	tab_print "linking" "$tmp_1 $bin_1"
fi

$cc $debug $libdir $incdir $warn -o ${bin_1} ${tmp_1} $lib

if [ "$?" -ne "0" ]
then
	echo "ERROR: linking error"
	exit 1;
else
	echo "ok";
fi


for compile in ${src_2}
do
	if [ -n "${DODEBUG}" ]
	then
		echo "$cc $debug -DPATH=\"${DIR}\" $libdir $incdir $opt $warn " \
			"-o $objdir/$compile.o -c $srcdir/$compile.c";
	else
		printf "compiling %-17s   ->   %-17s ... " \
			$srcdir/$compile.c $objdir/$compile.o
	fi
	
	$cc $debug -DPATH=\"${DIR}\" $libdir $incdir $opt $warn \
		-o $objdir/$compile.o -c $srcdir/$compile.c

	if [ "$?" -ne "0" ]
	then
		echo "ERROR: compilation error"
		exit 1;
	else
		echo "ok";
	fi

	tmp_2="${tmp_2} $objdir/$compile.o"
done

if [ -n "${DODEBUG}" ]
then
	echo "$cc $debug $libdir $incdir $lib $warn -o ${bin_2} ${tmp_2}"
else
	echo "";
	tab_print "linking" "$tmp_2 $bin_2"
fi

$cc $debug $libdir $incdir $warn -o ${bin_2} ${tmp_2} $lib

if [ "$?" -ne "0" ]
then
	echo "ERROR: linking error"
	exit 1;
else
	echo "ok";
fi

echo ""
echo "compilation done ;-)"
echo ""

printf "Install Piranha to ${DIR} ? ([Y]/n) "
read test

if [ -z "$test" ]; then test="Y"; fi;

case "$test" in

[Y]|[y])
	;;
*)
	echo "restart \"$0\" at any time to finish the installation.";
	echo "installation aborted";
	exit 1;
	;;
esac

echo "";

create_dir "${DIR}/bin";
create_dir "${DIR}/etc";
create_dir "${DIR}/var";
create_dir "${DIR}/dump";
create_dir "${DIR}/man/man1";
create_dir "${DIR}/man/man5";

echo ""

addpath "./utils/piranhactl";

copy_file ./${bin_1} ${DIR}/${bin_1} 555
copy_file ./${bin_2} ${DIR}/${bin_2} 555

copy_file ./etc/piranha_sample.conf ${DIR}/etc/piranha_sample.conf 644
copy_file ./utils/piranhactl ${DIR}/bin/piranhactl 555

copy_file ./man/man1/piranha.1 ${DIR}/man/man1/piranha.1 444
copy_file ./man/man1/piranhactl.1 ${DIR}/man/man1/piranhactl.1 444
copy_file ./man/man1/ptoa.1 ${DIR}/man/man1/ptoa.1 444
copy_file ./man/man5/piranha.conf.5 ${DIR}/man/man5/piranha.conf.5 444

echo "/-----------------------------------------------------------------";
echo "|";
echo "| Piranha has been installed to ${DIR}";
echo "|";
echo "| start: ${DIR}/bin/piranhactl start";
echo "|";
echo "| Please read the doc/piranha.txt file and the sample configuration";
echo "| (${DIR}/etc/piranha_sample.conf)";
echo "|";
echo "| manpages available!";
echo "| man -M${DIR}/man piranha";
echo "| man -M${DIR}/man piranha.conf";
echo "| man -M${DIR}/man piranhactl";
echo "| man -M${DIR}/man ptoa";
echo "|";
echo "\\------------------------------------------------------------------";
