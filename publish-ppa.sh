#!/bin/bash
#
# sudo apt-get install devscripts debhelper
#
version=`git describe --tags --abbrev=0`
branch=`git rev-parse --abbrev-ref HEAD`

ROOT=`pwd`
PPA="ppa:mathieu.leplatre/subtivals"
TMP_DIST=/tmp/subtivals

mkdir -p $TMP_DIST

# utopic vivid wily xenial yakkety zesty artful bionic cosmic
for flavor in trusty xenial bionic focal groovy hirsute impish jammy kinetic; do
    echo "\nAutomatically update branch $flavor"
    git checkout -b $flavor
    git reset --hard $version
    sed -i "s/) stable/~$flavor) $flavor/" debian/changelog
    git commit -a -m "$flavor package $version"

    echo "\nArchive $flavor repository in "`pwd`"..."    
    git archive --format=tar --prefix=$flavor/ $flavor | (cd $TMP_DIST && tar -xf -)
    ls $TMP_DIST
    git checkout $branch
    git branch -D $flavor

    echo "\nBuild package from "$TMP_DIST/$flavor
    cd $TMP_DIST/$flavor
    debuild -S -sa
    srcpckg=$(find ${TMP_DIST} -name "subtivals_${version}-*~${flavor}_source.changes" | sort -r | head -n1)
    echo "Will upload $srcpckg"
    if [ -f $srcpckg ] ; then
        dput -f $PPA $srcpckg
    else
        echo "No source package created: "$srcpckg
    fi   
    echo "----------------------------------------------------------------------\n\n"
    cd $ROOT
done

echo "Cleanup $TMP_DIST..."
#rm -rf $TMP_DIST

git checkout $branch
git reset $version
