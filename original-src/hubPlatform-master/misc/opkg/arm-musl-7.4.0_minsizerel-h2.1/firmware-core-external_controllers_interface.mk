define Package/firmware-core-external_controllers_interface
	$(call Package/hub-platform/Default)
	TITLE=firmware-core-external_controllers_interface
	VERSION=1.0
	EXTRA_DEPENDS += firmware-common-dbclient (=1.0),
	EXTRA_DEPENDS += firmware-common-service_interfaces (=1.0),
	EXTRA_DEPENDS += firmware-core-icc_interface (=1.0),
	EXTRA_DEPENDS += libc (=1.1.23-2),
	EXTRA_DEPENDS += libgcc (=7.4.0-2),
	EXTRA_DEPENDS += libstdcpp (=7.4.0-2),
	DEPENDS += +firmware-common-dbclient
	DEPENDS += +firmware-common-service_interfaces
	DEPENDS += +firmware-core-icc_interface
	DEPENDS += +libc
	DEPENDS += +libgcc
	DEPENDS += +libstdcpp
endef

define Package/firmware-core-external_controllers_interface/description
	Just firmware-core-external_controllers_interface package
endef

define Package/firmware-core-external_controllers_interface/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-core-external_controllers_interface/* $(1)/
endef

$(eval $(call BuildPackage,firmware-core-external_controllers_interface))
