define Package/firmware-core-cored_house_modes
	$(call Package/hub-platform/Default)
	TITLE=firmware-core-cored_house_modes
	VERSION=1.0
	EXTRA_DEPENDS += firmware-common-config_reader (=1.0),
	EXTRA_DEPENDS += firmware-common-dbclient (=1.0),
	EXTRA_DEPENDS += firmware-common-jsonrpc (=1.0),
	EXTRA_DEPENDS += firmware-common-log_utils (=1.0),
	EXTRA_DEPENDS += firmware-common-service_interfaces (=1.0),
	EXTRA_DEPENDS += firmware-common-tiny (=1.0),
	EXTRA_DEPENDS += firmware-core-cored_group_operations (=1.0),
	EXTRA_DEPENDS += firmware-core-cored_model (=1.0),
	EXTRA_DEPENDS += libc (=0.9.33.2-1),
	EXTRA_DEPENDS += libgcc (=4.8-linaro-1),
	EXTRA_DEPENDS += libpthread (=0.9.33.2-1),
	EXTRA_DEPENDS += libstdcpp (=4.8-linaro-1),
	DEPENDS += +firmware-common-config_reader
	DEPENDS += +firmware-common-dbclient
	DEPENDS += +firmware-common-jsonrpc
	DEPENDS += +firmware-common-log_utils
	DEPENDS += +firmware-common-service_interfaces
	DEPENDS += +firmware-common-tiny
	DEPENDS += +firmware-core-cored_group_operations
	DEPENDS += +firmware-core-cored_model
	DEPENDS += +libc
	DEPENDS += +libgcc
	DEPENDS += +libpthread
	DEPENDS += +libstdcpp
endef

define Package/firmware-core-cored_house_modes/description
	Just firmware-core-cored_house_modes package
endef

define Package/firmware-core-cored_house_modes/conffiles
/opt/firmware/etc/house_modes.cfg
endef

define Package/firmware-core-cored_house_modes/install
	$(INSTALL_DIR) $(1)/
	$(CP) $(PKG_INSTALL_DIR)/firmware-core-cored_house_modes/* $(1)/
endef

$(eval $(call BuildPackage,firmware-core-cored_house_modes))
