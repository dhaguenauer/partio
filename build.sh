#!/bin/bash

build_partio_lib=0
build_partio_maya=0
build_partio_arnold=1

suppress_build_dirs_prior_build=0

build_type=Release

#mayaVersions=(2015.sp6) #2014.sp4 do not build
mayaVersions=(2016.sp6) #2014.sp4 do not build
#mayaVersions=(ext2.2016.sp1)
#mayaVersions=(2017.0.0)


#############################################################################
#    for MAYA 2016:                                                 		#
# rez gxx-4.8 cmake swig-3.0.5 glew-2.0 python-2.7 mayaAPI-2016 boost-1.48  #
#																			#
#############################################################################


arnold_version='5.0.1.0'
mtoa_version='$mtoa_variant.0'
compiler_path='/usr/bin/g++-4.6'

swig_executable='/s/apps/packages/dev/swig/3.0.5/platform-linux/bin/swig'


glew_include_dir=$REZ_GLEW_ROOT'/include'
glew_static_library=$REZ_GLEW_ROOT'/lib64/libGLEW.a'

#glew_include_dir='/s/apps/lin/vfx_test_apps/glew/1.11.0/include'
#glew_static_library='/s/apps/lin/vfx_test_apps/glew/1.11.0/lib/libGLEW.a'




export ARNOLD_HOME=/s/apps/packages/cg/arnold/$arnold_version/platform-linux
#export MTOA_ROOT=/s/apps/packages/mikros/mayaModules/mimtoa/$mtoa_version/platform-linux/arnold-4.2/maya-2016
export MTOA_ROOT=/s/apps/packages/cg/mtoa/2.0.0.1/platform-linux/maya-2016					# no mimtoa version yet so we take the mtoa one
export PARTIO_HOME=/datas/hda/build/partio/build-Linux-x86_64


if [ "$suppress_build_dirs_prior_build" == 1 ]; then

	rm -fr /datas/hda/build/partio/partio.build
	rm -fr /datas/hda/build/partio/build-Linux-x86_64
	
	mkdir partio.build
	mkdir build-Linux-x86_64
fi

cd partio.build

if [ "$build_partio_lib" == 1 ]; then

	echo "BUILD PARTIO LIB"

	cmake .. \
	  -DBUILD_PARTIO_LIBRARY=1 \
	  -DBUILD_PARTIO_MAYA=0 \
	  -DBUILD_PARTIO_MTOA=0

	make -j12
	make install
fi

### Build Partio Maya plugin
echo $build_partio_maya

if [ "$build_partio_maya" == 1 ]; then

	echo "BUILD PARTIO MAYA"

	for mv in "${mayaVersions[@]}"
	do
		maya_executable='/s/apps/packages/cg/maya/'$mv'/platform-linux/bin/maya'	
		
		cmake .. \
		-DCMAKE_BUILD_TYPE=$build_type \
		-DGLEW_INCLUDE_DIR=$glew_include_dir \
		-DMAYA_EXECUTABLE=$maya_executable \
		-DBUILD_PARTIO_LIBRARY=1 \
		-DBUILD_PARTIO_MAYA=1 \
		-DBUILD_PARTIO_MTOA=0
		
		make -j1
		make install
	done	
fi

### Build Partio Arnold proceduralmaya_variant='$maya_variant'
mtoa_variant='$mtoa_variant'
cp /datas/hda/build/partio/build-Linux-x86_64/maya/$maya_variant/plug-ins/Linux-x86_64/partio4Maya.so 		/s/apps/users/hda/packages/cgDev/partioMaya/dev/platform-linux/maya-$maya_variant/plug-ins
cp /datas/hda/build/partio/build-Linux-x86_64/maya/$maya_variant/scripts/*.mel 								/s/apps/users/hda/packages/cgDev/partioMaya/dev/platform-linux/maya-$maya_variant/scripts
cp /datas/hda/build/partio/build-Linux-x86_64/maya/$maya_variant/icons/*.xpm 								/s/apps/users/hda/packages/cgDev/partioMaya/dev/platform-linux/maya-$maya_variant/icons

cp /datas/hda/build/partio/build-Linux-x86_64/arnold/procedurals/partioGenerator.so 				/s/apps/users/hda/packages/cgDev/partioArnold/dev/platform-linux/mtoa-$mtoa_variant/maya-$maya_variant/extensions/
cp /datas/hda/build/partio//contrib/partio4Arnold/plugin/partioTranslator.py 						/s/apps/users/hda/packages/cgDev/partioArnold/dev/platform-linux/mtoa-$mtoa_variant/maya-$maya_variant/extensions/
cp /datas/hda/build/partio/build-Linux-x86_64/extensions/partioTranslator.so 						/s/apps/users/hda/packages/cgDev/partioArnold/dev/platform-linux/mtoa-$mtoa_variant/maya-$maya_variant/extensions/
cp /datas/hda/build/partio/build-Linux-x86_64/arnold/procedurals/partioGenerator.so 				/s/apps/users/hda/packages/cgDev/partioArnold/dev/platform-linux/mtoa-$mtoa_variant/maya-$maya_variant/procedurals/




if [ "$build_partio_arnold" == 1 ]; then
cmake .. \
-DCMAKE_BUILD_TYPE=$build_type \
-DSWIG_EXECUTABLE=$swig_executable \
-DGLEW_INCLUDE_DIR=$glew_include_dir \
-DGLEW_STATIC_LIBRARY=$glew_static_library \
-DBUILD_PARTIO_LIBRARY=1 \
-DBUILD_PARTIO_MAYA=0 \
-DBUILD_PARTIO_MTOA=1

#-DCMAKE_CXX_COMPILER=$compiler_path \

make -j12
make install
fi



maya_variant='2016'
mtoa_variant='2.0.1'
cp -v /datas/hda/build/partio/build-Linux-x86_64/maya/$maya_variant/plug-ins/Linux-x86_64/partio4Maya.so 		/s/apps/users/hda/packages/cgDev/partioMaya/dev/platform-linux/maya-$maya_variant/plug-ins
#cp -v /datas/hda/build/partio/build-Linux-x86_64/maya/$maya_variant/scripts/*.mel 								/s/apps/users/hda/packages/cgDev/partioMaya/dev/platform-linux/maya-$maya_variant/scripts
cp -v /datas/hda/build/partio/contrib/partio4Maya/scripts/*.mel 												/s/apps/users/hda/packages/cgDev/partioMaya/dev/platform-linux/maya-$maya_variant/scripts
#cp -v /datas/hda/build/partio/build-Linux-x86_64/maya/$maya_variant/icons/*.xpm 								/s/apps/users/hda/packages/cgDev/partioMaya/dev/platform-linux/maya-$maya_variant/icons
cp -v /datas/hda/build/partio/contrib/partio4Maya/icons/*.xpm 													/s/apps/users/hda/packages/cgDev/partioMaya/dev/platform-linux/maya-$maya_variant/icons

cp -v /datas/hda/build/partio/build-Linux-x86_64/arnold/procedurals/partioGenerator.so 				/s/apps/users/hda/packages/cgDev/partioArnold/dev/platform-linux/mtoa-$mtoa_variant/maya-$maya_variant/extensions/
cp -v /datas/hda/build/partio//contrib/partio4Arnold/plugin/partioTranslator.py 						/s/apps/users/hda/packages/cgDev/partioArnold/dev/platform-linux/mtoa-$mtoa_variant/maya-$maya_variant/extensions/
cp -v /datas/hda/build/partio/build-Linux-x86_64/extensions/partioTranslator.so 						/s/apps/users/hda/packages/cgDev/partioArnold/dev/platform-linux/mtoa-$mtoa_variant/maya-$maya_variant/extensions/
cp -v /datas/hda/build/partio/build-Linux-x86_64/arnold/procedurals/partioGenerator.so 				/s/apps/users/hda/packages/cgDev/partioArnold/dev/platform-linux/mtoa-$mtoa_variant/maya-$maya_variant/procedurals/





