define Package/firmware-utils-hardware_test
	$(call Package/hub-platform/Default)
	TITLE=firmware-utils-hardware_test
	VERSION=1.0
	EXTRA_DEPENDS += firmware-common-base_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-dbclient (=1.0),
	EXTRA_DEPENDS += firmware-common-ember_znet (=1.0),
	EXTRA_DEPENDS += firmware-common-jsonrpc (=1.0),
	EXTRA_DEPENDS += firmware-common-log_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-system_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-tiny (=1.0),
	EXTRA_DEPENDS += glib2 (=2.46.2-1),
	EXTRA_DEPENDS += libc (=1.1.23-2),
	EXTRA_DEPENDS += libdbus (=1.9.14-1),
	EXTRA_DEPENDS += libgcc (=7.4.0-2),
	EXTRA_DEPENDS += libstdcpp (=7.4.0-2),
	DEPENDS += +firmware-common-base_utils
	DEPENDS += +firmware-common-dbclient
	DEPENDS += +firmware-common-ember_znet
	DEPENDS += +firmware-common-jsonrpc
	DEPENDS += +firmware-common-log_utils
	DEPENDS += +firmware-common-system_utils
	DEPENDS += +firmware-common-tiny
	DEPENDS += +glib2
	DEPENDS += +libc
	DEPENDS += +libdbus
	DEPENDS += +libgcc
	DEPENDS += +libstdcpp
endef

define Package/firmware-utils-hardware_test/description
	Just firmware-utils-hardware_test package
endef

define Package/firmware-utils-hardware_test/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-utils-hardware_test/* $(1)/
endef

$(eval $(call BuildPackage,firmware-utils-hardware_test))
