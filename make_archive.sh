#!/bin/sh

PATH=/usr/local/bin:${PATH} ; export PATH

THISHOST="`uname`"
case $THISHOST in
	Linux|Darwin)
		;;
	*)
		THISHOST="`uname -m`"
		;;
esac

case $THISHOST in
	Darwin|"Power Macintosh"|Linux)
		  # this is similar to tcsh's 'symlink chase' option: always use the physical path, not the path as given
		  # by any symlinks in it.
		set -P
		;;
esac

cd "`dirname $0`"/..

mkdir -p $HOME/work/Archive

ECHO="/usr/local/bin/echo"

CleanUp(){
	${ECHO} "Removing temp copies"
	rm -rf $HOME/work/Archive/QTilities $HOME/work/Archive/QTilities.tar $HOME/work/Archive/make_archive.tar &
	exit 2
}

trap CleanUp 1
trap CleanUp 2
trap CleanUp 15

CP="cp"

OS="`uname`"
gcp --help 2>&1 | fgrep -- --no-dereference > /dev/null
if [ $? = 0 ] ;then
	 # hack... gcp must be gnu cp ...
	OS="LINUX"
	CP="gcp"
fi

${CP} --help 2>&1 | fgrep -- --no-dereference > /dev/null
if [ $? = 0 -o "$OS" = "Linux" -o "$OS" = "linux" -o "$OS" = "LINUX" ] ;then
	${ECHO} -n "Making temp copies..."
	${CP} -prd QTilities $HOME/work/Archive/
	${ECHO} " done."
	cd $HOME/work/Archive/
else
	${ECHO} -n "Making temp copies (tar to preserve symb. links).."
	gnutar -cf $HOME/work/Archive/make_archive.tar QTilities
	sleep 1
	${ECHO} "(untar).."
	cd $HOME/work/Archive/
	gnutar -xf make_archive.tar
	rm make_archive.tar
fi

sleep 1
${ECHO} "Cleaning out the backup copy"
( gunzip -vf QTilities/*.gz ;\
  bunzip2 -v QTilities/*.bz2 ;\
) 2>&1
rm -rf  QTilities/*.all_data QTilities/*.{log,tc,mov,VOD,avi,aup,caf,png,rgb,tif,docset}
(cd QTilities ;\
	find . -iname "*.ncb" -exec rm '{}' ";" ;\
	find . -iname "*.mov" -exec rm '{}' ";" ;\
	find . -iname "*.exe" -exec rm '{}' ";" ;\
	find . -iname "*.obj" -exec rm '{}' ";" ;\
	find . -iname "*.user" -exec rm '{}' ";" ;\
	find . -iname "*.dmg" -exec rm '{}' ";" ;\
)
find QTilities -name "build" -type d | xargs rm -rf
find QTilities -name "Debug" -type d | xargs rm -rf
find QTilities -name "Develop" -type d | xargs rm -rf
find QTilities -name "Release" -type d | xargs rm -rf
find QTilities -name "win32obj" | xargs rm -rf
find QTilities -name "win32sym" | xargs rm -rf
find QTilities -name "Mod2.zip" | xargs rm -rf

trap "" 1
trap "" 2
trap "" 15

sleep 1
${ECHO} "Archiving the cleaned backup directory"
gnutar -cvvf QTilities.tar QTilities
rm -rf QTilities
ls -lU QTilities.tar.bz2
bzip2 -vf QTilities.tar

trap CleanUp 1
trap CleanUp 2
trap CleanUp 15
${ECHO} "Cleaning up"

open .

trap 1
trap 2
trap 15

${ECHO} "Making remote backups - put your commands here"
