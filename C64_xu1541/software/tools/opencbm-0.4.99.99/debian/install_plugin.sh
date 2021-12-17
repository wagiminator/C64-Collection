#! /bin/bash

PLUGIN_NAME="$2"

OPENCBM_LIB_DIR=/usr/lib/opencbm
PLUGIN_DIR=${OPENCBM_LIB_DIR}/plugin
LIBNAME=libopencbm-${PLUGIN_NAME}
SHLIB=${LIBNAME}.so

PLUGIN_CONF_NAME=10${PLUGIN_NAME}.conf

PLUGIN_HELPER_TOOLS=${OPENCBM_LIB_DIR}/plugin_helper_tools

case "$1" in
	"install")
		TMPFILE=`mktemp` || exit 1

		echo "[${PLUGIN_NAME}]" > ${TMPFILE}
		echo "location=${PLUGIN_DIR}/${SHLIB}" >> ${TMPFILE}
		echo "" >> ${TMPFILE}
		${PLUGIN_HELPER_TOOLS} install /etc ${PLUGIN_CONF_NAME} ${TMPFILE}
		rm ${TMPFILE}

		# set this plugin as default plugin, if there is none yet.
		${PLUGIN_HELPER_TOOLS} setdefaultplugin /etc 00opencbm.conf ${PLUGIN_NAME}
		;;

	"uninstall")
		${PLUGIN_HELPER_TOOLS} uninstall /etc ${PLUGIN_CONF_NAME}
		;;

	"*")
		;;
esac
