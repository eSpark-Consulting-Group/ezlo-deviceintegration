define Package/firmware-core-uid_interface
	$(call Package/hub-platform/Default)
	TITLE=firmware-core-uid_interface
	VERSION=1.0
	EXTRA_DEPENDS += firmware-common-base_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-bus (=1.0),
	EXTRA_DEPENDS += firmware-common-common_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-data_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-dbclient (=1.0),
	EXTRA_DEPENDS += firmware-common-jsonrpc (=1.0),
	EXTRA_DEPENDS += firmware-common-log_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-service_interfaces (=1.0),
	EXTRA_DEPENDS += firmware-common-tiny (=1.0),
	EXTRA_DEPENDS += firmware-core-cored_interface (=1.0),
	EXTRA_DEPENDS += firmware-core-hardware_manager (=1.0),
	EXTRA_DEPENDS += firmware-core-networkd (=1.0),
	EXTRA_DEPENDS += libc (=1.1.23-2),
	EXTRA_DEPENDS += libgcc (=7.4.0-2),
	EXTRA_DEPENDS += libstdcpp (=7.4.0-2),
	DEPENDS += +firmware-common-base_utils
	DEPENDS += +firmware-common-bus
	DEPENDS += +firmware-common-common_utils
	DEPENDS += +firmware-common-data_utils
	DEPENDS += +firmware-common-dbclient
	DEPENDS += +firmware-common-jsonrpc
	DEPENDS += +firmware-common-log_utils
	DEPENDS += +firmware-common-service_interfaces
	DEPENDS += +firmware-common-tiny
	DEPENDS += +firmware-core-cored_interface
	DEPENDS += +firmware-core-hardware_manager
	DEPENDS += +firmware-core-networkd
	DEPENDS += +libc
	DEPENDS += +libgcc
	DEPENDS += +libstdcpp
endef

define Package/firmware-core-uid_interface/description
	Just firmware-core-uid_interface package
endef

define Package/firmware-core-uid_interface/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-core-uid_interface/* $(1)/
endef

$(eval $(call BuildPackage,firmware-core-uid_interface))
