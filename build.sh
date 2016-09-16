#!/bin/bash

build_partio_lib=1
build_partio_maya=1
build_partio_arnold=1

#mayaVersions=(2015.sp6 2016.sp4) #2014.sp4 do not build
#mayaVersions=(2016.sp6) #2014.sp4 do not build
mayaVersions=(2015.sp6)


#rez env gxx-4.6 cmake swig-3.0.5 glew-1.13 python-2.7 mayaAPI-2016


arnold_version='4.2.12.2'
mtoa_version='1.3.1.0.15'
compiler_path='/usr/bin/g++-4.6'

swig_executable='/s/apps/packages/dev/swig/3.0.5/platform-linux/bin/swig'
glew_include_dir='/s/apps/lin/vfx_test_apps/glew/1.11.0/include'
glew_static_library='/s/apps/lin/vfx_test_apps/glew/1.11.0/lib/libGLEW.a'

export ARNOLD_HOME=/s/apps/packages/cg/arnold/$arnold_version/platform-linux
export MTOA_ROOT=/s/apps/users/hda/packages/mikros/mayaModules/mimtoa/$mtoa_version/platform-linux/arnold-4.2/maya-2015                 
export PARTIO_HOME=/s/apps/users/hda/build/partio/build-Linux-x86_64

echo "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ MAYA_API_LOCATION :"
echo $MAYA_API_LOCATION


rm -fr /s/apps/users/hda/build/partio/partio.build
#rm -fr /s/apps/users/hda/build/partio/build-Linux-x86_64

mkdir partio.build
mkdir build-Linux-x86_64

cd partio.build

# Build Partio Library anc bin
# cmake .. \
# #-DCMAKE_CXX_COMPILER=$compiler_path \
# -DSWIG_EXECUTABLE=$swig_executable \
# -DGLEW_INCLUDE_DIR=$glew_include_dir \
# -DGLEW_STATIC_LIBRARY=$glew_static_library \
# -DBUILD_PARTIO_LIBRARY=1 \
# -DBUILD_PARTIO_MAYA=0 \
# -DBUILD_PARTIO_MTOA=0

# cmake .. \
# -DSWIG_EXECUTABLE=$swig_executable \
# -DGLEW_INCLUDE_DIR=$glew_include_dir \
# -DGLEW_STATIC_LIBRARY=$glew_static_library \
# -DBUILD_PARTIO_LIBRARY=1 \
# -DBUILD_PARTIO_MAYA=0 \
# -DBUILD_PARTIO_MTOA=0

if [ "$build_partio_lib" == 1 ]; then

	cmake .. \
	  -DBUILD_PARTIO_LIBRARY=1 \
	  -DBUILD_PARTIO_MAYA=0 \
	  -DBUILD_PARTIO_MTOA=0

	make -j12
	make install
fi

# Build Partio Maya plugin
if [ "$build_partio_maya" == 1 ]; then

	echo "######################  build_partio_maya test ok"

	for mv in "${mayaVersions[@]}"
	do

		echo "######################  for test ok"

		maya_executable='/s/apps/packages/cg/maya/'$mv'/platform-linux/bin/maya'	
		
		cmake .. \
		-DCMAKE_BUILD_TYPE=Release \
		-DGLEW_INCLUDE_DIR=$glew_include_dir \
		-DGLEW_STATIC_LIBRARY=$glew_static_library \
		-DMAYA_EXECUTABLE=$maya_executable \
		-DBUILD_PARTIO_LIBRARY=1 \
		-DBUILD_PARTIO_MAYA=1 \
		-DBUILD_PARTIO_MTOA=0
		
		make -j12
		make install
	done	
fi


# Build Partio Arnold procedural
if [ "$build_partio_arnold" == 1 ]; then
cmake .. \
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

cp /s/apps/users/hda/build/partio/build-Linux-x86_64/maya/2016/plug-ins/Linux-x86_64/partio4Maya.so /s/apps/users/hda/packages/cgDev/partioMaya/dev/platform-linux/maya-2016/plug-ins


