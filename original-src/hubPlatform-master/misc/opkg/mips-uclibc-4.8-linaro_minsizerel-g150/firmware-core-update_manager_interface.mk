define Package/firmware-core-update_manager_interface
	$(call Package/hub-platform/Default)
	TITLE=firmware-core-update_manager_interface
	VERSION=1.0
	EXTRA_DEPENDS += firmware-common-common_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-dbclient (=1.0),
	EXTRA_DEPENDS += firmware-common-log_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-service_interfaces (=1.0),
	EXTRA_DEPENDS += firmware-common-tiny (=1.0),
	EXTRA_DEPENDS += libc (=0.9.33.2-1),
	EXTRA_DEPENDS += libgcc (=4.8-linaro-1),
	EXTRA_DEPENDS += libopenssl (=1.0.2p-1),
	EXTRA_DEPENDS += libpthread (=0.9.33.2-1),
	EXTRA_DEPENDS += libstdcpp (=4.8-linaro-1),
	DEPENDS += +firmware-common-common_utils
	DEPENDS += +firmware-common-dbclient
	DEPENDS += +firmware-common-log_utils
	DEPENDS += +firmware-common-service_interfaces
	DEPENDS += +firmware-common-tiny
	DEPENDS += +libc
	DEPENDS += +libgcc
	DEPENDS += +libopenssl
	DEPENDS += +libpthread
	DEPENDS += +libstdcpp
endef

define Package/firmware-core-update_manager_interface/description
	Just firmware-core-update_manager_interface package
endef

define Package/firmware-core-update_manager_interface/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-core-update_manager_interface/* $(1)/
endef

$(eval $(call BuildPackage,firmware-core-update_manager_interface))
