#!/bin/bash

# author Loic Touraine
for prefix in $(ls); do
    echo $prefix
    for package in $(ls ${prefix}); do
		echo $package
		for version in $(ls ${prefix}/${package}); do
			echo $version
			for platform in $(ls ${prefix}/${package}/${version}/lib); do
				echo $platform
				for mode in $(ls ${prefix}/${package}/${version}/lib/${platform}); do
					echo $mode
					for config in  $(ls ${prefix}/${package}/${version}/lib/${platform}/${mode}); do
					echo $config
					# Cleanup previous package
					rm ${platform}_${mode}_${config}/${package}_${version}_${platform}_${mode}_${config}.zip
					# Create temporary package directory
					mkdir -p ${platform}_${mode}_${config}/${package}/${version}
					# Populate package directory for platform, mode and config
					# Add interfaces
					cp -r ${prefix}/${package}/${version}/interfaces ${platform}_${mode}_${config}/${package}/${version}/.
					# Add .pkginfo
					cp -r ${prefix}/${package}/${version}/.pkginfo ${platform}_${mode}_${config}/${package}/${version}/.
					# Add csharp
					cp -r ${package}/${version}/csharp ${platform}_${mode}_${config}/${package}/${version}/.
					# Add wizards for xpcf
					cp -r ${package}/${version}/wizards ${platform}_${mode}_${config}/${package}/${version}/.
					# Add .pc files
					cp ${package}/${version}/*.pc ${platform}_${mode}_${config}/${package}/${version}/.
					# Add .txt files
					cp ${package}/${version}/*.txt ${platform}_${mode}_${config}/${package}/${version}/.
					# Add .xml files
					cp ${package}/${version}/*.xml ${platform}_${mode}_${config}/${package}/${version}/.
					# Add libraries files
					for libdir in $(find ${prefix}/${package}/${version}/lib -type d -name  ${config} | grep ${mode} | grep ${platform}); do
						TOPLIBDIR=$(dirname ${libdir})
						mkdir -p ${platform}_${mode}_${config}/${TOPLIBDIR}
						cp -r --preserve=links ${libdir} ${platform}_${mode}_${config}/${TOPLIBDIR}/.
					done
					# Prepare package archive
					cd  ${platform}_${mode}_${config}
					zip -r -y ${prefix}_${package}_${version}_${platform}_${mode}_${config}.zip ${package}
					cd -
					# Cleanup temporary directory
					rm -rf ${platform}_${mode}_${config}/${package}
					done
				done
			done
		done
    done
done
